#pragma once
#include <string>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace RSA {
    
    // RSA Key Pair structure
    struct KeyPair {
        long long publicKey;    // Public key e
        long long privateKey;   // Private key d
        long long modulus;      // Modulus n
    };
    
    // Calculate Greatest Common Divisor
    long long gcd(long long a, long long b) {
        while (b != 0) {
            long long temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }
    
    // Extended Euclidean Algorithm to calculate modular inverse
    long long extendedGcd(long long a, long long b, long long& x, long long& y) {
        if (a == 0) {
            x = 0;
            y = 1;
            return b;
        }
        long long x1, y1;
        long long gcd = extendedGcd(b % a, a, x1, y1);
        x = y1 - (b / a) * x1;
        y = x1;
        return gcd;
    }
    
    // Calculate modular inverse
    long long modInverse(long long a, long long m) {
        long long x, y;
        long long g = extendedGcd(a, m, x, y);
        if (g != 1) return -1; // Modular inverse does not exist
        return (x % m + m) % m;
    }
    
    // Fast modular exponentiation
    long long modPow(long long base, long long exp, long long mod) {
        long long result = 1;
        base = base % mod;
        while (exp > 0) {
            if (exp % 2 == 1) {
                result = (result * base) % mod;
            }
            exp = exp >> 1;
            base = (base * base) % mod;
        }
        return result;
    }
    
    // Simple primality test
    bool isPrime(long long n) {
        if (n <= 1) return false;
        if (n <= 3) return true;
        if (n % 2 == 0 || n % 3 == 0) return false;
        
        for (long long i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0) {
                return false;
            }
        }
        return true;
    }
    
    // Generate random prime in specified range
    long long generatePrime(long long min, long long max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<long long> dis(min, max);
        
        long long candidate;
        do {
            candidate = dis(gen);
            if (candidate % 2 == 0) candidate++; // Ensure it's odd
        } while (!isPrime(candidate));
        
        return candidate;
    }
    
    // Generate RSA key pair
    KeyPair generateKeyPair() {
        // Generate two distinct primes
        long long p = generatePrime(100, 1000);
        long long q;
        do {
            q = generatePrime(100, 1000);
        } while (q == p);
        
        // Calculate n = p * q
        long long n = p * q;
        
        // Calculate Euler's totient function φ(n) = (p-1)(q-1)
        long long phi = (p - 1) * (q - 1);
        
        // Choose public exponent e, typically 65537
        long long e = 65537;
        if (e >= phi || gcd(e, phi) != 1) {
            // If 65537 is not suitable, choose a smaller value
            e = 3;
            while (gcd(e, phi) != 1) {
                e += 2;
            }
        }
        
        // Calculate private exponent d
        long long d = modInverse(e, phi);
        
        return {e, d, n};
    }
    
    // RSA encrypt single number
    long long encryptNumber(long long message, long long publicKey, long long modulus) {
        return modPow(message, publicKey, modulus);
    }
    
    // RSA decrypt single number
    long long decryptNumber(long long ciphertext, long long privateKey, long long modulus) {
        return modPow(ciphertext, privateKey, modulus);
    }
    
    // Encrypt text
    std::vector<long long> encryptText(const std::string& plaintext, const KeyPair& keyPair) {
        std::vector<long long> ciphertext;
        
        for (char c : plaintext) {
            // Convert character to number (ASCII value)
            long long charValue = static_cast<long long>(static_cast<unsigned char>(c));
            
            // Ensure character value is less than modulus
            if (charValue >= keyPair.modulus) {
                throw std::runtime_error("Character value too large for modulus");
            }
            
            // Encrypt character
            long long encrypted = encryptNumber(charValue, keyPair.publicKey, keyPair.modulus);
            ciphertext.push_back(encrypted);
        }
        
        return ciphertext;
    }
    
    // Decrypt text
    std::string decryptText(const std::vector<long long>& ciphertext, const KeyPair& keyPair) {
        std::string plaintext;
        
        for (long long encrypted : ciphertext) {
            // Decrypt number
            long long decrypted = decryptNumber(encrypted, keyPair.privateKey, keyPair.modulus);
            
            // Convert number back to character
            char c = static_cast<char>(decrypted);
            plaintext += c;
        }
        
        return plaintext;
    }
    
    // Convert ciphertext vector to string (for storage or transmission)
    std::string ciphertextToString(const std::vector<long long>& ciphertext) {
        std::string result;
        for (size_t i = 0; i < ciphertext.size(); ++i) {
            if (i > 0) result += ",";
            result += std::to_string(ciphertext[i]);
        }
        return result;
    }
    
    // Convert string to ciphertext vector
    std::vector<long long> stringToCiphertext(const std::string& str) {
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
    
    // Print key information
    void printKeyInfo(const KeyPair& keyPair) {
        std::cout << "RSA Key Information:" << std::endl;
        std::cout << "Public Key (e): " << keyPair.publicKey << std::endl;
        std::cout << "Private Key (d): " << keyPair.privateKey << std::endl;
        std::cout << "Modulus (n): " << keyPair.modulus << std::endl;
    }
}