#ifndef BIN_HPP
#define BIN_HPP

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

// Reads an entire binary file and returns its raw contents as a std::string.
inline std::string ReadBinaryFileToString(const std::filesystem::path& file_path)
{
    std::ifstream input(file_path, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open binary file: " + file_path.string());
    }

    const std::streamsize file_size = input.tellg();
    if (file_size < 0) {
        throw std::runtime_error("Failed to determine file size for: " + file_path.string());
    }

    std::string buffer(static_cast<std::size_t>(file_size), '\0');
    input.seekg(0, std::ios::beg);

    if (!buffer.empty() && !input.read(buffer.data(), file_size)) {
        throw std::runtime_error("Failed to read binary file: " + file_path.string());
    }

    return buffer;
}

inline std::string ReadBinaryFileToString(const std::string& file_path)
{
    return ReadBinaryFileToString(std::filesystem::path(file_path));
}

// Writes raw binary data stored inside a std::string to the target file path.
inline void WriteStringToBinaryFile(const std::filesystem::path& file_path, const std::string& data)
{
    std::ofstream output(file_path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open binary file for writing: " + file_path.string());
    }

    if (!data.empty() && !output.write(data.data(), static_cast<std::streamsize>(data.size()))) {
        throw std::runtime_error("Failed to write binary file: " + file_path.string());
    }
}

inline void WriteStringToBinaryFile(const std::string& file_path, const std::string& data)
{
    WriteStringToBinaryFile(std::filesystem::path(file_path), data);
}

#endif // BIN_HPP
