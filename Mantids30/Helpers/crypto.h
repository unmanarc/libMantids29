#pragma once


#include <memory>
#include <optional>
#include <string>
#include "mem.h"

namespace Mantids30 { namespace Helpers {

/**
 * @brief A class that provides cryptographic functions for encrypting and hashing data.
 */
class Crypto
{
public:
    /**
     * Constructs a new Crypto object.
     */
    Crypto();

    /**
     * @brief Encrypts binary data using AES-256 encryption and base64 encoding.
     *
     * @param input A pointer to the input binary data.
     * @param inputLen The length of the input binary data in bytes.
     * @param key A pointer to the encryption key.
     * @param keyLen The length of the encryption key in bytes.
     *
     * @return The encrypted binary data as a base64-encoded string.
     */
    static std::optional<std::string> AES256EncryptB64(const unsigned char *input, const size_t &inputLen, const char *key, const size_t &keyLen);

    /**
     * @brief Encrypts a string using AES-256 encryption and base64 encoding.
     *
     * @param input The input string to encrypt.
     * @param key A pointer to the encryption key.
     * @param keyLen The length of the encryption key in bytes.
     *
     * @return The encrypted string as a base64-encoded string.
     */
    static std::optional<std::string> AES256EncryptB64(const std::string &input, const char *key, const size_t &keyLen);

    /**
     * @brief Encrypts a string using AES-256 encryption and base64 encoding.
     *
     * @param input The input string to encrypt.
     * @param key The encryption key as a string.
     *
     * @return The encrypted string as a base64-encoded string.
     */
    static std::optional<std::string> AES256EncryptB64(const std::string& input, const std::string& key);

    /**
     * @brief Decrypts a base64-encoded string to binary data using AES-256 encryption.
     *
     * @param input The base64-encoded string to decrypt.
     * @param key A pointer to the decryption key.
     * @param keyLen The length of the decryption key in bytes.
     *
     * @return A shared pointer to a BinaryDataContainer object containing the decrypted binary data.
     */
    static std::shared_ptr<Mem::BinaryDataContainer> AES256DecryptB64ToBin(const std::string& input, const char* key, const size_t & keyLen);

    /**
     * @brief Decrypts a base64-encoded string using AES-256 encryption.
     *
     * @param input The base64-encoded string to decrypt.
     * @param key A pointer to the decryption key.
     * @param keyLen The length of the decryption key in bytes.
     *
     * @return The decrypted string.
     */
    static std::optional<std::string> AES256DecryptB64(const std::string &input, const char *key, const size_t &keyLen);

    /**
     * @brief Decrypts a base64-encoded string using AES-256 encryption.
     *
     * @param input The base64-encoded string to decrypt.
     * @param key The decryption key as a string.
     *
     * @return The decrypted string.
     */
    static std::optional<std::string> AES256DecryptB64(const std::string& input, const std::string& key);

    /**
     * @brief Calculates the SHA-256 hash of a string.
     *
     * @param password The string to hash.
     *
     * @return The SHA-256 hash of the string.
     */
    static std::string calcSHA256(const std::string& password);

    /**
     * @brief Calculates the SHA-512 hash of a string.
     *
     * @param password The string to hash.
     *
     * @return The SHA-512 hash of the string.
     */
    static std::string calcSHA512(const std::string& password);

    /**
     * @brief Calculates the salted SHA-256 hash of a string.
     *
     * @param password The string to hash.
     * @param ssalt A pointer to a 4-byte salt value to use in the hash.
     *
     * @return The salted SHA-256 hash of the string.
     */
    static std::string calcSSHA256(const std::string& password, const unsigned char* ssalt);

    /**
     * @brief Calculates the salted SHA-512 hash of a string.
     *
     * @param password The string to hash.
     * @param ssalt A pointer to a 4-byte salt value to use in the hash.
     *
     * @return The salted SHA-512 hash of the string.
     */
    static std::string calcSSHA512(const std::string& password, const unsigned char* ssalt);
};



}}

