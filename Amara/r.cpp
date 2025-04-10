#include <filesystem>
#include <fstream>
#include <iostream>

#include "runtime/hermes/InstallEngine.h"


std::shared_ptr<StringBuffer> loadFileIntoBuffer(const std::string &filename) {
    // Get file size efficiently using filesystem
    std::error_code ec;
    const auto fileSize = std::filesystem::file_size(filename, ec);
    if (ec) {
        std::cerr << "Failed to get size of " << filename << ": " << ec.message() << std::endl;
        return nullptr;
    }

    // Open file in binary mode
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return nullptr;
    }

    std::string fileContent;
    fileContent.resize(fileSize); // Resize the string to accommodate fileSize bytes

    file.read(&fileContent[0], fileSize); // Read directly into the string buffer
    size_t bytesRead = file.gcount(); // Get actual number of bytes read

    if (file.bad()) {
        fileContent.clear(); // Clear content if there was an error
    } else {
        fileContent.resize(bytesRead); // Resize to actual bytes read
    }

    if (fileContent.size() != fileSize) {
        std::cerr << "Error reading file: Expected " << fileSize << " bytes, got " << fileContent.size() << std::endl;
        return nullptr;
    }
    // Return the buffer with moved content to avoid copying
    return std::make_shared<StringBuffer>(std::move(fileContent));
}

int main() {
    auto hbc = loadFileIntoBuffer("../../f.js");
    auto engine = installEngine();

    try {

        engine->execute(std::move(hbc));

    } catch (JSError &error) {
        std::cout << error.getMessage() << "\n" << error.getStack() << std::endl;
    }

    engine.reset();
    return 0;
}
