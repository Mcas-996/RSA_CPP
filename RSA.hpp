#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <limits>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

// RSA helpers implemented on top of OpenSSL while keeping the original interfaces.
namespace RSAUtil {
    
    struct KeyPair {
        std::string publicKey;    // Public exponent e
        std::string privateKey;   // Private exponent d
        std::string modulus;      // Modulus n
    };

    struct PemKeyPair {
        std::string publicKeyPem;   // PEM encoded public key
        std::string privateKeyPem;  // PEM encoded private key
        int keyBits;                // Key size
    };
    
    namespace detail {
        struct BNDeleter {
            void operator()(BIGNUM* bn) const noexcept {
                BN_free(bn);
            }
        };
        
        struct BNCTXDeleter {
            void operator()(BN_CTX* ctx) const noexcept {
                BN_CTX_free(ctx);
            }
        };
        
        struct RSADeleter {
            void operator()(::RSA* rsa) const noexcept {
                RSA_free(rsa);
            }
        };
        
        struct OpenSSLStringDeleter {
            void operator()(char* ptr) const noexcept {
                OPENSSL_free(ptr);
            }
        };
        
        struct BIODeleter {
            void operator()(BIO* bio) const noexcept {
                BIO_free(bio);
            }
        };
        
        using UniqueBN = std::unique_ptr<BIGNUM, BNDeleter>;
        using UniqueBNCTX = std::unique_ptr<BN_CTX, BNCTXDeleter>;
        using UniqueRSA = std::unique_ptr<::RSA, RSADeleter>;
        using UniqueOSSString = std::unique_ptr<char, OpenSSLStringDeleter>;
        using UniqueBIO = std::unique_ptr<BIO, BIODeleter>;
        
        [[noreturn]] void throwOpenSSLError(const std::string& message) {
            unsigned long errCode = ERR_get_error();
            char buf[256] = {};
            if (errCode != 0) {
                ERR_error_string_n(errCode, buf, sizeof(buf));
            }
            throw std::runtime_error(message + (errCode != 0 ? (": " + std::string(buf)) : ""));
        }
        
        UniqueBN makeBNFromDec(const std::string& decimal) {
            BIGNUM* raw = nullptr;
            if (BN_dec2bn(&raw, decimal.c_str()) == 0 || raw == nullptr) {
                throw std::runtime_error("failed to convert decimal string to BIGNUM");
            }
            return UniqueBN(raw);
        }
        
        UniqueBN makeBNFromWord(unsigned long value) {
            UniqueBN bn(BN_new());
            if (!bn || BN_set_word(bn.get(), value) != 1) {
                throwOpenSSLError("failed to create BIGNUM");
            }
            return bn;
        }
        
        long long bnToLongLong(const BIGNUM* bn) {
            if (!bn) {
                throw std::runtime_error("BIGNUM is null");
            }
            if (BN_num_bits(bn) > 63) {
                throw std::runtime_error("value exceeds long long range");
            }
#if defined(_MSC_VER) && (_MSC_VER < 1900)
            // BN_get_word returns unsigned long on older MSVC versions
            return static_cast<long long>(BN_get_word(const_cast<BIGNUM*>(bn)));
#else
            return static_cast<long long>(BN_get_word(bn));
#endif
        }
        
        std::string bioToString(BIO* bio) {
            BUF_MEM* mem = nullptr;
            BIO_get_mem_ptr(bio, &mem);
            if (!mem || !mem->data || mem->length <= 0) {
                throw std::runtime_error("failed to extract data from BIO");
            }
            return std::string(mem->data, static_cast<size_t>(mem->length));
        }
        
        UniqueBIO makeBioFromString(const std::string& data) {
            BIO* bio = BIO_new_mem_buf(data.data(), static_cast<int>(data.size()));
            if (!bio) {
                throwOpenSSLError("failed to create memory BIO");
            }
            return UniqueBIO(bio);
        }
        
        UniqueRSA loadPublicKey(const std::string& publicKeyPem) {
            UniqueBIO bio(makeBioFromString(publicKeyPem));
            RSA* rsa = PEM_read_bio_RSA_PUBKEY(bio.get(), nullptr, nullptr, nullptr);
            if (!rsa) {
                throwOpenSSLError("failed to load public key");
            }
            return UniqueRSA(rsa);
        }
        
        UniqueRSA loadPrivateKey(const std::string& privateKeyPem) {
            UniqueBIO bio(makeBioFromString(privateKeyPem));
            RSA* rsa = PEM_read_bio_RSAPrivateKey(bio.get(), nullptr, nullptr, nullptr);
            if (!rsa) {
                throwOpenSSLError("failed to load private key");
            }
            return UniqueRSA(rsa);
        }
        
        int maxChunkSizeForPadding(int rsaSize, int padding) {
            switch (padding) {
                case RSA_PKCS1_PADDING:
                    return rsaSize - 11;
                case RSA_PKCS1_OAEP_PADDING:
                    return rsaSize - 42; // default SHA-1 OAEP
                case RSA_PKCS1_PSS_PADDING:
                    throw std::invalid_argument("PSS padding is not supported with RSA_public_encrypt");
                case RSA_NO_PADDING:
                    return rsaSize;
                default:
                    throw std::invalid_argument("unsupported RSA padding mode");
            }
        }
    } // namespace detail
    
    inline void ensureOpenSSLInit() {
#if OPENSSL_VERSION_NUMBER < 0x30000000L
        static bool initialized = [] {
            ERR_load_crypto_strings();
            return true;
        }();
        (void)initialized;
#endif
    }
    
    inline KeyPair generateKeyPair() {
        ensureOpenSSLInit();
        
        constexpr int kMaxAttempts = 32;
        constexpr int kPrimeBits = 30; // ensures modulus fits into signed 64-bit range
        
        detail::UniqueBNCTX ctx(BN_CTX_new());
        if (!ctx) {
            detail::throwOpenSSLError("failed to create BN_CTX");
        }
        
        for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
            detail::UniqueBN p(BN_new());
            detail::UniqueBN q(BN_new());
            detail::UniqueBN n(BN_new());
            detail::UniqueBN phi(BN_new());
            detail::UniqueBN pMinus(BN_new());
            detail::UniqueBN qMinus(BN_new());
            detail::UniqueBN gcd(BN_new());
            
            if (!p || !q || !n || !phi || !pMinus || !qMinus || !gcd) {
                detail::throwOpenSSLError("failed to allocate BIGNUM");
            }
            
            if (BN_generate_prime_ex(p.get(), kPrimeBits, 0, nullptr, nullptr, nullptr) != 1) {
                detail::throwOpenSSLError("failed to generate prime p");
            }
            if (BN_generate_prime_ex(q.get(), kPrimeBits, 0, nullptr, nullptr, nullptr) != 1) {
                detail::throwOpenSSLError("failed to generate prime q");
            }
            if (BN_cmp(p.get(), q.get()) == 0) {
                continue;
            }
            
            if (BN_mul(n.get(), p.get(), q.get(), ctx.get()) != 1) {
                detail::throwOpenSSLError("failed to compute modulus");
            }
            if (BN_num_bits(n.get()) > 63) {
                continue;
            }
            
            if (!BN_copy(pMinus.get(), p.get()) || BN_sub_word(pMinus.get(), 1) != 1) {
                detail::throwOpenSSLError("failed to compute p-1");
            }
            if (!BN_copy(qMinus.get(), q.get()) || BN_sub_word(qMinus.get(), 1) != 1) {
                detail::throwOpenSSLError("failed to compute q-1");
            }
            if (BN_mul(phi.get(), pMinus.get(), qMinus.get(), ctx.get()) != 1) {
                detail::throwOpenSSLError("failed to compute phi");
            }
            
            detail::UniqueBN e(detail::makeBNFromWord(65537));
            if (BN_gcd(gcd.get(), e.get(), phi.get(), ctx.get()) != 1) {
                detail::throwOpenSSLError("failed to compute gcd for exponent");
            }
            if (!BN_is_one(gcd.get())) {
                static const unsigned long kFallbackExponents[] = {3UL, 5UL, 17UL, 257UL};
                bool found = false;
                for (unsigned long cand : kFallbackExponents) {
                    if (BN_set_word(e.get(), cand) != 1) {
                        detail::throwOpenSSLError("failed to set fallback exponent");
                    }
                    if (BN_gcd(gcd.get(), e.get(), phi.get(), ctx.get()) != 1) {
                        detail::throwOpenSSLError("failed to compute gcd for fallback exponent");
                    }
                    if (BN_is_one(gcd.get())) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    continue;
                }
            }
            
            BIGNUM* rawD = BN_mod_inverse(nullptr, e.get(), phi.get(), ctx.get());
            if (!rawD) {
                detail::throwOpenSSLError("failed to compute private exponent");
            }
            detail::UniqueBN d(rawD);
            
            if (BN_num_bits(d.get()) > 63 || BN_num_bits(e.get()) > 32) {
                continue;
            }
            
            detail::UniqueOSSString nStr(BN_bn2dec(n.get()));
            detail::UniqueOSSString dStr(BN_bn2dec(d.get()));
            detail::UniqueOSSString eStr(BN_bn2dec(e.get()));
            
            if (!nStr || !dStr || !eStr) {
                detail::throwOpenSSLError("failed to convert BIGNUM to string");
            }
            
            return {std::string(eStr.get()), std::string(dStr.get()), std::string(nStr.get())};
        }
        
        throw std::runtime_error("unable to generate legacy-compatible RSA key pair");
    }
    
    inline PemKeyPair generatePemKeyPair(int keyBits = 2048) {
        ensureOpenSSLInit();
        
        if (keyBits < 512) {
            throw std::invalid_argument("RSA key size must be at least 512 bits");
        }
        
        detail::UniqueBN exponent(detail::makeBNFromWord(RSA_F4));
        detail::UniqueRSA rsa(RSA_new());
        if (!rsa) {
            detail::throwOpenSSLError("failed to allocate RSA structure");
        }
        
        if (RSA_generate_key_ex(rsa.get(), keyBits, exponent.get(), nullptr) != 1) {
            detail::throwOpenSSLError("RSA key generation failed");
        }
        
        detail::UniqueBIO privateBio(BIO_new(BIO_s_mem()));
        detail::UniqueBIO publicBio(BIO_new(BIO_s_mem()));
        if (!privateBio || !publicBio) {
            detail::throwOpenSSLError("failed to allocate BIO");
        }
        
        if (PEM_write_bio_RSAPrivateKey(privateBio.get(), rsa.get(), nullptr, nullptr, 0, nullptr, nullptr) != 1) {
            detail::throwOpenSSLError("failed to write private key PEM");
        }
        
        if (PEM_write_bio_RSA_PUBKEY(publicBio.get(), rsa.get()) != 1) {
            detail::throwOpenSSLError("failed to write public key PEM");
        }
        
        return {detail::bioToString(publicBio.get()), detail::bioToString(privateBio.get()), keyBits};
    }
    
    inline int getKeyBitsFromPublicKey(const std::string& publicKeyPem) {
        detail::UniqueRSA rsa(detail::loadPublicKey(publicKeyPem));
        const int rsaSize = RSA_size(rsa.get());
        if (rsaSize <= 0) {
            throw std::runtime_error("invalid RSA key size");
        }
        return rsaSize * 8;
    }
    
    inline long long encryptNumber(long long message, const std::string& publicKey, const std::string& modulus) {
        ensureOpenSSLInit();
        
        if (message < 0) {
            throw std::invalid_argument("message must be non-negative");
        }
        
        detail::UniqueBN modulusBN(detail::makeBNFromDec(modulus));
        detail::UniqueBN exponentBN(detail::makeBNFromDec(publicKey));
        detail::UniqueBN messageBN(BN_new());
        detail::UniqueBN resultBN(BN_new());
        detail::UniqueBNCTX ctx(BN_CTX_new());
        
        if (!messageBN || !resultBN || !ctx) {
            detail::throwOpenSSLError("failed to initialise BIGNUM objects");
        }
        
        if (BN_set_word(messageBN.get(), static_cast<unsigned long>(message)) != 1) {
            detail::throwOpenSSLError("failed to set message BIGNUM");
        }
        
        if (BN_cmp(messageBN.get(), modulusBN.get()) >= 0) {
            throw std::runtime_error("message must be smaller than modulus");
        }
        
        if (BN_mod_exp(resultBN.get(), messageBN.get(), exponentBN.get(), modulusBN.get(), ctx.get()) != 1) {
            detail::throwOpenSSLError("RSA encryption failed");
        }
        
        return detail::bnToLongLong(resultBN.get());
    }
    
    inline long long decryptNumber(long long ciphertext, const std::string& privateKey, const std::string& modulus) {
        ensureOpenSSLInit();
        
        if (ciphertext < 0) {
            throw std::invalid_argument("ciphertext must be non-negative");
        }
        
        detail::UniqueBN modulusBN(detail::makeBNFromDec(modulus));
        detail::UniqueBN exponentBN(detail::makeBNFromDec(privateKey));
        detail::UniqueBN cipherBN(BN_new());
        detail::UniqueBN resultBN(BN_new());
        detail::UniqueBNCTX ctx(BN_CTX_new());
        
        if (!cipherBN || !resultBN || !ctx) {
            detail::throwOpenSSLError("failed to initialise BIGNUM objects");
        }
        
        if (BN_set_word(cipherBN.get(), static_cast<unsigned long>(ciphertext)) != 1) {
            detail::throwOpenSSLError("failed to set ciphertext BIGNUM");
        }
        
        if (BN_cmp(cipherBN.get(), modulusBN.get()) >= 0) {
            throw std::runtime_error("ciphertext must be smaller than modulus");
        }
        
        if (BN_mod_exp(resultBN.get(), cipherBN.get(), exponentBN.get(), modulusBN.get(), ctx.get()) != 1) {
            detail::throwOpenSSLError("RSA decryption failed");
        }
        
        return detail::bnToLongLong(resultBN.get());
    }
    
    inline std::vector<uint8_t> encryptBytes(const std::vector<uint8_t>& plaintext,
                                            const PemKeyPair& keyPair,
                                            int padding = RSA_PKCS1_OAEP_PADDING) {
        ensureOpenSSLInit();
        
        detail::UniqueRSA rsa(detail::loadPublicKey(keyPair.publicKeyPem));
        const int rsaSize = RSA_size(rsa.get());
        if (rsaSize <= 0) {
            throw std::runtime_error("invalid RSA key size");
        }
        
        const int maxChunk = detail::maxChunkSizeForPadding(rsaSize, padding);
        if (maxChunk <= 0) {
            throw std::invalid_argument("padding configuration results in non-positive chunk size");
        }
        
        std::vector<uint8_t> encrypted;
        if (!plaintext.empty()) {
            encrypted.reserve(((plaintext.size() + static_cast<size_t>(maxChunk) - 1) / static_cast<size_t>(maxChunk)) * static_cast<size_t>(rsaSize));
        }
        std::vector<uint8_t> buffer(static_cast<size_t>(rsaSize));
        
        for (size_t offset = 0; offset < plaintext.size(); offset += static_cast<size_t>(maxChunk)) {
            const size_t chunkSize = std::min(static_cast<size_t>(maxChunk), plaintext.size() - offset);
            const int written = RSA_public_encrypt(static_cast<int>(chunkSize),
                                                   plaintext.data() + offset,
                                                   buffer.data(),
                                                   rsa.get(),
                                                   padding);
            if (written <= 0) {
                detail::throwOpenSSLError("RSA public encrypt failed");
            }
            encrypted.insert(encrypted.end(), buffer.begin(), buffer.begin() + written);
        }
        
        return encrypted;
    }
    
    inline std::vector<uint8_t> decryptBytes(const std::vector<uint8_t>& ciphertext,
                                            const PemKeyPair& keyPair,
                                            int padding = RSA_PKCS1_OAEP_PADDING) {
        ensureOpenSSLInit();
        
        detail::UniqueRSA rsa(detail::loadPrivateKey(keyPair.privateKeyPem));
        const int rsaSize = RSA_size(rsa.get());
        if (rsaSize <= 0) {
            throw std::runtime_error("invalid RSA key size");
        }
        
        if (ciphertext.empty()) {
            return {};
        }
        
        if (ciphertext.size() % static_cast<size_t>(rsaSize) != 0) {
            throw std::invalid_argument("ciphertext length is not aligned with RSA block size");
        }
        
        std::vector<uint8_t> decrypted;
        decrypted.reserve(ciphertext.size());
        std::vector<uint8_t> buffer(static_cast<size_t>(rsaSize));
        
        for (size_t offset = 0; offset < ciphertext.size(); offset += static_cast<size_t>(rsaSize)) {
            const int written = RSA_private_decrypt(rsaSize,
                                                    ciphertext.data() + offset,
                                                    buffer.data(),
                                                    rsa.get(),
                                                    padding);
            if (written < 0) {
                detail::throwOpenSSLError("RSA private decrypt failed");
            }
            decrypted.insert(decrypted.end(), buffer.begin(), buffer.begin() + written);
        }
        
        return decrypted;
    }
    
    inline std::vector<uint8_t> encryptTextToBytes(const std::string& plaintext,
                                                   const PemKeyPair& keyPair,
                                                   int padding = RSA_PKCS1_OAEP_PADDING) {
        const std::vector<uint8_t> bytes(plaintext.begin(), plaintext.end());
        return encryptBytes(bytes, keyPair, padding);
    }
    
    inline std::string decryptTextFromBytes(const std::vector<uint8_t>& ciphertext,
                                            const PemKeyPair& keyPair,
                                            int padding = RSA_PKCS1_OAEP_PADDING) {
        const std::vector<uint8_t> bytes = decryptBytes(ciphertext, keyPair, padding);
        return std::string(bytes.begin(), bytes.end());
    }
    
    inline std::vector<long long> encryptText(const std::string& plaintext, const KeyPair& keyPair) {
        std::vector<long long> ciphertext;
        ciphertext.reserve(plaintext.size());
        
        for (unsigned char c : plaintext) {
            long long encrypted = encryptNumber(static_cast<long long>(c), keyPair.publicKey, keyPair.modulus);
            ciphertext.push_back(encrypted);
        }
        
        return ciphertext;
    }
    
    inline std::string decryptText(const std::vector<long long>& ciphertext, const KeyPair& keyPair) {
        std::string plaintext;
        plaintext.reserve(ciphertext.size());
        
        for (long long value : ciphertext) {
            long long decrypted = decryptNumber(value, keyPair.privateKey, keyPair.modulus);
            if (decrypted < 0 || decrypted > 255) {
                throw std::runtime_error("decrypted value is outside byte range");
            }
            plaintext += static_cast<char>(decrypted);
        }
        
        return plaintext;
    }
    
    inline std::string ciphertextToString(const std::vector<long long>& ciphertext) {
        std::string result;
        for (size_t i = 0; i < ciphertext.size(); ++i) {
            if (i > 0) result += ",";
            result += std::to_string(ciphertext[i]);
        }
        return result;
    }
    
    inline std::vector<long long> stringToCiphertext(const std::string& str) {
        std::vector<long long> result;
        std::string current;
        
        for (char c : str) {
            if (c == ',') {
                if (!current.empty()) {
                    result.push_back(std::stoll(current));
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        
        if (!current.empty()) {
            result.push_back(std::stoll(current));
        }
        
        return result;
    }
    
    inline void printKeyInfo(const KeyPair& keyPair) {
        std::cout << "RSA Key Information:" << std::endl;
        std::cout << "Public Key (e): " << keyPair.publicKey << std::endl;
        std::cout << "Private Key (d): " << keyPair.privateKey << std::endl;
        std::cout << "Modulus (n): " << keyPair.modulus << std::endl;
    }
}
