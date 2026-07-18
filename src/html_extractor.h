#ifndef HTML_EXTRACTOR_H
#define HTML_EXTRACTOR_H

#include <string>
#include <vector>

namespace liteEmailParser {

// Result of HTML extraction from a raw email buffer
struct ExtractionResult {
    std::string body;   // HTML content (or plain text fallback)
    bool isHtml;        // true if body came from a text/html part
};

// Main entry point: extracts the HTML (or text) body from a raw .eml buffer.
// Parses MIME boundaries, finds the text/html part (falls back to text/plain),
// and decodes Content-Transfer-Encoding (quoted-printable or base64).
ExtractionResult extractHtmlBody(const std::string& rawEmail);

// Reads an .eml file from disk into a string buffer
std::string readEmailFile(const std::string& filePath);

namespace detail {

// Represents a single MIME part extracted from the email
struct MimePart {
    std::string contentType;       // e.g. "text/html; charset=\"UTF-8\""
    std::string transferEncoding;  // e.g. "quoted-printable", "base64"
    std::string body;              // raw body content of this part
};

// Parse the top-level Content-Type header to extract the boundary string
std::string extractBoundary(const std::string& contentTypeHeader);

// Split a raw email (or a multipart section) into its MIME parts by boundary.
// Recurses into nested multipart parts.
std::vector<MimePart> parseMimeParts(const std::string& rawContent,
                                     const std::string& boundary);

// Extract a header value from raw headers block (case-insensitive key match)
std::string getHeaderValue(const std::string& headers, const std::string& key);

// Decode quoted-printable encoded content
std::string decodeQuotedPrintable(const std::string& input);

// Decode base64 encoded content
std::string decodeBase64(const std::string& input);

}  // namespace detail

}  // namespace liteEmailParser

#endif  // HTML_EXTRACTOR_H
