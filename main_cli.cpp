#include "RSA.hpp"
#include "bin.hpp"
#include "third_party/cppcodec/cppcodec/base64_rfc4648.hpp"
#include <cctype>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::string encodeCiphertextBase64(const std::vector<long long>& values) {
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

std::vector<long long> parseCiphertext(const std::string& input) {
    const std::string cleaned = stripWhitespace(input);
    if (cleaned.empty()) {
        return {};
    }
    try {
        return decodeCiphertextBase64(cleaned);
    } catch (const std::exception&) {
        if (isNumericCiphertext(cleaned)) {
            return RSA::stringToCiphertext(cleaned);
        }
        throw;
    }
}
} // namespace

int main() {
    std::cout << "RSA Encryption/Decryption CLI Tool" << std::endl;
    std::cout << "===================================" << std::endl;

    RSA::KeyPair keyPair;
    bool hasKeyPair = false;
    std::string result;

    while (true) {
        std::cout << "\nOptions:" << std::endl;
        std::cout << "1. Generate new key pair" << std::endl;
        std::cout << "2. Input existing key pair" << std::endl;
        std::cout << "3. Show current key pair" << std::endl;
        std::cout << "4. Encrypt text" << std::endl;
        std::cout << "5. Decrypt text" << std::endl;
        std::cout << "6. Save string to binary file" << std::endl;
        std::cout << "7. Encrypt binary file" << std::endl;
        std::cout << "8. Decrypt Base64 ciphertext to binary string" << std::endl;
        std::cout << "9. Load binary file into string" << std::endl;
        std::cout << "10. Exit" << std::endl;
        std::cout << "Choose an option (1-10): ";

        int choice;
        std::cin >> choice;
        std::cin.ignore(); // Clear newline character

        switch (choice) {
        case 1: {
            std::cout << "Generating new key pair..." << std::endl;
            keyPair = RSA::generateKeyPair();
            hasKeyPair = true;
            RSA::printKeyInfo(keyPair);
            break;
        }
        case 2: {
            if (!hasKeyPair) {
                keyPair = RSA::KeyPair();
            }
            
            std::cout << "Enter public key (e): ";
            std::getline(std::cin, keyPair.publicKey);
            
            std::cout << "Enter private key (d): ";
            std::getline(std::cin, keyPair.privateKey);
            
            std::cout << "Enter modulus (n): ";
            std::getline(std::cin, keyPair.modulus);
            
            std::cin.ignore(); // 清除输入缓冲区中的换行符
            hasKeyPair = true;
            std::cout << "Key pair has been set." << std::endl;
            break;
        }
        case 3: {
            if (hasKeyPair) {
                RSA::printKeyInfo(keyPair);
            } else {
                std::cout << "No key pair has been generated or entered yet." << std::endl;
            }
            break;
        }
        case 4: {
            if (!hasKeyPair) {
                std::cout << "Please generate or input a key pair first." << std::endl;
                break;
            }
            
            std::cout << "Enter text to encrypt: ";
            std::string plaintext;
            std::getline(std::cin, plaintext);
            
            std::cout << "Text to encrypt: \"" << plaintext << "\" (length: " << plaintext.length() << ")" << std::endl;
            
            try {
                std::vector<long long> encrypted = RSA::encryptText(plaintext, keyPair);
                std::cout << "Encrypted numbers: ";
                for (size_t i = 0; i < encrypted.size(); ++i) {
                    std::cout << encrypted[i];
                    if (i < encrypted.size() - 1) std::cout << ", ";
                }
                std::cout << std::endl;
                
                result = encodeCiphertextBase64(encrypted);
                std::cout << "Encrypted Base64: " << result << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Encryption failed: " << e.what() << std::endl;
            }
            break;
        }
        case 5: {
            if (!hasKeyPair) {
                std::cout << "Please generate or input a key pair first." << std::endl;
                break;
            }
            
            std::cout << "Enter Base64 ciphertext (or comma-separated numbers): ";
            std::string ciphertextInput;
            std::getline(std::cin, ciphertextInput);
            
            std::cout << "Text to decrypt: \"" << ciphertextInput << "\"" << std::endl;
            
            try {
                std::vector<long long> encrypted = parseCiphertext(ciphertextInput);
                std::cout << "Numbers to decrypt: ";
                for (size_t i = 0; i < encrypted.size(); ++i) {
                    std::cout << encrypted[i];
                    if (i < encrypted.size() - 1) std::cout << ", ";
                }
                std::cout << std::endl;
                
                std::string decrypted = RSA::decryptText(encrypted, keyPair);
                std::cout << "Decrypted text: \"" << decrypted << "\"" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Decryption failed: " << e.what() << std::endl;
            }
            break;
        }
        case 6: {
            std::cout << "Enter string to save (leave empty to use the last encryption result): ";
            std::string dataToSave;
            std::getline(std::cin, dataToSave);

            if (dataToSave.empty()) {
                if (result.empty()) {
                    std::cout << "No previous encryption result available. Please enter a string." << std::endl;
                    break;
                }
                dataToSave = result;
                std::cout << "Using last encryption result." << std::endl;
            }

            std::cout << "Enter target file path: ";
            std::string targetPath;
            std::getline(std::cin, targetPath);

            try {
                WriteStringToBinaryFile(targetPath, dataToSave);
                std::cout << "String saved to file: \"" << targetPath << "\"" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Failed to save string: " << e.what() << std::endl;
            }
            break;
        }
        case 7: {
            if (!hasKeyPair) {
                std::cout << "Please generate or input a key pair first." << std::endl;
                break;
            }

            std::cout << "Enter source binary file path: ";
            std::string sourcePath;
            std::getline(std::cin, sourcePath);

            try {
                std::string binaryData = ReadBinaryFileToString(sourcePath);
                std::vector<long long> encrypted = RSA::encryptText(binaryData, keyPair);
                result = encodeCiphertextBase64(encrypted);
                std::cout << "Encrypted Base64 (use option 6 to save if needed):" << std::endl;
                std::cout << result << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Binary encryption failed: " << e.what() << std::endl;
            }
            break;
        }
        case 8: {
            if (!hasKeyPair) {
                std::cout << "Please generate or input a key pair first." << std::endl;
                break;
            }

            std::cout << "Enter Base64 ciphertext (or comma-separated numbers): ";
            std::string ciphertextInput;
            std::getline(std::cin, ciphertextInput);

            try {
                std::vector<long long> encrypted = parseCiphertext(ciphertextInput);
                std::string decrypted = RSA::decryptText(encrypted, keyPair);
                result = decrypted;
                const std::string base64Preview = cppcodec::base64_rfc4648::encode(
                    reinterpret_cast<const uint8_t*>(decrypted.data()),
                    decrypted.size()
                );
                std::cout << "Decrypted binary data stored in memory (use option 6 to save)." << std::endl;
                if (!base64Preview.empty()) {
                    std::cout << "Base64 preview: " << base64Preview << std::endl;
                } else {
                    std::cout << "Decrypted data is empty." << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "Binary decryption failed: " << e.what() << std::endl;
            }
            break;
        }
        case 9: {
            std::cout << "Enter binary file path: ";
            std::string sourcePath;
            std::getline(std::cin, sourcePath);

            try {
                std::string binaryData = ReadBinaryFileToString(sourcePath);
                result = binaryData;
                const std::string base64Preview = cppcodec::base64_rfc4648::encode(
                    reinterpret_cast<const uint8_t*>(binaryData.data()),
                    binaryData.size()
                );
                std::cout << "Binary file loaded into memory (use option 6 to save elsewhere)." << std::endl;
                if (!base64Preview.empty()) {
                    std::cout << "Base64 preview: " << base64Preview << std::endl;
                } else {
                    std::cout << "Loaded data is empty." << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "Failed to load binary file: " << e.what() << std::endl;
            }
            break;
        }
        case 10: {
            std::cout << "Exiting program. Goodbye!" << std::endl;
            return 0;
        }
        default: {
            std::cout << "Invalid option. Please choose a number between 1 and 10." << std::endl;
            break;
        }
        }
    }

    return 0;
}
