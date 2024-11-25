#include "payment_service.h"
#include <ctime>
#include <map>

namespace {
    // In-memory storage for demo purposes
    struct SubscriptionData {
        std::string transactionId;
        time_t expiryDate;
        bool active;
    };
    
    std::map<std::string, SubscriptionData> subscriptions;
}

PaymentService& PaymentService::getInstance() {
    static PaymentService instance;
    return instance;
}

PaymentResult PaymentService::processPayment(const std::string& email, double amount) {
    PaymentResult result;
    
    try {
        // TODO: Integrate with actual payment processor
        // This is a placeholder implementation
        
        // Simulate payment processing
        result.success = true;
        result.transactionId = "TXN" + std::to_string(std::time(nullptr));
        
        // If payment successful, update subscription
        if (result.success) {
            SubscriptionData data;
            data.transactionId = result.transactionId;
            data.expiryDate = std::time(nullptr) + (30 * 24 * 60 * 60); // 30 days
            data.active = true;
            
            subscriptions[email] = data;
        }
    }
    catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
    }
    
    return result;
}

bool PaymentService::hasActiveSubscription(const std::string& email) {
    auto it = subscriptions.find(email);
    if (it != subscriptions.end()) {
        return it->second.active && (std::time(nullptr) < it->second.expiryDate);
    }
    return false;
}

time_t PaymentService::getSubscriptionExpiry(const std::string& email) {
    auto it = subscriptions.find(email);
    if (it != subscriptions.end()) {
        return it->second.expiryDate;
    }
    return 0;
}