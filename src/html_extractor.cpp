#include "html_extractor.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace liteEmailParser {

ParseEmailResult parseEmail(const std::string& rawEmail) {
    ParseEmailResult result;

    if (rawEmail.empty()) {
        return result;
    }

    // Split headers from body (separated by first blank line)
    std::string::size_type headerEnd = rawEmail.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = rawEmail.find("\n\n");
    }

    std::string headers;
    if (headerEnd != std::string::npos) {
        headers = rawEmail.substr(0, headerEnd);
    }

    // Extract top-level headers
    result.subject = detail::getHeaderValue(headers, "Subject");
    result.from = detail::getHeaderValue(headers, "From");
    result.to = detail::getHeaderValue(headers, "To");

    // Extract HTML body
    ExtractionResult extraction = extractHtmlBody(rawEmail);
    result.text = extraction.body;

    // TODO: attachment extraction will be added here later

    return result;
}

std::string readEmailFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: could not open file: " << filePath << std::endl;
        return "";
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

ExtractionResult extractHtmlBody(const std::string& rawEmail) {
    if (rawEmail.empty()) {
        return {"", false};
    }

    // Split headers from body (separated by first blank line)
    std::string::size_type headerEnd = rawEmail.find("\r\n\r\n");
    std::string lineEnding = "\r\n";
    if (headerEnd == std::string::npos) {
        headerEnd = rawEmail.find("\n\n");
        lineEnding = "\n";
    }
    if (headerEnd == std::string::npos) {
        // No body found — treat entire content as body? Return empty.
        return {"", false};
    }

    std::string headers = rawEmail.substr(0, headerEnd);
    std::string body = rawEmail.substr(headerEnd + lineEnding.size() * 2);

    // Get the top-level Content-Type to find the boundary
    std::string contentType = detail::getHeaderValue(headers, "Content-Type");
    std::string boundary = detail::extractBoundary(contentType);

    std::vector<detail::MimePart> parts;
    if (!boundary.empty()) {
        parts = detail::parseMimeParts(body, boundary);
    } else {
        // Not multipart — single body
        std::string encoding = detail::getHeaderValue(headers, "Content-Transfer-Encoding");
        detail::MimePart single;
        single.contentType = contentType;
        single.transferEncoding = encoding;
        single.body = body;
        parts.push_back(single);
    }

    // First pass: look for text/html
    for (const auto& part : parts) {
        std::string ct = part.contentType;
        std::transform(ct.begin(), ct.end(), ct.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (ct.find("text/html") != std::string::npos) {
            std::string decoded = part.body;
            std::string enc = part.transferEncoding;
            std::transform(enc.begin(), enc.end(), enc.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (enc.find("quoted-printable") != std::string::npos) {
                decoded = detail::decodeQuotedPrintable(part.body);
            } else if (enc.find("base64") != std::string::npos) {
                decoded = detail::decodeBase64(part.body);
            }
            return {decoded, true};
        }
    }

    // Second pass: fall back to text/plain
    for (const auto& part : parts) {
        std::string ct = part.contentType;
        std::transform(ct.begin(), ct.end(), ct.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (ct.find("text/plain") != std::string::npos) {
            std::string decoded = part.body;
            std::string enc = part.transferEncoding;
            std::transform(enc.begin(), enc.end(), enc.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (enc.find("quoted-printable") != std::string::npos) {
                decoded = detail::decodeQuotedPrintable(part.body);
            } else if (enc.find("base64") != std::string::npos) {
                decoded = detail::decodeBase64(part.body);
            }
            return {decoded, false};
        }
    }

    // Nothing found
    return {"", false};
}

namespace detail {

std::string extractBoundary(const std::string& contentTypeHeader) {
    // Look for boundary= in the Content-Type header
    std::string lower = contentTypeHeader;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::string::size_type pos = lower.find("boundary=");
    if (pos == std::string::npos) {
        return "";
    }

    pos += 9;  // skip "boundary="
    std::string boundary;
    if (pos < contentTypeHeader.size() && contentTypeHeader[pos] == '"') {
        // Quoted boundary
        pos++;
        std::string::size_type end = contentTypeHeader.find('"', pos);
        if (end != std::string::npos) {
            boundary = contentTypeHeader.substr(pos, end - pos);
        }
    } else {
        // Unquoted boundary — ends at semicolon, whitespace, or end of string
        std::string::size_type end = pos;
        while (end < contentTypeHeader.size() &&
               contentTypeHeader[end] != ';' &&
               contentTypeHeader[end] != ' ' &&
               contentTypeHeader[end] != '\t' &&
               contentTypeHeader[end] != '\r' &&
               contentTypeHeader[end] != '\n') {
            end++;
        }
        boundary = contentTypeHeader.substr(pos, end - pos);
    }

    return boundary;
}

std::vector<MimePart> parseMimeParts(const std::string& rawContent,
                                     const std::string& boundary) {
    std::vector<MimePart> results;

    std::string delimiter = "--" + boundary;
    std::string endDelimiter = delimiter + "--";

    // Find all parts between delimiters
    std::string::size_type pos = rawContent.find(delimiter);
    if (pos == std::string::npos) {
        return results;
    }

    // Skip past the first delimiter line
    pos += delimiter.size();
    // Skip to end of line
    if (pos < rawContent.size() && rawContent[pos] == '\r') pos++;
    if (pos < rawContent.size() && rawContent[pos] == '\n') pos++;

    while (pos < rawContent.size()) {
        // Find the next delimiter
        std::string::size_type nextPos = rawContent.find(delimiter, pos);
        if (nextPos == std::string::npos) {
            break;
        }

        // Extract this part's content (between current pos and next delimiter)
        std::string partContent = rawContent.substr(pos, nextPos - pos);

        // Remove trailing \r\n before the delimiter
        if (partContent.size() >= 2 &&
            partContent[partContent.size() - 2] == '\r' &&
            partContent[partContent.size() - 1] == '\n') {
            partContent.erase(partContent.size() - 2);
        } else if (partContent.size() >= 1 &&
                   partContent[partContent.size() - 1] == '\n') {
            partContent.erase(partContent.size() - 1);
        }

        // Split part into headers and body
        std::string::size_type partHeaderEnd = partContent.find("\r\n\r\n");
        std::string partLineEnding = "\r\n";
        if (partHeaderEnd == std::string::npos) {
            partHeaderEnd = partContent.find("\n\n");
            partLineEnding = "\n";
        }

        if (partHeaderEnd != std::string::npos) {
            std::string partHeaders = partContent.substr(0, partHeaderEnd);
            std::string partBody = partContent.substr(
                partHeaderEnd + partLineEnding.size() * 2);

            std::string partCt = getHeaderValue(partHeaders, "Content-Type");
            std::string partEnc = getHeaderValue(partHeaders, "Content-Transfer-Encoding");

            // Check if this part is itself multipart (nested)
            std::string nestedBoundary = extractBoundary(partCt);
            if (!nestedBoundary.empty()) {
                // Recurse into nested multipart
                auto nested = parseMimeParts(partBody, nestedBoundary);
                results.insert(results.end(), nested.begin(), nested.end());
            } else {
                MimePart part;
                part.contentType = partCt;
                part.transferEncoding = partEnc;
                part.body = partBody;
                results.push_back(part);
            }
        }

        // Move past the delimiter
        pos = nextPos + delimiter.size();
        // Check if this is the end delimiter
        if (pos + 2 <= rawContent.size() &&
            rawContent[pos] == '-' && rawContent[pos + 1] == '-') {
            break;
        }
        // Skip to end of line
        if (pos < rawContent.size() && rawContent[pos] == '\r') pos++;
        if (pos < rawContent.size() && rawContent[pos] == '\n') pos++;
    }

    return results;
}

std::string getHeaderValue(const std::string& headers, const std::string& key) {
    // Case-insensitive search for "Key: value" in headers
    // Handles folded headers (continuation lines starting with whitespace)
    std::string lowerHeaders = headers;
    std::transform(lowerHeaders.begin(), lowerHeaders.end(), lowerHeaders.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::string lowerKey = key;
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    lowerKey += ":";

    std::string::size_type pos = lowerHeaders.find(lowerKey);
    if (pos == std::string::npos) {
        return "";
    }

    // Make sure it's at the start of a line (or start of headers)
    if (pos > 0 && lowerHeaders[pos - 1] != '\n') {
        // Search again from after this position
        while (pos != std::string::npos) {
            pos = lowerHeaders.find(lowerKey, pos + 1);
            if (pos != std::string::npos && (pos == 0 || lowerHeaders[pos - 1] == '\n')) {
                break;
            }
        }
        if (pos == std::string::npos) {
            return "";
        }
    }

    // Skip "Key:"
    pos += lowerKey.size();
    // Skip leading whitespace
    while (pos < headers.size() && (headers[pos] == ' ' || headers[pos] == '\t')) {
        pos++;
    }

    // Collect value including folded lines
    std::string value;
    while (pos < headers.size()) {
        if (headers[pos] == '\r') {
            pos++;
            continue;
        }
        if (headers[pos] == '\n') {
            // Check if next line is a continuation (starts with space/tab)
            if (pos + 1 < headers.size() &&
                (headers[pos + 1] == ' ' || headers[pos + 1] == '\t')) {
                value += ' ';
                pos += 2;  // skip \n and the whitespace
                // Skip additional whitespace
                while (pos < headers.size() &&
                       (headers[pos] == ' ' || headers[pos] == '\t')) {
                    pos++;
                }
                continue;
            } else {
                break;
            }
        }
        value += headers[pos];
        pos++;
    }

    return value;
}

std::string decodeQuotedPrintable(const std::string& input) {
    std::string output;
    output.reserve(input.size());

    for (std::string::size_type i = 0; i < input.size(); i++) {
        if (input[i] == '=') {
            if (i + 1 < input.size() && input[i + 1] == '\n') {
                // Soft line break (LF only)
                i += 1;
            } else if (i + 2 < input.size() &&
                       input[i + 1] == '\r' && input[i + 2] == '\n') {
                // Soft line break (CRLF)
                i += 2;
            } else if (i + 2 < input.size() &&
                       std::isxdigit(static_cast<unsigned char>(input[i + 1])) &&
                       std::isxdigit(static_cast<unsigned char>(input[i + 2]))) {
                // Hex-encoded byte
                char hex[3] = {input[i + 1], input[i + 2], '\0'};
                unsigned int byte = 0;
                std::sscanf(hex, "%x", &byte);
                output += static_cast<char>(byte);
                i += 2;
            } else {
                // Malformed — just pass through
                output += input[i];
            }
        } else {
            output += input[i];
        }
    }

    return output;
}

// Base64 decode table and decoder based on reference from base64.dev
static const char kEncodeTable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::array<int, 256> buildDecodeTable() {
    std::array<int, 256> t;
    t.fill(-1);
    for (int i = 0; i < 64; ++i) {
        t[static_cast<unsigned char>(kEncodeTable[i])] = i;
    }
    return t;
}

std::string decodeBase64(const std::string& input) {
    static const auto table = buildDecodeTable();
    std::string out;
    out.reserve((input.size() / 4) * 3);

    std::uint32_t buffer = 0;
    int bits = 0;
    for (unsigned char c : input) {
        if (c == '=') break;
        int v = table[c];
        if (v < 0) continue;  // skip whitespace / newlines
        buffer = (buffer << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((buffer >> bits) & 0xFF));
        }
    }
    return out;
}

}  // namespace detail

}  // namespace liteEmailParser
