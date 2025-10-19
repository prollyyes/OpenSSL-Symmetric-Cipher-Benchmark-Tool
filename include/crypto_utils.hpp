#ifndef CRYPTO_UTILS_HPP
#define CRYPTO_UTILS_HPP

#include <string>
#include <vector>
#include <utility>

// define an enum for cipher types

enum class CipherType {
    AES,
    CAMELLIA,
    SM4,
};

// generate random bytes for keys and IVs
std::vector<unsigned char> generateRandomBytes(size_t length); // size : number of bytes (16 for 128-bit key/IV)

// encrypt data using specified cipher in CBC mode
std::vector<unsigned char> encrypt(
    CipherType cipher,
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
);

// decrypt data using specified cipher in CBC mode
// cipher: which cipher to use (AES, CAMELLIA, SM4)
// ciphertext: data to decrypt
// key: encryption key (16 bytes for 128-bit key)
// iv: initialization vector (16 bytes for 128-bit IV)
std::vector<unsigned char> decrypt(
    CipherType cipher,
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
);

// helper function to convert CipherType to string (for logging/debugging)
std::string cipherTypeToString(CipherType cipher);

// Encrypt data using specified cipher in CBC mode and measure only the crypto time
// (from EVP_EncryptInit_ex through EVP_EncryptFinal_ex) using steady_clock.
// Returns pair<ciphertext, timeMs>
std::pair<std::vector<unsigned char>, double> encrypt_with_timing(
    CipherType cipher,
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
);

// Decrypt data using specified cipher in CBC mode and measure only the crypto time
// (from EVP_DecryptInit_ex through EVP_DecryptFinal_ex) using steady_clock.
// Returns pair<plaintext, timeMs>
std::pair<std::vector<unsigned char>, double> decrypt_with_timing(
    CipherType cipher,
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
);

#endif // CRYPTO_UTILS_HPP
