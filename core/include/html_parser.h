#ifndef HTML_PARSER_H
#define HTML_PARSER_H

#include <string>

namespace liteEmailParser {
    std::string removeSignature(std::string html);
    std::string removeReplies(std::string html);
    std::string removeDividers(std::string html);
    std::string cleanHtml(std::string html);
}

#endif // HTML_PARSER_H
