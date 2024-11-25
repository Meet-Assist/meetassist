#include "auth.h"
#include "../services/email_service.h"
#include <windows.h>
#include <wincrypt.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>

#pragma comment(lib, "crypt32.lib")

TokenManager::TokenManager() {
    // Initialize encryption key with secure random data
    HCRYPTPROV hProvider = 0;
    if (!CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (!CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_AES, CRYPT_NEWKEYSET | CRYPT_VERIFYCONTEXT)) {
            throw std::runtime_error("Failed to acquire crypto context");
        }
    }
    
    if (!CryptGenRandom(hProvider, sizeof(key), key)) {
        CryptReleaseContext(hProvider, 0);
        throw std::runtime_error("Failed to generate encryption key");
    }

    CryptReleaseContext(hProvider, 0);
}

TokenManager::~TokenManager() {
    // Securely clear the key from memory
    SecureZeroMemory(key, sizeof(key));
}

std::string TokenManager::generateToken(const std::string& email) {
    // Generate random token
    std::string tokenData = generateRandomString(AUTH_TOKEN_LENGTH);
    
    // Combine with email and timestamp for uniqueness
    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << email << ":" << now << ":" << tokenData;
    
    // Encrypt the token
    EncryptedData encrypted = encryptData(ss.str());
    
    // Convert to hex string for safe transmission
    std::stringstream result;
    for (unsigned char c : encrypted.data) {
        result << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    
    return result.str();
}

bool TokenManager::validateToken(const std::string& token) {
    try {
        // Convert from hex string back to bytes
        std::vector<unsigned char> data;
        for (size_t i = 0; i < token.length(); i += 2) {
            std::string byteString = token.substr(i, 2);
            unsigned char byte = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
            data.push_back(byte);
        }
        
        // Create encrypted data structure
        EncryptedData encrypted{data, std::vector<unsigned char>(16)}; // IV size is 16 for AES
        
        // Decrypt and parse token components
        std::string decrypted = decryptData(encrypted);
        std::stringstream ss(decrypted);
        std::string email, timestamp, tokenData;
        std::getline(ss, email, ':');
        std::getline(ss, timestamp, ':');
        std::getline(ss, tokenData);
        
        // Check if token has expired
        std::time_t tokenTime = std::stoll(timestamp);
        std::time_t now = std::time(nullptr);
        bool isValid = (now - tokenTime) < (AUTH_TOKEN_EXPIRY_HOURS * 3600);
        
        return isValid;
        
    } catch (const std::exception&) {
        return false;
    }
}

std::string TokenManager::generateRandomString(size_t length) {
    HCRYPTPROV hProvider = 0;
    if (!CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        throw std::runtime_error("Failed to acquire crypto context");
    }

    std::string result;
    result.reserve(length);
    
    std::vector<BYTE> buffer(length);
    if (!CryptGenRandom(hProvider, static_cast<DWORD>(length), buffer.data())) {
        CryptReleaseContext(hProvider, 0);
        throw std::runtime_error("Failed to generate random data");
    }

    for (size_t i = 0; i < length; ++i) {
        result += AUTH_TOKEN_CHARS[buffer[i] % (sizeof(AUTH_TOKEN_CHARS) - 1)];
    }

    CryptReleaseContext(hProvider, 0);
    return result;
}

EncryptedData TokenManager::encryptData(const std::string& data) {
    HCRYPTPROV hProvider = 0;
    HCRYPTKEY hKey = 0;
    std::vector<BYTE> iv(16);
    std::vector<BYTE> ciphertext;

    try {
        // Acquire crypto context
        if (!CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            throw std::runtime_error("Failed to acquire crypto context");
        }

        // Generate IV
        if (!CryptGenRandom(hProvider, static_cast<DWORD>(iv.size()), iv.data())) {
            throw std::runtime_error("Failed to generate IV");
        }

        // Import key
        HCRYPTHASH hHash = 0;
        if (!CryptCreateHash(hProvider, CALG_SHA_256, 0, 0, &hHash)) {
            throw std::runtime_error("Failed to create hash");
        }

        if (!CryptHashData(hHash, key, sizeof(key), 0)) {
            CryptDestroyHash(hHash);
            throw std::runtime_error("Failed to hash key");
        }

        if (!CryptDeriveKey(hProvider, CALG_AES_256, hHash, 0, &hKey)) {
            CryptDestroyHash(hHash);
            throw std::runtime_error("Failed to derive key");
        }

        CryptDestroyHash(hHash);

        // Prepare data for encryption
        std::vector<BYTE> buffer(data.begin(), data.end());
        DWORD dataSize = static_cast<DWORD>(buffer.size());
        DWORD blockSize = 16; // AES block size

        // Pad the data
        size_t paddedSize = ((dataSize + blockSize - 1) / blockSize) * blockSize;
        buffer.resize(paddedSize);

        // Encrypt
        if (!CryptEncrypt(hKey, 0, TRUE, 0, buffer.data(), &dataSize, static_cast<DWORD>(buffer.size()))) {
            throw std::runtime_error("Failed to encrypt data");
        }

        buffer.resize(dataSize);
        ciphertext = std::move(buffer);
    }
    catch (...) {
        if (hKey) CryptDestroyKey(hKey);
        if (hProvider) CryptReleaseContext(hProvider, 0);
        throw;
    }

    if (hKey) CryptDestroyKey(hKey);
    if (hProvider) CryptReleaseContext(hProvider, 0);

    return {ciphertext, iv};
}

std::string TokenManager::decryptData(const EncryptedData& encryptedData) {
    HCRYPTPROV hProvider = 0;
    HCRYPTKEY hKey = 0;
    std::vector<BYTE> plaintext = encryptedData.data;

    try {
        // Acquire crypto context
        if (!CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            throw std::runtime_error("Failed to acquire crypto context");
        }

        // Import key
        HCRYPTHASH hHash = 0;
        if (!CryptCreateHash(hProvider, CALG_SHA_256, 0, 0, &hHash)) {
            throw std::runtime_error("Failed to create hash");
        }

        if (!CryptHashData(hHash, key, sizeof(key), 0)) {
            CryptDestroyHash(hHash);
            throw std::runtime_error("Failed to hash key");
        }

        if (!CryptDeriveKey(hProvider, CALG_AES_256, hHash, 0, &hKey)) {
            CryptDestroyHash(hHash);
            throw std::runtime_error("Failed to derive key");
        }

        CryptDestroyHash(hHash);

        // Decrypt
        DWORD dataSize = static_cast<DWORD>(plaintext.size());
        if (!CryptDecrypt(hKey, 0, TRUE, 0, plaintext.data(), &dataSize)) {
            throw std::runtime_error("Failed to decrypt data");
        }

        plaintext.resize(dataSize);
    }
    catch (...) {
        if (hKey) CryptDestroyKey(hKey);
        if (hProvider) CryptReleaseContext(hProvider, 0);
        throw;
    }

    if (hKey) CryptDestroyKey(hKey);
    if (hProvider) CryptReleaseContext(hProvider, 0);

    return std::string(plaintext.begin(), plaintext.end());
}

// AuthenticationManager implementation
AuthenticationManager::AuthenticationManager()
    : tokenManager(std::make_unique<TokenManager>())
    , isLoggedIn(false) {
}

AuthenticationManager::~AuthenticationManager() = default;

AuthenticationManager& AuthenticationManager::getInstance() {
    static AuthenticationManager instance;
    return instance;
}

bool AuthenticationManager::registerUser(const std::string& email) {
    if (!EmailValidator::isValidEmail(email)) {
        return false;
    }
    
    // Generate activation token
    std::string token = tokenManager->generateToken(email);
    
    // Store user information
    currentUser.email = email;
    currentUser.token = token;
    currentUser.expiryTime = std::time(nullptr) + (AUTH_TOKEN_EXPIRY_HOURS * 3600);
    
    // Send activation email
    bool emailSent = EmailService::getInstance().sendActivationToken(email, token);
    
    return emailSent;
}

bool AuthenticationManager::loginWithToken(const std::string& token) {
    if (!tokenManager->validateToken(token)) {
        return false;
    }
    
    isLoggedIn = true;
    return true;
}

std::string AuthenticationManager::getCurrentUserEmail() const {
    return currentUser.email;
}

bool AuthenticationManager::isUserLoggedIn() const {
    return isLoggedIn && std::time(nullptr) < currentUser.expiryTime;
}

void AuthenticationManager::logout() {
    isLoggedIn = false;
    currentUser = UserToken();
}