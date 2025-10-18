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

using std::string;
using std::vector;

namespace {

string encodeCiphertextBase64(const vector<long long>& values) {
    if (values.empty()) {
        return {};
    }
    vector<uint8_t> bytes;
    bytes.reserve(values.size() * sizeof(uint64_t));
    for (long long value : values) {
        uint64_t uvalue = static_cast<uint64_t>(value);
        for (int shift = 0; shift < 64; shift += 8) {
            bytes.push_back(static_cast<uint8_t>((uvalue >> shift) & 0xFF));
        }
    }
    return cppcodec::base64_rfc4648::encode(bytes);
}

vector<long long> decodeCiphertextBase64(const string& encoded) {
    vector<uint8_t> bytes = cppcodec::base64_rfc4648::decode(encoded);
    if (bytes.size() % sizeof(uint64_t) != 0) {
        throw std::invalid_argument("Base64 ciphertext length mismatch");
    }
    vector<long long> values(bytes.size() / sizeof(uint64_t));
    for (size_t i = 0; i < values.size(); ++i) {
        uint64_t value = 0;
        for (int byte = 0; byte < 8; ++byte) {
            value |= static_cast<uint64_t>(bytes[i * 8 + byte]) << (byte * 8);
        }
        values[i] = static_cast<long long>(value);
    }
    return values;
}

string stripWhitespace(const string& input) {
    string cleaned;
    cleaned.reserve(input.size());
    for (unsigned char c : input) {
        if (!std::isspace(c)) {
            cleaned.push_back(static_cast<char>(c));
        }
    }
    return cleaned;
}

bool isNumericCiphertext(const string& input) {
    if (input.empty()) {
        return true;
    }
    return input.find_first_not_of("0123456789,-") == string::npos;
}

string trim(const string& input) {
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    if (begin >= end) {
        return {};
    }
    return string(begin, end);
}

string stripSurroundingQuotes(const string& input) {
    if (input.size() >= 2) {
        const char first = input.front();
        const char last = input.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return input.substr(1, input.size() - 2);
        }
    }
    return input;
}

vector<long long> parseCiphertext(const string& input) {
    const string cleaned = stripWhitespace(input);
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

string encodeBase64(const vector<uint8_t>& data) {
    if (data.empty()) {
        return {};
    }
    return cppcodec::base64_rfc4648::encode(data);
}

vector<uint8_t> decodeBase64(const string& text) {
    string cleaned = stripWhitespace(text);
    try {
        return cppcodec::base64_rfc4648::decode(cleaned);
    } catch (const std::exception&) {
        const size_t mod = cleaned.size() % 4;
        if (mod != 0) {
            cleaned.append(4 - mod, '=');
            try {
                return cppcodec::base64_rfc4648::decode(cleaned);
            } catch (const std::exception&) {
                // fall through
            }
        }
        throw;
    }
}

string readTextFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("Unable to open text file: " + path.string());
    }
    return string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

enum class Mode {
    Legacy = 0,
    Pem = 1
};

constexpr const char* kCliVersion = "RSA_CLI 1.0.0";

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

int readInt(const string& prompt, int minValue, int maxValue) {
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

string readLine(const string& prompt) {
    std::cout << prompt;
    string line;
    std::getline(std::cin, line);
    return line;
}

void printSeparator() {
    std::cout << "----------------------------------------" << std::endl;
}

} // namespace

int main(int argc, char** argv) {
    bool showHelp = false;
    bool showVersion = false;
    bool encryptCommand = false;
    bool decryptCommand = false;
    string commandType = "text";
    string commandInput;
    string commandInputPath;
    string commandPublicKey;
    string commandPublicKeyPath;
    string commandPrivateKey;
    string commandPrivateKeyPath;
    bool generateKeyCommand = false;
    string generatePrivatePath;
    string generatePublicPath;
    int generateKeyBits = 2048;

    auto stripValue = [](string value) {
        return stripSurroundingQuotes(trim(std::move(value)));
    };

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            showHelp = true;
        } else if (arg == "-v" || arg == "--version") {
            showVersion = true;
        } else if (arg == "-encrypt" || arg == "--encrypt" || arg == "-enscrypt") {
            encryptCommand = true;
        } else if (arg == "-decrypt" || arg == "--decrypt" || arg == "-descrypt") {
            decryptCommand = true;
        } else if (arg == "-generate_key" || arg == "--generate_key") {
            generateKeyCommand = true;
        } else if (arg.rfind("-type=", 0) == 0) {
            commandType = stripValue(arg.substr(6));
        } else if (arg == "-type") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after -type\n";
                return 1;
            }
            commandType = stripValue(argv[++i]);
        } else if (arg.rfind("-input=", 0) == 0) {
            commandInput = stripValue(arg.substr(7));
        } else if (arg == "-input") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after -input\n";
                return 1;
            }
            commandInput = stripValue(argv[++i]);
        } else if (arg.rfind("-input_path=", 0) == 0) {
            commandInputPath = stripValue(arg.substr(12));
        } else if (arg == "-input_path") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after -input_path\n";
                return 1;
            }
            commandInputPath = stripValue(argv[++i]);
        } else if (arg.rfind("-public_key=", 0) == 0) {
            commandPublicKey = stripValue(arg.substr(12));
        } else if (arg == "-public_key") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after -public_key\n";
                return 1;
            }
            commandPublicKey = stripValue(argv[++i]);
        } else if (arg.rfind("-private_key=", 0) == 0) {
            commandPrivateKey = stripValue(arg.substr(13));
        } else if (arg == "-private_key") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after -private_key\n";
                return 1;
            }
            commandPrivateKey = stripValue(argv[++i]);
        } else if (!generateKeyCommand && arg.rfind("-private_key_path=", 0) == 0) {
            commandPrivateKeyPath = stripValue(arg.substr(18));
        } else if (!generateKeyCommand && arg == "-private_key_path") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after -private_key_path\n";
                return 1;
            }
            commandPrivateKeyPath = stripValue(argv[++i]);
        } else if (generateKeyCommand && arg.rfind("-private_key_path=", 0) == 0) {
            generatePrivatePath = stripValue(arg.substr(18));
        } else if (generateKeyCommand && arg == "-private_key_path") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after -private_key_path\n";
                return 1;
            }
            generatePrivatePath = stripValue(argv[++i]);
        } else if (!generateKeyCommand && arg.rfind("-public_key_path=", 0) == 0) {
            commandPublicKeyPath = stripValue(arg.substr(17));
        } else if (!generateKeyCommand && arg == "-public_key_path") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after -public_key_path\n";
                return 1;
            }
            commandPublicKeyPath = stripValue(argv[++i]);
        } else if (generateKeyCommand && arg.rfind("-public_key_path=", 0) == 0) {
            generatePublicPath = stripValue(arg.substr(17));
        } else if (generateKeyCommand && arg == "-public_key_path") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after -public_key_path\n";
                return 1;
            }
            generatePublicPath = stripValue(argv[++i]);
        } else if (!generateKeyCommand && arg.rfind("-length=", 0) == 0) {
            const string lenStr = stripValue(arg.substr(8));
            try {
                generateKeyBits = std::stoi(lenStr);
            } catch (...) {
                std::cerr << "Invalid value for -length: " << lenStr << std::endl;
                return 1;
            }
        } else if (!generateKeyCommand && arg == "-length") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after -length\n";
                return 1;
            }
            const string lenStr = stripValue(argv[++i]);
            try {
                generateKeyBits = std::stoi(lenStr);
            } catch (...) {
                std::cerr << "Invalid value for -length: " << lenStr << std::endl;
                return 1;
            }
        } else if (generateKeyCommand && arg.rfind("-length=", 0) == 0) {
            const string lenStr = stripValue(arg.substr(8));
            try {
                generateKeyBits = std::stoi(lenStr);
            } catch (...) {
                std::cerr << "Invalid value for -length: " << lenStr << std::endl;
                return 1;
            }
        } else if (generateKeyCommand && arg == "-length") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after -length\n";
                return 1;
            }
            const string lenStr = stripValue(argv[++i]);
            try {
                generateKeyBits = std::stoi(lenStr);
            } catch (...) {
                std::cerr << "Invalid value for -length: " << lenStr << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return 1;
        }
    }

    if (showHelp) {
        std::cout << "RSA_CLI usage:\n"
                  << "  RSA_CLI                # start interactive menu\n"
                  << "  RSA_CLI -h|--help      # show this help message\n"
                  << "  RSA_CLI -v|--version   # print CLI version\n"
                  << "  RSA_CLI -encrypt -type=text -input=\"...\" -public_key=\"PEM\"\n"
                  << "                        # one-shot text encryption (alias: -enscrypt)\n"
                  << "     (use -input_path and -public_key_path to read from files)\n"
                  << "  RSA_CLI -decrypt -type=text -input=\"Base64\" -private_key=\"PEM\"\n"
                  << "                        # one-shot text decryption (alias: -descrypt)\n"
                  << "     (use -input_path and -private_key_path to read from files)\n"
                  << "  RSA_CLI -generate_key -length=2048 -public_key_path=pub.pem -private_key_path=priv.pem\n"
                  << "                        # generate PEM key pair and write to paths\n"
                  << "     (length <512 will be rounded up automatically)\n\n"
                  << "Interactive menu options:\n"
                  << "  1  Switch mode between legacy (integer) and PEM (OpenSSL)\n"
                  << "  2  Generate keys in current mode\n"
                  << "  3  Import keys from files or input\n"
                  << "  4  Display current keys\n"
                  << "  5  Encrypt text input\n"
                  << "  6  Decrypt text input\n"
                  << "  7  Save a string or result to binary file\n"
                  << "  8  Encrypt binary file (memory -> Base64)\n"
                  << "  9  Decrypt Base64 ciphertext to binary\n"
                  << "  10 Load binary file into memory\n"
                  << "  11 Save current keys to files\n"
                  << "  12 Encrypt file to file\n"
                  << "  13 Decrypt file to file\n"
                  << "  14 Exit\n";
        return 0;
    }

    if (showVersion) {
        std::cout << kCliVersion << std::endl;
        return 0;
    }

    if ((encryptCommand ? 1 : 0) + (decryptCommand ? 1 : 0) + (generateKeyCommand ? 1 : 0) > 1) {
        std::cerr << "Cannot combine encrypt, decrypt, or generate commands simultaneously.\n";
        return 1;
    }

    if (encryptCommand) {
        if (commandType.empty()) {
            commandType = "text";
        }
        if (commandType != "text") {
            std::cerr << "Unsupported encryption type: " << commandType << std::endl;
            return 1;
        }
        string plaintext = commandInput;
        if (plaintext.empty() && !commandInputPath.empty()) {
            try {
                plaintext = readTextFile(std::filesystem::u8path(commandInputPath));
            } catch (const std::exception& ex) {
                std::cerr << "Failed to read input file: " << ex.what() << std::endl;
                return 1;
            }
        }
        if (plaintext.empty()) {
            std::cerr << "Missing -input or -input_path value for encryption.\n";
            return 1;
        }

        string publicKeyPem = commandPublicKey;
        if (publicKeyPem.empty() && !commandPublicKeyPath.empty()) {
            try {
                publicKeyPem = readTextFile(std::filesystem::u8path(commandPublicKeyPath));
            } catch (const std::exception& ex) {
                std::cerr << "Failed to read public key file: " << ex.what() << std::endl;
                return 1;
            }
        }
        if (publicKeyPem.empty()) {
            std::cerr << "Missing -public_key or -public_key_path value for encryption.\n";
            return 1;
        }

        try {
            RSAUtil::PemKeyPair pair{};
            pair.publicKeyPem = publicKeyPem;
            pair.keyBits = RSAUtil::getKeyBitsFromPublicKey(publicKeyPem);
            const vector<uint8_t> encrypted = RSAUtil::encryptTextToBytes(plaintext, pair);
            const string base64 = encodeBase64(encrypted);
            std::cout << base64 << std::endl;
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "Encryption failed: " << ex.what() << std::endl;
            return 1;
        }
    }

    if (decryptCommand) {
        if (commandType.empty()) {
            commandType = "text";
        }
        if (commandType != "text") {
            std::cerr << "Unsupported decryption type: " << commandType << std::endl;
            return 1;
        }
        string ciphertext = commandInput;
        if (ciphertext.empty() && !commandInputPath.empty()) {
            try {
                ciphertext = readTextFile(std::filesystem::u8path(commandInputPath));
            } catch (const std::exception& ex) {
                std::cerr << "Failed to read input file: " << ex.what() << std::endl;
                return 1;
            }
        }
        if (ciphertext.empty()) {
            std::cerr << "Missing -input or -input_path value for decryption.\n";
            return 1;
        }

        string privateKeyPem = commandPrivateKey;
        if (privateKeyPem.empty() && !commandPrivateKeyPath.empty()) {
            try {
                privateKeyPem = readTextFile(std::filesystem::u8path(commandPrivateKeyPath));
            } catch (const std::exception& ex) {
                std::cerr << "Failed to read private key file: " << ex.what() << std::endl;
                return 1;
            }
        }
        if (privateKeyPem.empty()) {
            std::cerr << "Missing -private_key or -private_key_path value for decryption.\n";
            return 1;
        }

        try {
            const vector<uint8_t> cipherBytes = decodeBase64(ciphertext);
            RSAUtil::PemKeyPair pair{};
            pair.privateKeyPem = privateKeyPem;
            const vector<uint8_t> plainBytes = RSAUtil::decryptBytes(cipherBytes, pair);
            string plaintext(plainBytes.begin(), plainBytes.end());
            std::cout << plaintext << std::endl;
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "Decryption failed: " << ex.what() << std::endl;
            return 1;
        }
    }

    if (generateKeyCommand) {
        if (generatePublicPath.empty() && generatePrivatePath.empty()) {
            std::cerr << "Provide at least -public_key_path or -private_key_path to save generated keys.\n";
            return 1;
        }
        if (generateKeyBits < 512) {
            std::cout << "Requested key length below 512 bits; using 512.\n";
            generateKeyBits = 512;
        }
        try {
            const RSAUtil::PemKeyPair pair = RSAUtil::generatePemKeyPair(generateKeyBits);
            const string sanitizedPublic = stripSurroundingQuotes(generatePublicPath);
            const string sanitizedPrivate = stripSurroundingQuotes(generatePrivatePath);
            if (!generatePublicPath.empty()) {
                WriteStringToBinaryFile(sanitizedPublic, pair.publicKeyPem);
            }
            if (!generatePrivatePath.empty()) {
                WriteStringToBinaryFile(sanitizedPrivate, pair.privateKeyPem);
            }
            std::cout << "Generated " << generateKeyBits << "-bit key pair.\n";
            if (!generatePublicPath.empty()) {
                std::cout << "Public key saved to: " << sanitizedPublic << std::endl;
            }
            if (!generatePrivatePath.empty()) {
                std::cout << "Private key saved to: " << sanitizedPrivate << std::endl;
            }
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "Key generation failed: " << ex.what() << std::endl;
            return 1;
        }
    }

    std::cout << "RSA Encryption/Decryption CLI Tool" << std::endl;
    std::cout << "===================================" << std::endl;

    LegacyState legacy;
    PemState pem;
    Mode mode = Mode::Pem;
    string result;

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
        std::cout << "11. Save current keys to files" << std::endl;
        std::cout << "12. Encrypt file to file" << std::endl;
        std::cout << "13. Decrypt file to file" << std::endl;
        std::cout << "14. Exit" << std::endl;

        const int choice = readInt("Choose an option (1-14): ", 1, 14);

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
                string bitsInput = stripWhitespace(readLine("Enter key size in bits (>=512, default " + std::to_string(pem.keyBits) + "): "));
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
                    string publicPath = trim(readLine("Enter public key PEM path: "));
                    if (publicPath.empty()) {
                        throw std::runtime_error("Public key path may not be empty");
                    }
                    const string publicPem = readTextFile(publicPath);
                    string privatePath = trim(readLine("Enter private key PEM path (optional, needed for decrypt): "));
                    string privatePem;
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
                const string plaintext = readLine("Text to encrypt: ");
                try {
                    vector<long long> encrypted = RSAUtil::encryptText(plaintext, legacy.keyPair);
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
                const string plaintext = readLine("Text to encrypt: ");
                try {
                    const vector<uint8_t> encrypted = RSAUtil::encryptTextToBytes(plaintext, pem.keyPair);
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
                string ciphertextInput = readLine("Enter Base64 ciphertext or comma-separated numbers: ");
                try {
                    const vector<long long> encrypted = parseCiphertext(ciphertextInput);
                    const string decrypted = RSAUtil::decryptText(encrypted, legacy.keyPair);
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
                const string ciphertext = readLine("Enter Base64 ciphertext: ");
                try {
                    const vector<uint8_t> bytes = decodeBase64(ciphertext);
                    const string decrypted = RSAUtil::decryptTextFromBytes(bytes, pem.keyPair);
                    result = decrypted;
                    std::cout << "Decryption complete. Plaintext:\n" << decrypted << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "Decryption failed: " << e.what() << std::endl;
                }
            }
            break;
        }
        case 7: {
            string dataToSave = readLine("String to save (leave empty to reuse last result): ");
            if (dataToSave.empty()) {
                if (result.empty()) {
                    std::cout << "Nothing to save." << std::endl;
                    break;
                }
                dataToSave = result;
                std::cout << "Using last result." << std::endl;
            }
            const string targetPath = stripSurroundingQuotes(trim(readLine("Target file path: ")));
            try {
                WriteStringToBinaryFile(targetPath, dataToSave);
                std::cout << "Saved to: " << targetPath << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Save failed: " << e.what() << std::endl;
            }
            break;
        }
        case 8: {
            const string sourcePath = stripSurroundingQuotes(trim(readLine("Source binary file path: ")));
            try {
                const string binaryData = ReadBinaryFileToString(sourcePath);
                if (mode == Mode::Legacy) {
                    if (!legacy.hasKey) {
                        std::cout << "Generate or import legacy keys first." << std::endl;
                        break;
                    }
                    const vector<long long> encrypted = RSAUtil::encryptText(binaryData, legacy.keyPair);
                    result = encodeCiphertextBase64(encrypted);
                } else {
                    if (!pem.hasPublic) {
                        std::cout << "Load or generate a PEM public key first." << std::endl;
                        break;
                    }
                    const vector<uint8_t> plainBytes(binaryData.begin(), binaryData.end());
                    const vector<uint8_t> encrypted = RSAUtil::encryptBytes(plainBytes, pem.keyPair);
                    result = encodeBase64(encrypted);
                }
                std::cout << "Encryption complete. Base64 ciphertext:\n" << result << std::endl;
            } catch (const std::exception& e) {
                std::cout << "File encryption failed: " << e.what() << std::endl;
            }
            break;
        }
        case 9: {
            const string ciphertextInput = readLine("Enter Base64 ciphertext (whitespace ignored): ");
            try {
                if (mode == Mode::Legacy) {
                    if (!legacy.hasKey) {
                        std::cout << "Generate or import legacy keys first." << std::endl;
                        break;
                    }
                    const vector<long long> encrypted = parseCiphertext(ciphertextInput);
                    const string decrypted = RSAUtil::decryptText(encrypted, legacy.keyPair);
                    result = decrypted;
                    std::cout << "Decryption complete." << std::endl;
                } else {
                    if (!pem.hasPrivate) {
                        std::cout << "Private key not loaded; cannot decrypt." << std::endl;
                        break;
                    }
                    const vector<uint8_t> cipherBytes = decodeBase64(ciphertextInput);
                    const vector<uint8_t> plainBytes = RSAUtil::decryptBytes(cipherBytes, pem.keyPair);
                    result.assign(plainBytes.begin(), plainBytes.end());
                    std::cout << "Decryption complete. Use option 7 to save the data." << std::endl;
                    if (!plainBytes.empty()) {
                        const size_t previewLen = std::min<size_t>(plainBytes.size(), 32);
                        const string preview = cppcodec::base64_rfc4648::encode(plainBytes.data(), previewLen);
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
            const string sourcePath = stripSurroundingQuotes(trim(readLine("Binary file path: ")));
            try {
                result = ReadBinaryFileToString(sourcePath);
                const size_t previewLen = std::min<size_t>(result.size(), 32);
                const string preview = cppcodec::base64_rfc4648::encode(
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
            if (mode == Mode::Legacy) {
                if (!legacy.hasKey) {
                    std::cout << "Generate or import legacy keys first." << std::endl;
                    break;
                }
                const string publicPath = stripSurroundingQuotes(trim(readLine("Path to save public exponent (e) [leave blank to skip]: ")));
                const string privatePath = stripSurroundingQuotes(trim(readLine("Path to save private exponent (d) [leave blank to skip]: ")));
                const string modulusPath = stripSurroundingQuotes(trim(readLine("Path to save modulus (n) [leave blank to skip]: ")));
                bool savedAny = false;
                try {
                    if (!publicPath.empty()) {
                        WriteStringToBinaryFile(publicPath, legacy.keyPair.publicKey);
                        std::cout << "Saved public exponent to: " << publicPath << std::endl;
                        savedAny = true;
                    }
                    if (!privatePath.empty()) {
                        WriteStringToBinaryFile(privatePath, legacy.keyPair.privateKey);
                        std::cout << "Saved private exponent to: " << privatePath << std::endl;
                        savedAny = true;
                    }
                    if (!modulusPath.empty()) {
                        WriteStringToBinaryFile(modulusPath, legacy.keyPair.modulus);
                        std::cout << "Saved modulus to: " << modulusPath << std::endl;
                        savedAny = true;
                    }
                    if (!savedAny) {
                        std::cout << "No files specified; nothing saved." << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cout << "Saving legacy keys failed: " << e.what() << std::endl;
                }
            } else {
                if (!pem.hasPublic && !pem.hasPrivate) {
                    std::cout << "No PEM keys loaded. Generate or import keys first." << std::endl;
                    break;
                }
                const string publicPath = stripSurroundingQuotes(trim(readLine("Path to save public key PEM [leave blank to skip]: ")));
                const string privatePath = stripSurroundingQuotes(trim(readLine("Path to save private key PEM [leave blank to skip]: ")));
                bool savedAny = false;
                try {
                    if (!publicPath.empty()) {
                        if (!pem.hasPublic) {
                            std::cout << "Public key not loaded; skipping save." << std::endl;
                        } else {
                            WriteStringToBinaryFile(publicPath, pem.keyPair.publicKeyPem);
                            std::cout << "Saved public key PEM to: " << publicPath << std::endl;
                            savedAny = true;
                        }
                    }
                    if (!privatePath.empty()) {
                        if (!pem.hasPrivate) {
                            std::cout << "Private key not loaded; skipping save." << std::endl;
                        } else {
                            WriteStringToBinaryFile(privatePath, pem.keyPair.privateKeyPem);
                            std::cout << "Saved private key PEM to: " << privatePath << std::endl;
                            savedAny = true;
                        }
                    }
                    if (!savedAny) {
                        std::cout << "No files specified; nothing saved." << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cout << "Saving PEM keys failed: " << e.what() << std::endl;
                }
            }
            break;
        }
        case 12: {
            if (mode == Mode::Legacy && !legacy.hasKey) {
                std::cout << "Generate or import legacy keys first." << std::endl;
                break;
            }
            if (mode == Mode::Pem && !pem.hasPublic) {
                std::cout << "Load or generate a PEM public key first." << std::endl;
                break;
            }
            const string sourcePath = stripSurroundingQuotes(trim(readLine("Source file path (plaintext): ")));
            const string targetPath = stripSurroundingQuotes(trim(readLine("Target file path (ciphertext output): ")));
            try {
                const string binaryData = ReadBinaryFileToString(sourcePath);
                if (mode == Mode::Legacy) {
                    const vector<long long> encrypted = RSAUtil::encryptText(binaryData, legacy.keyPair);
                    result = encodeCiphertextBase64(encrypted);
                } else {
                    const vector<uint8_t> plainBytes(binaryData.begin(), binaryData.end());
                    const vector<uint8_t> encrypted = RSAUtil::encryptBytes(plainBytes, pem.keyPair);
                    result = encodeBase64(encrypted);
                }
                WriteStringToBinaryFile(targetPath, result);
                std::cout << "Encryption complete. Wrote Base64 ciphertext to: " << targetPath << std::endl;
            } catch (const std::exception& e) {
                std::cout << "File encryption failed: " << e.what() << std::endl;
            }
            break;
        }
        case 13: {
            if (mode == Mode::Legacy && !legacy.hasKey) {
                std::cout << "Generate or import legacy keys first." << std::endl;
                break;
            }
            if (mode == Mode::Pem && !pem.hasPrivate) {
                std::cout << "Private key not loaded; cannot decrypt." << std::endl;
                break;
            }
            const string cipherPath = stripSurroundingQuotes(trim(readLine("Ciphertext file path: ")));
            const string targetPath = stripSurroundingQuotes(trim(readLine("Target file path (plaintext output): ")));
            try {
                const string cipherData = ReadBinaryFileToString(cipherPath);
                if (mode == Mode::Legacy) {
                    const vector<long long> encrypted = parseCiphertext(cipherData);
                    result = RSAUtil::decryptText(encrypted, legacy.keyPair);
                } else {
                    const vector<uint8_t> cipherBytes = decodeBase64(cipherData);
                    const vector<uint8_t> plainBytes = RSAUtil::decryptBytes(cipherBytes, pem.keyPair);
                    result.assign(plainBytes.begin(), plainBytes.end());
                }
                WriteStringToBinaryFile(targetPath, result);
                std::cout << "Decryption complete. Wrote plaintext to: " << targetPath << std::endl;
            } catch (const std::exception& e) {
                std::cout << "File decryption failed: " << e.what() << std::endl;
            }
            break;
        }
        case 14: {
            std::cout << "Goodbye!" << std::endl;
            return 0;
        }
        default:
            break;
        }
    }

    return 0;
}
