#include "html_extractor.h"
#include "html_parser.h"

#include <iostream>
#include <string>
#include <fstream>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: email-parser <filename>\n";
        exit(EXIT_FAILURE);
    }
    const char* filename = argv[1];

    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in) {
        std::cout << "File " << filename << " not found!\n";
        exit(EXIT_FAILURE);
    }

    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();

    std::cout << "Buffer size: " << contents.size() << " bytes" << std::endl;

    // Parse the email from the raw buffer
    liteEmailParser::ParseEmailResult result = liteEmailParser::parseEmail(contents);

    std::cout << "Subject: " << result.subject << std::endl;
    std::cout << "From: " << result.from << std::endl;
    std::cout << "To: " << result.to << std::endl;
    std::cout << "Body length: " << result.text.size() << " chars" << std::endl;

    std::cout << "Attachments count: " << result.attachments.size() << std::endl;
    for (const auto& att : result.attachments) {
        std::cout << "  - " << att.name << " (" << att.type << ", " << att.size << " bytes)" << std::endl;
    }

    std::cout << "Inline Attachments count: " << result.inlineAttachments.size() << std::endl;
    for (const auto& att : result.inlineAttachments) {
        std::cout << "  - " << att.name << " (" << att.type << ", " << att.size << " bytes)";
        if (!att.originalSrc.empty()) {
            std::cout << " [originalSrc: " << att.originalSrc.substr(0, 50) << "...]";
        }
        std::cout << std::endl;
    }

    if (!result.text.empty()) {
        std::cout << "\n--- Body output (Cleaned) ---\n" << liteEmailParser::cleanHtml(result.text) << std::endl;
    } else {
        std::cout << "\n(No body extracted)" << std::endl;
    }

    return 0;
}
