#pragma once
#include <string>
#include <memory>

class EmailService {
public:
    static EmailService& getInstance();
    
    // Send activation token via email
    bool sendActivationToken(const std::string& email, const std::string& token);
    
private:
    EmailService() = default;
    ~EmailService() = default;
    EmailService(const EmailService&) = delete;
    EmailService& operator=(const EmailService&) = delete;

    // Helper function to simulate email sending
    bool simulateEmailSend(const std::string& to, const std::string& subject, const std::string& body);
};