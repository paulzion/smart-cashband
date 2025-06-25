#ifndef BLOCKCHAIN_INTERFACE_H
#define BLOCKCHAIN_INTERFACE_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

class BlockchainInterface {
private:
    const char* serverUrl;
    
public:
    BlockchainInterface(const char* url) : serverUrl(url) {}
    
    bool logAccess(const char* rfidId, bool success, const char* fingerprintId) {
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            String url = String(serverUrl) + "/log-access";
            Serial.print("Connecting to blockchain server: ");
            Serial.println(url);
            
            http.begin(url);
            http.addHeader("Content-Type", "application/json");
            
            String jsonData = "{\"rfidId\":\"" + String(rfidId) + 
                            "\",\"success\":" + String(success ? "true" : "false") + 
                            ",\"fingerprintId\":\"" + String(fingerprintId) + "\"}";
            
            Serial.print("Sending data: ");
            Serial.println(jsonData);
            
            int httpCode = http.POST(jsonData);
            Serial.print("HTTP Response code: ");
            Serial.println(httpCode);
            
            if (httpCode > 0) {
                String response = http.getString();
                Serial.print("Server response: ");
                Serial.println(response);
            } else {
                Serial.print("Error code: ");
                Serial.println(httpCode);
            }
            
            http.end();
            
            if (httpCode == 200) {
                Serial.println("Transaction Completed");
                Serial.println("Band Unlocked");
                return true;
            }
        } else {
            Serial.println("WiFi not connected");
        }
        return false;
    }
};

#endif 