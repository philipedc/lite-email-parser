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
    std::string lineEnding = "\r\n";
    if (headerEnd == std::string::npos) {
        headerEnd = rawEmail.find("\n\n");
        lineEnding = "\n";
    }
    if (headerEnd == std::string::npos) {
        return result;
    }

    std::string headers = rawEmail.substr(0, headerEnd);
    std::string body = rawEmail.substr(headerEnd + lineEnding.size() * 2);

    // Extract top-level headers
    result.subject = detail::getHeaderValue(headers, "Subject");
    result.from = detail::getHeaderValue(headers, "From");
    result.to = detail::getHeaderValue(headers, "To");

    // Parse MIME parts once
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

    // Find HTML body (or fall back to text/plain) from the parsed parts
    for (const auto& part : parts) {
        std::string ct = part.contentType;
        std::transform(ct.begin(), ct.end(), ct.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (ct.find("text/html") != std::string::npos) {
            std::string enc = part.transferEncoding;
            std::transform(enc.begin(), enc.end(), enc.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (enc.find("quoted-printable") != std::string::npos) {
                result.text = detail::decodeQuotedPrintable(part.body);
            } else if (enc.find("base64") != std::string::npos) {
                result.text = detail::decodeBase64(part.body);
            } else {
                result.text = part.body;
            }
            break;
        }
    }

    // Fallback to text/plain if no HTML found
    if (result.text.empty()) {
        for (const auto& part : parts) {
            std::string ct = part.contentType;
            std::transform(ct.begin(), ct.end(), ct.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (ct.find("text/plain") != std::string::npos) {
                std::string enc = part.transferEncoding;
                std::transform(enc.begin(), enc.end(), enc.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                if (enc.find("quoted-printable") != std::string::npos) {
                    result.text = detail::decodeQuotedPrintable(part.body);
                } else if (enc.find("base64") != std::string::npos) {
                    result.text = detail::decodeBase64(part.body);
                } else {
                    result.text = part.body;
                }
                break;
            }
        }
    }

    // Replace cid: references with base64 data URIs (using same parts, no re-parse)
    if (!result.text.empty() && !parts.empty()) {
        result.text = detail::replaceCidWithBase64(result.text, parts);
    }

    // Extract attachments from parsed MIME parts
    for (const auto& part : parts) {
        std::string ct = part.contentType;
        std::transform(ct.begin(), ct.end(), ct.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        // Skip text parts (already used for body)
        if (ct.find("text/html") != std::string::npos ||
            ct.find("text/plain") != std::string::npos) {
            continue;
        }

        // Skip parts with no content type (shouldn't happen, but guard)
        if (ct.empty()) continue;

        // Decode the attachment body
        std::string rawData;
        std::string enc = part.transferEncoding;
        std::transform(enc.begin(), enc.end(), enc.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (enc.find("base64") != std::string::npos) {
            rawData = detail::decodeBase64(part.body);
        } else if (enc.find("quoted-printable") != std::string::npos) {
            rawData = detail::decodeQuotedPrintable(part.body);
        } else {
            rawData = part.body;
        }

        Attachment att;
        att.name = part.filename.empty() ? "attachment" : part.filename;
        att.type = detail::extractMimeType(part.contentType);
        att.size = rawData.size();
        att.buffer = std::move(rawData);

        // If it has a Content-ID, it's an inline attachment
        if (!part.contentId.empty()) {
            att.originalSrc = "data:" + att.type + ";base64," + detail::encodeBase64(att.buffer);
            result.inlineAttachments.push_back(std::move(att));
        } else {
            result.attachments.push_back(std::move(att));
        }
    }

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

std::string replaceSrc(const std::string& html, const std::vector<Attachment>& files) {
    std::string result = html;

    for (const auto& file : files) {
        if (file.originalSrc.empty() || file.src.empty()) {
            continue;
        }

        // Find and replace src="<originalSrc>" with src="<newSrc>"
        std::string searchAttr = "src=\"" + file.originalSrc + "\"";
        std::string replaceAttr = "src=\"" + file.src + "\"";

        std::string::size_type pos = 0;
        while ((pos = result.find(searchAttr, pos)) != std::string::npos) {
            result.replace(pos, searchAttr.size(), replaceAttr);
            pos += replaceAttr.size();
        }
    }

    return result;
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
            std::string partCid = getHeaderValue(partHeaders, "Content-ID");
            std::string partDisp = getHeaderValue(partHeaders, "Content-Disposition");

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
                part.contentId = extractContentId(partCid);
                part.contentDisposition = partDisp;
                part.filename = extractFilename(partDisp, partCt);
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

std::string encodeBase64(const std::string& input) {
    std::string out;
    const auto* data = reinterpret_cast<const unsigned char*>(input.data());
    std::size_t len = input.size();
    out.reserve(((len + 2) / 3) * 4);

    std::size_t i = 0;
    while (i + 3 <= len) {
        std::uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out.push_back(kEncodeTable[(n >> 18) & 0x3F]);
        out.push_back(kEncodeTable[(n >> 12) & 0x3F]);
        out.push_back(kEncodeTable[(n >> 6) & 0x3F]);
        out.push_back(kEncodeTable[n & 0x3F]);
        i += 3;
    }

    if (std::size_t rem = len - i; rem > 0) {
        std::uint32_t n = data[i] << 16;
        if (rem == 2) n |= data[i + 1] << 8;
        out.push_back(kEncodeTable[(n >> 18) & 0x3F]);
        out.push_back(kEncodeTable[(n >> 12) & 0x3F]);
        out.push_back(rem == 2 ? kEncodeTable[(n >> 6) & 0x3F] : '=');
        out.push_back('=');
    }
    return out;
}

std::string extractMimeType(const std::string& contentTypeHeader) {
    // Get just the type/subtype (e.g. "image/png") before any semicolons
    std::string::size_type semi = contentTypeHeader.find(';');
    std::string mimeType = (semi != std::string::npos)
        ? contentTypeHeader.substr(0, semi)
        : contentTypeHeader;
    // Trim whitespace
    while (!mimeType.empty() && (mimeType.back() == ' ' || mimeType.back() == '\t')) {
        mimeType.pop_back();
    }
    while (!mimeType.empty() && (mimeType.front() == ' ' || mimeType.front() == '\t')) {
        mimeType.erase(mimeType.begin());
    }
    return mimeType;
}

std::string extractContentId(const std::string& contentIdHeader) {
    // Strip angle brackets: "<ii_mrnoxu771>" -> "ii_mrnoxu771"
    std::string cid = contentIdHeader;
    if (!cid.empty() && cid.front() == '<') {
        cid.erase(cid.begin());
    }
    if (!cid.empty() && cid.back() == '>') {
        cid.pop_back();
    }
    return cid;
}

std::string extractFilename(const std::string& contentDisposition,
                            const std::string& contentType) {
    // Try Content-Disposition filename= first
    auto extractParam = [](const std::string& header, const std::string& param) -> std::string {
        std::string lower = header;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        std::string::size_type pos = lower.find(param + "=");
        if (pos == std::string::npos) return "";
        pos += param.size() + 1;
        if (pos < header.size() && header[pos] == '"') {
            pos++;
            std::string::size_type end = header.find('"', pos);
            if (end != std::string::npos) return header.substr(pos, end - pos);
        } else {
            std::string::size_type end = pos;
            while (end < header.size() && header[end] != ';' &&
                   header[end] != ' ' && header[end] != '\r' && header[end] != '\n') {
                end++;
            }
            return header.substr(pos, end - pos);
        }
        return "";
    };

    std::string filename = extractParam(contentDisposition, "filename");
    if (filename.empty()) {
        filename = extractParam(contentType, "name");
    }
    return filename;
}

std::string replaceCidWithBase64(const std::string& html,
                                 const std::vector<MimePart>& parts) {
    std::string result = html;

    for (const auto& part : parts) {
        if (part.contentId.empty()) continue;

        // Build the cid: reference to search for
        std::string cidRef = "cid:" + part.contentId;

        // Check if this cid is referenced in the HTML
        if (result.find(cidRef) == std::string::npos) continue;

        // Decode the attachment body (typically base64-encoded in email)
        std::string rawData;
        std::string enc = part.transferEncoding;
        std::transform(enc.begin(), enc.end(), enc.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (enc.find("base64") != std::string::npos) {
            rawData = decodeBase64(part.body);
        } else {
            rawData = part.body;
        }

        // Re-encode to base64 (clean, no line breaks) for data URI
        std::string base64Data = encodeBase64(rawData);

        // Build data URI: data:image/png;base64,iVBOR...
        std::string mimeType = extractMimeType(part.contentType);
        std::string dataUri = "data:" + mimeType + ";base64," + base64Data;

        // Replace all occurrences of cid:xxx with the data URI
        std::string::size_type pos = 0;
        while ((pos = result.find(cidRef, pos)) != std::string::npos) {
            result.replace(pos, cidRef.size(), dataUri);
            pos += dataUri.size();
        }
    }

    return result;
}

}  // namespace detail

}  // namespace liteEmailParser
