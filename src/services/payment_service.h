#pragma once
#include <string>

struct PaymentResult {
    bool success;
    std::string transactionId;
    std::string errorMessage;
};

class PaymentService {
public:
    static PaymentService& getInstance();
    
    // Process payment and return result
    PaymentResult processPayment(const std::string& email, double amount);
    
    // Check if user has active subscription
    bool hasActiveSubscription(const std::string& email);
    
    // Get subscription expiry date
    time_t getSubscriptionExpiry(const std::string& email);

private:
    PaymentService() = default;
    ~PaymentService() = default;
    PaymentService(const PaymentService&) = delete;
    PaymentService& operator=(const PaymentService&) = delete;
};