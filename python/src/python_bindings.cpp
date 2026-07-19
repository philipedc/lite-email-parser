#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "html_extractor.h"
#include "html_parser.h"

namespace py = pybind11;

namespace {

// Converts an Attachment into a Python dict shaped like Node's IFile,
// but in snake_case: {name, type, size, buffer, src?, original_src?}.
py::dict AttachmentToDict(const liteEmailParser::Attachment& attachment,
                           bool includeSrcFields) {
  py::dict result;
  result["name"] = attachment.name;
  result["type"] = attachment.type;
  result["size"] = attachment.size;
  result["buffer"] = py::bytes(attachment.buffer);

  if (includeSrcFields) {
    if (!attachment.originalSrc.empty()) {
      result["original_src"] = attachment.originalSrc;
    }
    if (!attachment.src.empty()) {
      result["src"] = attachment.src;
    }
  }

  return result;
}

// Accepts either str or bytes/bytearray, matching Node's Buffer-or-string
// flexibility for parse_email's raw email input.
std::string ToRawString(const py::object& emailObj) {
  if (py::isinstance<py::bytes>(emailObj) ||
      py::isinstance<py::bytearray>(emailObj)) {
    return emailObj.cast<std::string>();
  }
  if (py::isinstance<py::str>(emailObj)) {
    // Encode as UTF-8, matching how a Node Buffer would be produced from a
    // JS string containing the raw email bytes.
    py::bytes encoded = emailObj.attr("encode")("utf-8");
    return encoded.cast<std::string>();
  }
  throw py::type_error("parse_email expects str, bytes, or bytearray");
}

std::string CleanHtml(const std::string& html) {
  return liteEmailParser::cleanHtml(html);
}

std::string RemoveSignature(const std::string& html) {
  return liteEmailParser::removeSignature(html);
}

std::string RemoveReplies(const std::string& html) {
  return liteEmailParser::removeReplies(html);
}

std::string RemoveDividers(const std::string& html) {
  return liteEmailParser::removeDividers(html);
}

py::dict ParseEmail(const py::object& emailObj) {
  std::string rawEmail = ToRawString(emailObj);

  liteEmailParser::ParseEmailResult result =
      liteEmailParser::parseEmail(rawEmail);
  std::string cleaned = liteEmailParser::cleanHtml(result.text);

  py::dict output;
  output["subject"] = result.subject;
  output["from"] = result.from;
  output["to"] = result.to;
  output["last_message"] = cleaned;

  py::list attachments;
  for (const auto& attachment : result.attachments) {
    attachments.append(AttachmentToDict(attachment, /*includeSrcFields=*/false));
  }
  output["attachments"] = attachments;

  py::list inlineAttachments;
  for (const auto& attachment : result.inlineAttachments) {
    inlineAttachments.append(AttachmentToDict(attachment, /*includeSrcFields=*/true));
  }
  output["inline_attachments"] = inlineAttachments;

  return output;
}

std::string ReplaceSrc(const std::string& html, const py::list& files) {
  std::vector<liteEmailParser::Attachment> attachments;
  attachments.reserve(files.size());

  for (const py::handle& item : files) {
    if (!py::isinstance<py::dict>(item)) {
      continue;
    }
    py::dict fileDict = item.cast<py::dict>();

    liteEmailParser::Attachment attachment;
    if (fileDict.contains("original_src")) {
      attachment.originalSrc = fileDict["original_src"].cast<std::string>();
    }
    if (fileDict.contains("src")) {
      attachment.src = fileDict["src"].cast<std::string>();
    }
    attachments.push_back(std::move(attachment));
  }

  return liteEmailParser::replaceSrc(html, attachments);
}

}  // namespace

PYBIND11_MODULE(_core, module) {
  module.doc() = "Native C++ core bindings for lite-email-parser";

  module.def("clean_html", &CleanHtml, py::arg("html"));
  module.def("remove_signature", &RemoveSignature, py::arg("html"));
  module.def("remove_replies", &RemoveReplies, py::arg("html"));
  module.def("remove_dividers", &RemoveDividers, py::arg("html"));
  module.def("parse_email", &ParseEmail, py::arg("email"));
  module.def("replace_src", &ReplaceSrc, py::arg("html"), py::arg("files"));
}
