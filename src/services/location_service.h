#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include <nlohmann/json.hpp>  // Add this for JSON parsing

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

struct LocationInfo {
    std::string ip;
    std::string country;
    std::string country_code;
    std::string region;
    std::string region_code;
    std::string city;
    std::string zip;
    std::string timezone;
    std::string currency;
    std::string currency_symbol;
    double latitude;
    double longitude;

    // Constructor with default values
    LocationInfo() : 
        latitude(0.0), 
        longitude(0.0) {
        ip = "Detecting...";
        country = "Detecting...";
        country_code = "Detecting...";
        region = "Detecting...";
        region_code = "Detecting...";
        city = "Detecting...";
        zip = "Detecting...";
        timezone = "Detecting...";
        currency = "USD";
        currency_symbol = "$";
    }
};

class LocationService {
public:
    static LocationService& getInstance() {
        static LocationService instance;
        return instance;
    }
    
    LocationInfo getLocationInfo();
    const LocationInfo& getCachedLocationInfo() const { return cachedInfo; }
    bool isLocationAvailable() const { return locationInitialized; }
    
private:
    LocationService() : locationInitialized(false) {}
    ~LocationService() = default;
    LocationService(const LocationService&) = delete;
    LocationService& operator=(const LocationService&) = delete;

    // Helper functions
    std::string getCurrentIP();
    std::string getLocalIP();
    bool getLocationDetails();
    std::string makeHttpRequest(const wchar_t* host, const wchar_t* path);
    bool validateIPFormat(const std::string& ip);
    void parseCurrencyInfo(const std::string& countryCode);
    
    // String conversion helper
    static std::string wstring_to_string(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
        return strTo;
    }

    // Member variables
    LocationInfo cachedInfo;
    bool locationInitialized;
    std::mutex locationMutex;

    // Currency data
    static const std::map<std::string, std::pair<std::string, std::string>> currencyData;
};