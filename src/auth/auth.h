#pragma once
#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include <regex>

// Constants for authentication
const int AUTH_TOKEN_LENGTH = 32;
const int AUTH_TOKEN_EXPIRY_HOURS = 24;
const char AUTH_TOKEN_CHARS[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

// Email validation class
class EmailValidator {
public:
    static bool isValidEmail(const std::string& email) {
        const std::regex pattern(
            "^[a-zA-Z0-9.!#$%&'*+/=?^_{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}"
            "[a-zA-Z0-9])?(?:\\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$"
        );
        return std::regex_match(email, pattern);
    }

private:
    static bool checkEmailFormat(const std::string& email) {
        return isValidEmail(email);
    }
};

struct UserToken {
    std::string token;
    time_t expiryTime;
    std::string email;
};

struct EncryptedData {
    std::vector<unsigned char> data;
    std::vector<unsigned char> iv;
};

class TokenManager {
public:
    TokenManager();
    ~TokenManager();
    
    std::string generateToken(const std::string& email);
    bool validateToken(const std::string& token);
    time_t getTokenExpiry(const std::string& token);
    
private:
    std::string generateRandomString(size_t length);
    EncryptedData encryptData(const std::string& data);
    std::string decryptData(const EncryptedData& encryptedData);
    
    unsigned char key[32];
};

class AuthenticationManager {
public:
    static AuthenticationManager& getInstance();
    
    bool registerUser(const std::string& email);
    bool loginWithToken(const std::string& token);
    std::string getCurrentUserEmail() const;
    std::string getCurrentToken() const { return currentUser.token; }
    bool isUserLoggedIn() const;
    void logout();

private:
    AuthenticationManager();
    ~AuthenticationManager();
    
    AuthenticationManager(const AuthenticationManager&) = delete;
    AuthenticationManager& operator=(const AuthenticationManager&) = delete;

    std::unique_ptr<TokenManager> tokenManager;
    UserToken currentUser;
    bool isLoggedIn;
};