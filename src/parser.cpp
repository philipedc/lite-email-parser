#include "html_extractor.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string filePath = "sample/sample1.eml";
    if (argc > 1) {
        filePath = argv[1];
    }

    std::cout << "Reading: " << filePath << std::endl;

    std::string rawEmail = liteEmailParser::readEmailFile(filePath);
    if (rawEmail.empty()) {
        std::cerr << "Failed to read email file." << std::endl;
        return 1;
    }

    std::cout << "Buffer size: " << rawEmail.size() << " bytes" << std::endl;

    liteEmailParser::ExtractionResult result = liteEmailParser::extractHtmlBody(rawEmail);

    std::cout << "Is HTML: " << (result.isHtml ? "true" : "false") << std::endl;
    std::cout << "Body length: " << result.body.size() << " chars" << std::endl;

    if (!result.body.empty()) {
        std::cout << "\n--- Body output ---\n" << result.body << std::endl;
    } else {
        std::cout << "\n(No body extracted)" << std::endl;
    }

    return 0;
}
