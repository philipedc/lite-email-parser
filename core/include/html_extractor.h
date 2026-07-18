#ifndef HTML_EXTRACTOR_H
#define HTML_EXTRACTOR_H

#include <string>
#include <vector>

namespace liteEmailParser {

// Represents a file attachment (mirrors IFile from types.ts)
struct Attachment {
    std::string name;          // filename (defaults to current date if absent)
    std::string type;          // MIME content type (e.g. "image/png")
    std::size_t size = 0;      // size in bytes
    std::string buffer;        // raw binary content
    std::string src;           // uploaded URL (populated by replaceSrc after upload)
    std::string originalSrc;   // base64 data URI for inline attachments (e.g. "data:image/png;base64,...")
};

// Full result of parsing an email
struct ParseEmailResult {
    std::string subject;
    std::string from;
    std::string to;
    std::string text;                          // cleaned HTML (or plain text fallback)
    std::vector<Attachment> attachments;        // regular attachments
    std::vector<Attachment> inlineAttachments;  // inline (cid-referenced) attachments
};

// Result of HTML extraction from a raw email buffer
struct ExtractionResult {
    std::string body;   // HTML content (or plain text fallback)
    bool isHtml;        // true if body came from a text/html part
};

// Main entry point: parses a raw .eml buffer into a full result.
ParseEmailResult parseEmail(const std::string& rawEmail);

// Main entry point: extracts the HTML (or text) body from a raw .eml buffer.
// Parses MIME boundaries, finds the text/html part (falls back to text/plain),
// and decodes Content-Transfer-Encoding (quoted-printable or base64).
ExtractionResult extractHtmlBody(const std::string& rawEmail);

// Reads an .eml file from disk into a string buffer
std::string readEmailFile(const std::string& filePath);

// Replaces img src attributes in HTML: for each file with originalSrc and src set,
// finds matching img elements and swaps the src to the new URL.
std::string replaceSrc(const std::string& html, const std::vector<Attachment>& files);

namespace detail {

// Represents a single MIME part extracted from the email
struct MimePart {
    std::string contentType;       // e.g. "text/html; charset=\"UTF-8\""
    std::string transferEncoding;  // e.g. "quoted-printable", "base64"
    std::string contentId;         // e.g. "ii_mrnoxu771" (without angle brackets)
    std::string contentDisposition; // e.g. "attachment", "inline"
    std::string filename;          // from Content-Disposition or Content-Type name=
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

// Encode binary data to base64
std::string encodeBase64(const std::string& input);

// Extract the MIME type (e.g. "image/png") from a Content-Type header value
std::string extractMimeType(const std::string& contentTypeHeader);

// Extract Content-ID value, stripping angle brackets
std::string extractContentId(const std::string& contentIdHeader);

// Extract filename from Content-Disposition or Content-Type name= parameter
std::string extractFilename(const std::string& contentDisposition,
                            const std::string& contentType);

// Replace cid: references in HTML with base64 data URIs using the provided parts
std::string replaceCidWithBase64(const std::string& html,
                                 const std::vector<MimePart>& parts);

}  // namespace detail

}  // namespace liteEmailParser

#endif  // HTML_EXTRACTOR_H
