#include "email_service.h"
#include "location_service.h"
#include <sstream>
#include <fstream>
#include <iostream>

EmailService& EmailService::getInstance() {
    static EmailService instance;
    return instance;
}

bool EmailService::sendActivationToken(const std::string& email, const std::string& token) {
    // Get location info
    LocationInfo location = LocationService::getInstance().getLocationInfo();
    
    std::stringstream body;
    body << "Welcome to MeetAssist!\n\n"
         << "Your activation token is: " << token << "\n\n"
         << "Please use this token to activate your account.\n"
         << "This token will expire in 24 hours.\n\n"
         << "Location Information:\n"
         << "IP: " << location.ip << "\n"
         << "Country: " << location.country << "\n"
         << "Region: " << location.region << "\n"
         << "City: " << location.city << "\n"
         << "Currency: " << location.currency << " (" << location.currency_symbol << ")\n\n"
         << "Best regards,\n"
         << "MeetAssist Team";

    return simulateEmailSend(email, "MeetAssist Activation Token", body.str());
}

bool EmailService::simulateEmailSend(const std::string& to, const std::string& subject, const std::string& body) {
    try {
        // Log the email for debugging
        std::ofstream logFile("email_log.txt", std::ios::app);
        if (logFile.is_open()) {
            logFile << "\n=== New Email ===\n"
                   << "Timestamp: " << std::time(nullptr) << "\n"
                   << "To: " << to << std::endl
                   << "Subject: " << subject << std::endl
                   << "Body:\n" << body << std::endl
                   << "==================\n" << std::endl;
            logFile.close();
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error sending email: " << e.what() << std::endl;
        return false;
    }
}