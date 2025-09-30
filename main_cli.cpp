#include "RSA.hpp"
#include <iostream>
#include <string>

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
        std::cout << "6. Exit" << std::endl;
        std::cout << "Choose an option (1-6): ";

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
            std::cin >> keyPair.publicKey;
            
            std::cout << "Enter private key (d): ";
            std::cin >> keyPair.privateKey;
            
            std::cout << "Enter modulus (n): ";
            std::cin >> keyPair.modulus;
            
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
                
                result = RSA::ciphertextToString(encrypted);
                std::cout << "Encrypted text: " << result << std::endl;
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
            
            std::cout << "Enter text to decrypt: ";
            std::string ciphertextInput;
            std::getline(std::cin, ciphertextInput);
            
            std::cout << "Text to decrypt: \"" << ciphertextInput << "\"" << std::endl;
            
            try {
                std::vector<long long> encrypted = RSA::stringToCiphertext(ciphertextInput);
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
            std::cout << "Exiting program. Goodbye!" << std::endl;
            return 0;
        }
        default: {
            std::cout << "Invalid option. Please choose a number between 1 and 6." << std::endl;
            break;
        }
        }
    }

    return 0;
}