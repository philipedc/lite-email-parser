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

    liteEmailParser::ParseEmailResult result = liteEmailParser::parseEmail(rawEmail);

    std::cout << "Subject: " << result.subject << std::endl;
    std::cout << "From: " << result.from << std::endl;
    std::cout << "To: " << result.to << std::endl;
    std::cout << "Body length: " << result.text.size() << " chars" << std::endl;

    if (!result.text.empty()) {
        std::cout << "\n--- Body output ---\n" << result.text << std::endl;
    } else {
        std::cout << "\n(No body extracted)" << std::endl;
    }

    return 0;
}
