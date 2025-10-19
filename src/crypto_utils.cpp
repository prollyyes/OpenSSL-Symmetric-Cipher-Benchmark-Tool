#include "crypto_utils.hpp"

#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <stdexcept> // for exceptions
#include <cstring> // for memcpy if needed
#include <chrono>

std::string cipherTypeToString(CipherType cipher) {
    switch(cipher) {
        case CipherType::AES:
            return "AES";
        case CipherType::CAMELLIA:
            return "CAMELLIA";
        case CipherType::SM4:
            return "SM4";
        default:
            return "Unknown";
    }
}

namespace {
const EVP_CIPHER* resolve_cipher(CipherType cipher) {
    switch (cipher) {
        case CipherType::AES:
            return EVP_aes_128_cbc();
        case CipherType::CAMELLIA:
            return EVP_camellia_128_cbc();
        case CipherType::SM4:
            return EVP_sm4_cbc();
        default:
            return nullptr;
    }
}
}

// generate random bytes for keys and IVs
std::vector<unsigned char> generateRandomBytes(size_t size) {
    // create a vector to hold the random bytes, 'size' elements
    std::vector<unsigned char> bytes(size);
    if (!RAND_bytes(bytes.data(), static_cast<int>(size))) {
        throw std::runtime_error("Failed to generate random bytes");
    }
    return bytes;
}


// encrypt data using specified cipher in CBC mode
std::vector<unsigned char> encrypt(
    CipherType cipher,
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
    }

    // select cipher based on CipherType
    const EVP_CIPHER* evp_cipher = resolve_cipher(cipher);
    if (!evp_cipher) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Unsupported cipher type");
    }
    // initialize encryption operation and handle errors
    if (EVP_EncryptInit_ex(ctx, evp_cipher, nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptInit_ex failed");
    }
    // allocate buffer for ciphertext (plaintext size + block size for padding)
    std::vector<unsigned char> ciphertext(plaintext.size() + EVP_CIPHER_block_size(evp_cipher));
    int len = 0;
    int ciphertext_len = 0;

    // encrypt the plaintext
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptUpdate failed");
    }
    ciphertext_len = len;
    // finalize encryption (handle padding)
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptFinal_ex failed");
    }
    ciphertext_len += len;

    // clean up
    EVP_CIPHER_CTX_free(ctx);

    // resize and return the ciphertext
    ciphertext.resize(ciphertext_len);
    return ciphertext;
}

// implement the decrypt function, using the specified cipher in CBC mode
std::vector<unsigned char> decrypt(
    CipherType cipher,
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
    }

    // select cipher based on CipherType
    const EVP_CIPHER* evp_cipher = resolve_cipher(cipher);
    if (!evp_cipher) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Unsupported cipher type");
    }

    // initialize decryption operation and handle errors
    if (EVP_DecryptInit_ex(ctx, evp_cipher, nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptInit_ex failed");
    }

    // allocate buffer for plaintext (ciphertext size, as decrypted data will be <= ciphertext size structurally)
    std::vector<unsigned char> plaintext(ciphertext.size());
    int len = 0;
    int plaintext_len = 0;

    // decrypt the ciphertext
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptUpdate failed");
    }
    plaintext_len = len;

    // finalize decryption (and handle padding removal)
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptFinal_ex failed: failed to finalize decryption");
    }
    plaintext_len += len;

    // clean up
    EVP_CIPHER_CTX_free(ctx);
    // resize and return the plaintext
    plaintext.resize(plaintext_len);
    return plaintext;
}

// Timed encryption: measure only EVP init/update/final using steady_clock
std::pair<std::vector<unsigned char>, double> encrypt_with_timing(
    CipherType cipher,
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
) {
    using clock = std::chrono::steady_clock;
    const EVP_CIPHER* evp_cipher = resolve_cipher(cipher);
    if (!evp_cipher) {
        throw std::runtime_error("Unsupported cipher type");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
    }

    auto t0 = clock::now();
    if (EVP_EncryptInit_ex(ctx, evp_cipher, nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptInit_ex failed");
    }
    std::vector<unsigned char> ciphertext(plaintext.size() + EVP_CIPHER_block_size(evp_cipher));
    int len = 0;
    int ciphertext_len = 0;
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptUpdate failed");
    }
    ciphertext_len = len;
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptFinal_ex failed");
    }
    ciphertext_len += len;
    auto t1 = clock::now();

    EVP_CIPHER_CTX_free(ctx);
    ciphertext.resize(ciphertext_len);

    std::chrono::duration<double, std::milli> dt = t1 - t0;
    return {ciphertext, dt.count()};
}

// Timed decryption: measure only EVP init/update/final using steady_clock
std::pair<std::vector<unsigned char>, double> decrypt_with_timing(
    CipherType cipher,
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
) {
    using clock = std::chrono::steady_clock;
    const EVP_CIPHER* evp_cipher = resolve_cipher(cipher);
    if (!evp_cipher) {
        throw std::runtime_error("Unsupported cipher type");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
    }

    auto t0 = clock::now();
    if (EVP_DecryptInit_ex(ctx, evp_cipher, nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptInit_ex failed");
    }
    std::vector<unsigned char> plaintext(ciphertext.size());
    int len = 0;
    int plaintext_len = 0;
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptUpdate failed");
    }
    plaintext_len = len;
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptFinal_ex failed: failed to finalize decryption");
    }
    plaintext_len += len;
    auto t1 = std::chrono::steady_clock::now();

    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(plaintext_len);

    std::chrono::duration<double, std::milli> dt = t1 - t0;
    return {plaintext, dt.count()};
}
