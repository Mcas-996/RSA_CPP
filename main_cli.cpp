#include "RSA.hpp"
#include "bin.hpp"
#include "third_party/cppcodec/cppcodec/base64_rfc4648.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string encodeCiphertextBase64(const std::vector<long long>& values) {
    if (values.empty()) {
        return {};
    }
    std::vector<uint8_t> bytes;
    bytes.reserve(values.size() * sizeof(uint64_t));
    for (long long value : values) {
        uint64_t uvalue = static_cast<uint64_t>(value);
        for (int shift = 0; shift < 64; shift += 8) {
            bytes.push_back(static_cast<uint8_t>((uvalue >> shift) & 0xFF));
        }
    }
    return cppcodec::base64_rfc4648::encode(bytes);
}

std::vector<long long> decodeCiphertextBase64(const std::string& encoded) {
    std::vector<uint8_t> bytes = cppcodec::base64_rfc4648::decode(encoded);
    if (bytes.size() % sizeof(uint64_t) != 0) {
        throw std::invalid_argument("Base64 ciphertext length mismatch");
    }
    std::vector<long long> values(bytes.size() / sizeof(uint64_t));
    for (size_t i = 0; i < values.size(); ++i) {
        uint64_t value = 0;
        for (int byte = 0; byte < 8; ++byte) {
            value |= static_cast<uint64_t>(bytes[i * 8 + byte]) << (byte * 8);
        }
        values[i] = static_cast<long long>(value);
    }
    return values;
}

std::string stripWhitespace(const std::string& input) {
    std::string cleaned;
    cleaned.reserve(input.size());
    for (unsigned char c : input) {
        if (!std::isspace(c)) {
            cleaned.push_back(static_cast<char>(c));
        }
    }
    return cleaned;
}

bool isNumericCiphertext(const std::string& input) {
    if (input.empty()) {
        return true;
    }
    return input.find_first_not_of("0123456789,-") == std::string::npos;
}

std::string trim(const std::string& input) {
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::string stripSurroundingQuotes(const std::string& input) {
    if (input.size() >= 2) {
        const char first = input.front();
        const char last = input.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return input.substr(1, input.size() - 2);
        }
    }
    return input;
}

std::vector<long long> parseCiphertext(const std::string& input) {
    const std::string cleaned = stripWhitespace(input);
    if (cleaned.empty()) {
        return {};
    }
    try {
        return decodeCiphertextBase64(cleaned);
    } catch (const std::exception&) {
        if (isNumericCiphertext(cleaned)) {
            return RSAUtil::stringToCiphertext(cleaned);
        }
        throw;
    }
}

std::string encodeBase64(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return {};
    }
    return cppcodec::base64_rfc4648::encode(data);
}

std::vector<uint8_t> decodeBase64(const std::string& text) {
    return cppcodec::base64_rfc4648::decode(stripWhitespace(text));
}

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("Unable to open text file: " + path.string());
    }
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

enum class Mode {
    Legacy = 0,
    Pem = 1
};

const char* modeName(Mode mode) {
    return mode == Mode::Legacy ? "Legacy long long mode" : "PEM/OpenSSL mode";
}

struct LegacyState {
    RSAUtil::KeyPair keyPair{};
    bool hasKey = false;
};

struct PemState {
    RSAUtil::PemKeyPair keyPair{};
    bool hasPublic = false;
    bool hasPrivate = false;
    int keyBits = 2048;
};

int readInt(const std::string& prompt, int minValue, int maxValue) {
    while (true) {
        std::cout << prompt;
        int value;
        if (std::cin >> value) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (value < minValue || value > maxValue) {
                std::cout << "Input out of range. Please enter a value between " << minValue << " and " << maxValue << ".\n";
                continue;
            }
            return value;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input; please enter a number.\n";
    }
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void printSeparator() {
    std::cout << "----------------------------------------" << std::endl;
}

} // namespace

int main() {
    std::cout << "RSA Encryption/Decryption CLI Tool" << std::endl;
    std::cout << "===================================" << std::endl;

    LegacyState legacy;
    PemState pem;
    Mode mode = Mode::Pem;
    std::string result;

    while (true) {
        printSeparator();
        std::cout << "Current mode: " << modeName(mode) << std::endl;
        std::cout << "1. Switch mode" << std::endl;
        std::cout << "2. Generate key pair (current mode)" << std::endl;
        std::cout << "3. Import keys" << std::endl;
        std::cout << "4. Show current keys" << std::endl;
        std::cout << "5. Encrypt text" << std::endl;
        std::cout << "6. Decrypt text" << std::endl;
        std::cout << "7. Save string to binary file" << std::endl;
        std::cout << "8. Encrypt binary file" << std::endl;
        std::cout << "9. Decrypt Base64 ciphertext to binary" << std::endl;
        std::cout << "10. Load binary file into memory" << std::endl;
        std::cout << "11. Encrypt file to file" << std::endl;
        std::cout << "12. Decrypt file to file" << std::endl;
        std::cout << "13. Exit" << std::endl;

        const int choice = readInt("Choose an option (1-13): ", 1, 13);

        switch (choice) {
        case 1: {
            mode = (mode == Mode::Legacy) ? Mode::Pem : Mode::Legacy;
            std::cout << "Switched to " << modeName(mode) << std::endl;
            break;
        }
        case 2: {
            if (mode == Mode::Legacy) {
                legacy.keyPair = RSAUtil::generateKeyPair();
                legacy.hasKey = true;
                std::cout << "Generated legacy key pair." << std::endl;
                RSAUtil::printKeyInfo(legacy.keyPair);
            } else {
                std::string bitsInput = stripWhitespace(readLine("Enter key size in bits (>=512, default " + std::to_string(pem.keyBits) + "): "));
                int bits = pem.keyBits;
                if (!bitsInput.empty()) {
                    try {
                        bits = std::stoi(bitsInput);
                    } catch (const std::exception&) {
                        std::cout << "Invalid number, using default " << bits << " bits." << std::endl;
                    }
                }
                bits = std::clamp(bits, 512, 16384);
                try {
                    pem.keyPair = RSAUtil::generatePemKeyPair(bits);
                    pem.hasPublic = true;
                    pem.hasPrivate = true;
                    pem.keyBits = pem.keyPair.keyBits;
                    std::cout << "Generated PEM key pair, " << pem.keyPair.keyBits << " bits." << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "Key generation failed: " << e.what() << std::endl;
                }
            }
            break;
        }
        case 3: {
            if (mode == Mode::Legacy) {
                if (!legacy.hasKey) {
                    legacy.keyPair = RSAUtil::KeyPair();
                }
                legacy.keyPair.publicKey = stripWhitespace(readLine("Enter public exponent (e): "));
                legacy.keyPair.privateKey = stripWhitespace(readLine("Enter private exponent (d): "));
                legacy.keyPair.modulus = stripWhitespace(readLine("Enter modulus (n): "));
                legacy.hasKey = true;
                std::cout << "Legacy key stored." << std::endl;
            } else {
                try {
                    std::string publicPath = trim(readLine("Enter public key PEM path: "));
                    if (publicPath.empty()) {
                        throw std::runtime_error("Public key path may not be empty");
                    }
                    const std::string publicPem = readTextFile(publicPath);
                    std::string privatePath = trim(readLine("Enter private key PEM path (optional, needed for decrypt): "));
                    std::string privatePem;
                    if (!privatePath.empty()) {
                        privatePem = readTextFile(privatePath);
                    }
                    pem.keyPair.publicKeyPem = publicPem;
                    pem.keyPair.privateKeyPem = privatePem;
                    try {
                        pem.keyPair.keyBits = RSAUtil::getKeyBitsFromPublicKey(publicPem);
                        pem.keyBits = pem.keyPair.keyBits;
                    } catch (const std::exception&) {
                        pem.keyPair.keyBits = 0;
                    }
                    pem.hasPublic = !publicPem.empty();
                    pem.hasPrivate = !privatePem.empty();
                    if (pem.hasPublic) {
                        std::cout << "Loaded public key." << std::endl;
                    }
                    if (pem.hasPrivate) {
                        std::cout << "Loaded private key." << std::endl;
                    } else {
                        std::cout << "Private key missing; only encryption is available." << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cout << "Failed to load key: " << e.what() << std::endl;
                }
            }
            break;
        }
        case 4: {
            if (mode == Mode::Legacy) {
                if (legacy.hasKey) {
                    RSAUtil::printKeyInfo(legacy.keyPair);
                } else {
                    std::cout << "Legacy key not set." << std::endl;
                }
            } else {
                if (!pem.hasPublic && !pem.hasPrivate) {
                    std::cout << "No PEM keys loaded." << std::endl;
                } else {
                    if (pem.keyPair.keyBits > 0) {
                        std::cout << "Key length: " << pem.keyPair.keyBits << " bits" << std::endl;
                    }
                    if (pem.hasPublic) {
                        std::cout << "\n--- Public key PEM ---\n" << pem.keyPair.publicKeyPem << std::endl;
                    }
                    if (pem.hasPrivate) {
                        std::cout << "\n--- Private key PEM ---\n" << pem.keyPair.privateKeyPem << std::endl;
                    }
                }
            }
            break;
        }
        case 5: {
            if (mode == Mode::Legacy) {
                if (!legacy.hasKey) {
                    std::cout << "Generate or import legacy keys first." << std::endl;
                    break;
                }
                const std::string plaintext = readLine("Text to encrypt: ");
                try {
                    std::vector<long long> encrypted = RSAUtil::encryptText(plaintext, legacy.keyPair);
                    result = encodeCiphertextBase64(encrypted);
                    std::cout << "Encryption complete. Base64 ciphertext:\n" << result << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "Encryption failed: " << e.what() << std::endl;
                }
            } else {
                if (!pem.hasPublic) {
                    std::cout << "Load or generate a PEM public key first." << std::endl;
                    break;
                }
                const std::string plaintext = readLine("Text to encrypt: ");
                try {
                    const std::vector<uint8_t> encrypted = RSAUtil::encryptTextToBytes(plaintext, pem.keyPair);
                    result = encodeBase64(encrypted);
                    std::cout << "Encryption complete. Base64 ciphertext:\n" << result << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "Encryption failed: " << e.what() << std::endl;
                }
            }
            break;
        }
        case 6: {
            if (mode == Mode::Legacy) {
                if (!legacy.hasKey) {
                    std::cout << "Generate or import legacy keys first." << std::endl;
                    break;
                }
                std::string ciphertextInput = readLine("Enter Base64 ciphertext or comma-separated numbers: ");
                try {
                    const std::vector<long long> encrypted = parseCiphertext(ciphertextInput);
                    const std::string decrypted = RSAUtil::decryptText(encrypted, legacy.keyPair);
                    result = decrypted;
                    std::cout << "Decrypted text: \"" << decrypted << "\"" << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "Decryption failed: " << e.what() << std::endl;
                }
            } else {
                if (!pem.hasPrivate) {
                    std::cout << "Private key not loaded; cannot decrypt." << std::endl;
                    break;
                }
                const std::string ciphertext = readLine("Enter Base64 ciphertext: ");
                try {
                    const std::vector<uint8_t> bytes = decodeBase64(ciphertext);
                    const std::string decrypted = RSAUtil::decryptTextFromBytes(bytes, pem.keyPair);
                    result = decrypted;
                    std::cout << "Decryption complete. Plaintext:\n" << decrypted << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "Decryption failed: " << e.what() << std::endl;
                }
            }
            break;
        }
        case 7: {
            std::string dataToSave = readLine("String to save (leave empty to reuse last result): ");
            if (dataToSave.empty()) {
                if (result.empty()) {
                    std::cout << "Nothing to save." << std::endl;
                    break;
                }
                dataToSave = result;
                std::cout << "Using last result." << std::endl;
            }
            const std::string targetPath = stripSurroundingQuotes(trim(readLine("Target file path: ")));
            try {
                WriteStringToBinaryFile(targetPath, dataToSave);
                std::cout << "Saved to: " << targetPath << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Save failed: " << e.what() << std::endl;
            }
            break;
        }
        case 8: {
            const std::string sourcePath = stripSurroundingQuotes(trim(readLine("Source binary file path: ")));
            try {
                const std::string binaryData = ReadBinaryFileToString(sourcePath);
                if (mode == Mode::Legacy) {
                    if (!legacy.hasKey) {
                        std::cout << "Generate or import legacy keys first." << std::endl;
                        break;
                    }
                    const std::vector<long long> encrypted = RSAUtil::encryptText(binaryData, legacy.keyPair);
                    result = encodeCiphertextBase64(encrypted);
                } else {
                    if (!pem.hasPublic) {
                        std::cout << "Load or generate a PEM public key first." << std::endl;
                        break;
                    }
                    const std::vector<uint8_t> plainBytes(binaryData.begin(), binaryData.end());
                    const std::vector<uint8_t> encrypted = RSAUtil::encryptBytes(plainBytes, pem.keyPair);
                    result = encodeBase64(encrypted);
                }
                std::cout << "Encryption complete. Base64 ciphertext:\n" << result << std::endl;
            } catch (const std::exception& e) {
                std::cout << "File encryption failed: " << e.what() << std::endl;
            }
            break;
        }
        case 9: {
            const std::string ciphertextInput = readLine("Enter Base64 ciphertext (whitespace ignored): ");
            try {
                if (mode == Mode::Legacy) {
                    if (!legacy.hasKey) {
                        std::cout << "Generate or import legacy keys first." << std::endl;
                        break;
                    }
                    const std::vector<long long> encrypted = parseCiphertext(ciphertextInput);
                    const std::string decrypted = RSAUtil::decryptText(encrypted, legacy.keyPair);
                    result = decrypted;
                    std::cout << "Decryption complete." << std::endl;
                } else {
                    if (!pem.hasPrivate) {
                        std::cout << "Private key not loaded; cannot decrypt." << std::endl;
                        break;
                    }
                    const std::vector<uint8_t> cipherBytes = decodeBase64(ciphertextInput);
                    const std::vector<uint8_t> plainBytes = RSAUtil::decryptBytes(cipherBytes, pem.keyPair);
                    result.assign(plainBytes.begin(), plainBytes.end());
                    std::cout << "Decryption complete. Use option 7 to save the data." << std::endl;
                    if (!plainBytes.empty()) {
                        const size_t previewLen = std::min<size_t>(plainBytes.size(), 32);
                        const std::string preview = cppcodec::base64_rfc4648::encode(plainBytes.data(), previewLen);
                        std::cout << "Base64 preview (first " << previewLen << " bytes): " << preview;
                        if (plainBytes.size() > previewLen) {
                            std::cout << "...";
                        }
                        std::cout << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                std::cout << "Decryption failed: " << e.what() << std::endl;
            }
            break;
        }
        case 10: {
            const std::string sourcePath = stripSurroundingQuotes(trim(readLine("Binary file path: ")));
            try {
                result = ReadBinaryFileToString(sourcePath);
                const size_t previewLen = std::min<size_t>(result.size(), 32);
                const std::string preview = cppcodec::base64_rfc4648::encode(
                    reinterpret_cast<const uint8_t*>(result.data()),
                    previewLen);
                std::cout << "Loaded file, " << result.size() << " bytes." << std::endl;
                if (!result.empty()) {
                    std::cout << "Base64 preview (first " << previewLen << " bytes): " << preview;
                    if (result.size() > previewLen) {
                        std::cout << "...";
                    }
                    std::cout << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "Failed to read file: " << e.what() << std::endl;
            }
            break;
        }
        case 11: {
            if (mode == Mode::Legacy && !legacy.hasKey) {
                std::cout << "Generate or import legacy keys first." << std::endl;
                break;
            }
            if (mode == Mode::Pem && !pem.hasPublic) {
                std::cout << "Load or generate a PEM public key first." << std::endl;
                break;
            }
            const std::string sourcePath = stripSurroundingQuotes(trim(readLine("Source file path (plaintext): ")));
            const std::string targetPath = stripSurroundingQuotes(trim(readLine("Target file path (ciphertext output): ")));
            try {
                const std::string binaryData = ReadBinaryFileToString(sourcePath);
                if (mode == Mode::Legacy) {
                    const std::vector<long long> encrypted = RSAUtil::encryptText(binaryData, legacy.keyPair);
                    result = encodeCiphertextBase64(encrypted);
                } else {
                    const std::vector<uint8_t> plainBytes(binaryData.begin(), binaryData.end());
                    const std::vector<uint8_t> encrypted = RSAUtil::encryptBytes(plainBytes, pem.keyPair);
                    result = encodeBase64(encrypted);
                }
                WriteStringToBinaryFile(targetPath, result);
                std::cout << "Encryption complete. Wrote Base64 ciphertext to: " << targetPath << std::endl;
            } catch (const std::exception& e) {
                std::cout << "File encryption failed: " << e.what() << std::endl;
            }
            break;
        }
        case 12: {
            if (mode == Mode::Legacy && !legacy.hasKey) {
                std::cout << "Generate or import legacy keys first." << std::endl;
                break;
            }
            if (mode == Mode::Pem && !pem.hasPrivate) {
                std::cout << "Private key not loaded; cannot decrypt." << std::endl;
                break;
            }
            const std::string cipherPath = stripSurroundingQuotes(trim(readLine("Ciphertext file path: ")));
            const std::string targetPath = stripSurroundingQuotes(trim(readLine("Target file path (plaintext output): ")));
            try {
                const std::string cipherData = ReadBinaryFileToString(cipherPath);
                if (mode == Mode::Legacy) {
                    const std::vector<long long> encrypted = parseCiphertext(cipherData);
                    result = RSAUtil::decryptText(encrypted, legacy.keyPair);
                } else {
                    const std::vector<uint8_t> cipherBytes = decodeBase64(cipherData);
                    const std::vector<uint8_t> plainBytes = RSAUtil::decryptBytes(cipherBytes, pem.keyPair);
                    result.assign(plainBytes.begin(), plainBytes.end());
                }
                WriteStringToBinaryFile(targetPath, result);
                std::cout << "Decryption complete. Wrote plaintext to: " << targetPath << std::endl;
            } catch (const std::exception& e) {
                std::cout << "File decryption failed: " << e.what() << std::endl;
            }
            break;
        }
        case 13: {
            std::cout << "Goodbye!" << std::endl;
            return 0;
        }
        default:
            break;
        }
    }

    return 0;
}
