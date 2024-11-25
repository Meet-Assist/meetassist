#include "location_service.h"
#include <regex>
#include <sstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <iphlpapi.h>

using json = nlohmann::json;

// Static currency data initialization
const std::map<std::string, std::pair<std::string, std::string>> LocationService::currencyData = {
    {"US", {"USD", "$"}},
    {"GB", {"GBP", "£"}},
    {"EU", {"EUR", "€"}},
    {"JP", {"JPY", "¥"}},
    {"IN", {"INR", "₹"}},
    // Add more currency mappings as needed
};

void WriteDebugLog(const std::string& message) {
    std::string fullMessage = "[Location] " + message + "\n";
    OutputDebugStringA(fullMessage.c_str());
}

LocationInfo LocationService::getLocationInfo() {
    std::lock_guard<std::mutex> lock(locationMutex);
    
    if (locationInitialized) {
        return cachedInfo;
    }

    WriteDebugLog("Starting location detection...");

    // Get IP first
    cachedInfo.ip = getCurrentIP();
    WriteDebugLog("IP Address detected: " + cachedInfo.ip);

    // Get location details
    if (getLocationDetails()) {
        WriteDebugLog("Location details retrieved successfully");
    } else {
        WriteDebugLog("Failed to get location details");
    }

    locationInitialized = true;
    return cachedInfo;
}

bool LocationService::getLocationDetails() {
    try {
        // Use ip-api.com for location data
        std::wstring host = L"ip-api.com";
        std::wstring path = L"/json/" + std::wstring(cachedInfo.ip.begin(), cachedInfo.ip.end());
        
        std::string response = makeHttpRequest(host.c_str(), path.c_str());
        if (response.empty()) {
            WriteDebugLog("Failed to get response from ip-api.com");
            return false;
        }

        // Parse JSON response
        json data = json::parse(response);
        
        if (data["status"] == "success") {
            cachedInfo.country = data["country"].get<std::string>();
            cachedInfo.country_code = data["countryCode"].get<std::string>();
            cachedInfo.region = data["regionName"].get<std::string>();
            cachedInfo.region_code = data["region"].get<std::string>();
            cachedInfo.city = data["city"].get<std::string>();
            cachedInfo.zip = data.value("zip", "");
            cachedInfo.timezone = data["timezone"].get<std::string>();
            cachedInfo.latitude = data["lat"].get<double>();
            cachedInfo.longitude = data["lon"].get<double>();

            // Get currency information based on country code
            parseCurrencyInfo(cachedInfo.country_code);

            WriteDebugLog("Location data parsed successfully");
            return true;
        } else {
            WriteDebugLog("IP-API returned error status");
            return false;
        }
    }
    catch (const std::exception& e) {
        WriteDebugLog("Error parsing location data: " + std::string(e.what()));
        return false;
    }
}

void LocationService::parseCurrencyInfo(const std::string& countryCode) {
    auto it = currencyData.find(countryCode);
    if (it != currencyData.end()) {
        cachedInfo.currency = it->second.first;
        cachedInfo.currency_symbol = it->second.second;
    } else {
        // Use a fallback currency service
        try {
            std::wstring host = L"restcountries.com";
            std::wstring path = L"/v3.1/alpha/" + std::wstring(countryCode.begin(), countryCode.end());
            
            std::string response = makeHttpRequest(host.c_str(), path.c_str());
            if (!response.empty()) {
                json data = json::parse(response);
                if (!data.empty()) {
                    // Parse currency information from the response
                    auto currencies = data[0]["currencies"];
                    for (auto& [code, info] : currencies.items()) {
                        cachedInfo.currency = code;
                        if (info.contains("symbol")) {
                            cachedInfo.currency_symbol = info["symbol"].get<std::string>();
                        }
                        break; // Just take the first currency
                    }
                }
            }
        }
        catch (const std::exception& e) {
            WriteDebugLog("Error getting currency data: " + std::string(e.what()));
        }
    }
}

std::string LocationService::getCurrentIP() {
    WriteDebugLog("Starting IP detection...");
    
    // First try online services
    std::vector<std::pair<std::wstring, std::wstring>> ipServices = {
        {L"api.ipify.org", L"/?format=text"},
        {L"checkip.amazonaws.com", L"/"},
        {L"icanhazip.com", L"/"},
        {L"ifconfig.me", L"/ip"}
    };

    for (const auto& service : ipServices) {
        try {
            WriteDebugLog("Trying IP service: " + wstring_to_string(service.first));
            std::string response = makeHttpRequest(service.first.c_str(), service.second.c_str());
            
            // Clean up response
            response.erase(std::remove_if(response.begin(), response.end(), 
                [](char c) { return std::isspace(c); }), response.end());
            
            if (validateIPFormat(response)) {
                WriteDebugLog("Successfully retrieved IP: " + response);
                return response;
            }
        }
        catch (const std::exception& e) {
            WriteDebugLog("Error with service " + wstring_to_string(service.first) + ": " + e.what());
        }
    }

    WriteDebugLog("Online services failed, trying local IP detection...");
    return getLocalIP();
}

std::string LocationService::getLocalIP() {
    std::string result;
    DWORD dwRetVal = 0;
    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    
    do {
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        if (pAddresses == nullptr) {
            WriteDebugLog("Memory allocation failed");
            return "";
        }

        dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);
        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            free(pAddresses);
            pAddresses = nullptr;
        }
    } while (dwRetVal == ERROR_BUFFER_OVERFLOW);

    if (dwRetVal == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            if (pCurrAddresses->OperStatus == IfOperStatusUp &&
                pCurrAddresses->IfType != IF_TYPE_SOFTWARE_LOOPBACK) {
                
                PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                while (pUnicast != nullptr) {
                    if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                        sockaddr_in* sa_in = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                        char ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &(sa_in->sin_addr), ip, INET_ADDRSTRLEN);
                        std::string ipStr(ip);
                        if (ipStr != "127.0.0.1" && ipStr.substr(0, 3) != "169") {
                            result = ipStr;
                            break;
                        }
                    }
                    pUnicast = pUnicast->Next;
                }
            }
            if (!result.empty()) break;
            pCurrAddresses = pCurrAddresses->Next;
        }
    }

    if (pAddresses) {
        free(pAddresses);
    }

    if (result.empty()) {
        result = "127.0.0.1";
    }

    WriteDebugLog("Local IP detected: " + result);
    return result;
}

bool LocationService::validateIPFormat(const std::string& ip) {
    std::regex ipPattern("^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$");
    
    if (std::regex_match(ip, ipPattern)) {
        std::stringstream ss(ip);
        std::string octet;
        while (std::getline(ss, octet, '.')) {
            try {
                int value = std::stoi(octet);
                if (value < 0 || value > 255) {
                    return false;
                }
            }
            catch (...) {
                return false;
            }
        }
        return true;
    }
    return false;
}

std::string LocationService::makeHttpRequest(const wchar_t* host, const wchar_t* path) {
    std::string response;
    HINTERNET hInternet = nullptr;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;

    try {
        hInternet = WinHttpOpen(L"MeetAssist Location Service/1.0",
                               WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                               WINHTTP_NO_PROXY_NAME,
                               WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hInternet) {
            WriteDebugLog("Failed to initialize WinHTTP");
            return response;
        }

        hConnect = WinHttpConnect(hInternet, host, INTERNET_DEFAULT_HTTP_PORT, 0);
        if (!hConnect) {
            WriteDebugLog("Failed to connect to host");
            WinHttpCloseHandle(hInternet);
            return response;
        }

        hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
                                    nullptr, WINHTTP_NO_REFERER,
                                    WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest) {
            WriteDebugLog("Failed to create request");
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hInternet);
            return response;
        }

        if (!WinHttpSendRequest(hRequest,
                               WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            WriteDebugLog("Failed to send request");
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hInternet);
            return response;
        }

        if (!WinHttpReceiveResponse(hRequest, nullptr)) {
            WriteDebugLog("Failed to receive response");
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hInternet);
            return response;
        }

        DWORD bytesAvailable = 0;
        DWORD bytesRead = 0;
        std::vector<char> buffer;

        do {
            bytesAvailable = 0;
            WinHttpQueryDataAvailable(hRequest, &bytesAvailable);
            
            if (bytesAvailable == 0) break;

            buffer.resize(bytesAvailable + 1);
            
            if (!WinHttpReadData(hRequest, buffer.data(),
                                bytesAvailable, &bytesRead)) {
                WriteDebugLog("Error reading data");
                break;
            }

            buffer[bytesRead] = '\0';
            response += buffer.data();
        } while (bytesAvailable > 0);
    }
    catch (const std::exception& e) {
        WriteDebugLog("Error in makeHttpRequest: " + std::string(e.what()));
    }
    catch (...) {
        WriteDebugLog("Unknown error in makeHttpRequest");
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hInternet) WinHttpCloseHandle(hInternet);

    return response;
}