#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <sstream>
#include <regex>

#include "gumbo.h"
#include "html_parser.h"

// Helper: Converts hex characters to integer values
static int hexValue(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;
}

// 1. Decodes Quoted-Printable emails so formatting/IDs don't break across lines
static std::string decodeQuotedPrintable(const std::string& input) {
    std::ostringstream output;
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '=') {
            if (i + 1 < input.length() && input[i + 1] == '\r') {
                if (i + 2 < input.length() && input[i + 2] == '\n') { i += 2; continue; }
                i += 1; continue;
            }
            if (i + 1 < input.length() && input[i + 1] == '\n') { i += 1; continue; }
            
            if (i + 2 < input.length()) {
                int high = hexValue(input[i + 1]);
                int low = hexValue(input[i + 2]);
                if (high != -1 && low != -1) {
                    output << (char)((high << 4) | low);
                    i += 2;
                    continue;
                }
            }
        }
        output << input[i];
    }
    return output.str();
}

static bool hasAttributeValue(const GumboElement* element, const std::string& attr_name, const std::string& target_value, bool partial_match = false) {
    GumboAttribute* attr = gumbo_get_attribute(&element->attributes, attr_name.c_str());
    if (!attr) return false;

    std::string value = attr->value;
    value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
    value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    if (partial_match) {
        return value.find(target_value) != std::string::npos;
    }
    return value == target_value;
}

static bool isSignatureNode(const GumboElement* element) {
    if (element->tag == GUMBO_TAG_DIV) {
        if (hasAttributeValue(element, "id", "signature", true) || 
            hasAttributeValue(element, "class", "signature", true) ||
            hasAttributeValue(element, "class", "gmail_signature", true)) {
            return true;
        }
    } else if (element->tag == GUMBO_TAG_SPAN) {
        if (hasAttributeValue(element, "class", "gmail_signature_prefix", true) ||
            hasAttributeValue(element, "class", "gmail_signature", true)) {
            return true;
        }
    }
    return false;
}

static bool isReplyNodeAndFollowing(const GumboElement* element) {
    if (element->tag == GUMBO_TAG_DIV) {
        if (hasAttributeValue(element, "class", "gmail_quote", true) ||
            hasAttributeValue(element, "id", "divrplyfwdmsg", false) ||
            hasAttributeValue(element, "id", "quote", true) ||
            hasAttributeValue(element, "id", "reply", true) ||
            hasAttributeValue(element, "class", "quote", true) ||
            hasAttributeValue(element, "class", "reply", true)) {
            return true;
        }
    } else if (element->tag == GUMBO_TAG_BLOCKQUOTE) {
        if (hasAttributeValue(element, "type", "cite", false)) {
            return true;
        }
    }
    return false;
}

static bool isGmailChip(const GumboElement* element) {
    return hasAttributeValue(element, "class", "gmail_chip", true);
}

// 3. Rebuilds the DOM node framework into functional HTML
static std::string rebuildHtml(GumboNode* node, bool remove_signatures = false, bool remove_replies = false, bool remove_dividers = false, bool* stop_rendering = nullptr) {
    if (stop_rendering && *stop_rendering) return "";

    if (node->type == GUMBO_NODE_TEXT) {
        std::string text = node->v.text.text;
        if (remove_dividers && stop_rendering) {
            std::regex divider_regex("-{5,}.*?-{5,}");
            std::smatch match;
            if (std::regex_search(text, match, divider_regex)) {
                *stop_rendering = true;
                return text.substr(0, match.position());
            }
        }
        return text;
    }
    if (node->type == GUMBO_NODE_ELEMENT) {
        std::string html = "";
        GumboElement* element = &node->v.element;
        
        if (remove_signatures && isSignatureNode(element)) {
            return "";
        }
        if (remove_replies && isGmailChip(element)) {
            return "";
        }
        
        const char* tag_name = gumbo_normalized_tagname(element->tag);
        bool write_tags = (element->tag != GUMBO_TAG_UNKNOWN);
        
        if (write_tags) {
            html += "<" + std::string(tag_name);
            for (unsigned int i = 0; i < element->attributes.length; ++i) {
                GumboAttribute* attr = (GumboAttribute*)element->attributes.data[i];
                html += " " + std::string(attr->name) + "=\"" + std::string(attr->value) + "\"";
            }
            html += ">";
        }

        GumboVector* children = &element->children;
        for (unsigned int i = 0; i < children->length; ++i) {
            if (stop_rendering && *stop_rendering) break;
            
            GumboNode* child = (GumboNode*)children->data[i];
            if (remove_replies && child->type == GUMBO_NODE_ELEMENT) {
                if (isReplyNodeAndFollowing(&child->v.element)) {
                    break; // Stop rendering this child and all following siblings
                }
            }
            html += rebuildHtml(child, remove_signatures, remove_replies, remove_dividers, stop_rendering);
        }

        if (write_tags && element->tag != GUMBO_TAG_AREA && element->tag != GUMBO_TAG_BASE && 
            element->tag != GUMBO_TAG_BR && element->tag != GUMBO_TAG_IMG && element->tag != GUMBO_TAG_INPUT) {
            html += "</" + std::string(tag_name) + ">";
        }
        return html;
    }
    return "";
}

namespace liteEmailParser {

std::string removeSignature(std::string html) {
    // Clean email transmission artifacts first
    std::string decodedHtml = decodeQuotedPrintable(html);
    
    GumboOutput* output = gumbo_parse(decodedHtml.c_str());
    std::string cleanedHtml = rebuildHtml(output->root, true, false);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    
    return cleanedHtml;
}

std::string removeReplies(std::string html) {
    std::string decodedHtml = decodeQuotedPrintable(html);
    
    GumboOutput* output = gumbo_parse(decodedHtml.c_str());
    std::string cleanedHtml = rebuildHtml(output->root, false, true);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    
    return cleanedHtml;
}

std::string removeDividers(std::string html) {
    std::string decodedHtml = decodeQuotedPrintable(html);
    
    GumboOutput* output = gumbo_parse(decodedHtml.c_str());
    bool stop_rendering = false;
    std::string cleanedHtml = rebuildHtml(output->root, false, false, true, &stop_rendering);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    
    return cleanedHtml;
}

std::string cleanHtml(std::string html) {
    std::string decodedHtml = decodeQuotedPrintable(html);
    
    GumboOutput* output = gumbo_parse(decodedHtml.c_str());
    bool stop_rendering = false;
    std::string cleanedHtml = rebuildHtml(output->root, true, true, true, &stop_rendering);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    
    return cleanedHtml;
}

} // namespace liteEmailParser