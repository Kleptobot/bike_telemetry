#pragma once
#include <Arduino.h>

// Option 1: Lightweight MAC address struct (RECOMMENDED)
struct MacAddress {
    uint8_t bytes[6];
    
    // Default constructor
    MacAddress() {
        memset(bytes, 0, 6);
    }
    
    // Constructor from array
    MacAddress(const uint8_t* data) {
        memcpy(bytes, data, 6);
    }
    
    // Constructor from individual bytes
    MacAddress(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5) {
        bytes[0] = b0; bytes[1] = b1; bytes[2] = b2;
        bytes[3] = b3; bytes[4] = b4; bytes[5] = b5;
    }
    
    // Assignment from uint8_t pointer
    MacAddress& operator=(const uint8_t* data) {
        memcpy(bytes, data, 6);
        return *this;
    }
    
    // Copy from uint8_t pointer
    void copyFrom(const uint8_t* data) {
        memcpy(bytes, data, 6);
    }
    
    // Comparison operators (very useful for searching/filtering)
    bool operator==(const MacAddress& other) const {
        return memcmp(bytes, other.bytes, 6) == 0;
    }
    
    bool operator!=(const MacAddress& other) const {
        return !(*this == other);
    }
    
    // Less-than operator (for sorting, maps, sets)
    bool operator<(const MacAddress& other) const {
        return memcmp(bytes, other.bytes, 6) < 0;
    }
    
    // Array access
    uint8_t& operator[](size_t index) {
        return bytes[index];
    }
    
    const uint8_t& operator[](size_t index) const {
        return bytes[index];
    }
    
    // Get as raw pointer (for BLE APIs)
    uint8_t* data() {
        return bytes;
    }
    
    const uint8_t* data() const {
        return bytes;
    }
    
    // String conversion for debugging
    String toString() const {
        String result;
        for (int i = 0; i < 6; i++) {
            if (bytes[i] < 0x10) result += "0";
            result += String(bytes[i], HEX);
            if (i < 5) result += ":";
        }
        result.toUpperCase();
        return result;
    }
    
    // Parse from string (e.g., "AA:BB:CC:DD:EE:FF")
    static MacAddress fromString(const String& str) {
        MacAddress mac;
        int byteIndex = 0;
        String temp = "";
        
        for (unsigned int i = 0; i < str.length() && byteIndex < 6; i++) {
            char c = str.charAt(i);
            if (c == ':' || c == '-' || c == ' ') {
                if (temp.length() > 0) {
                    mac.bytes[byteIndex++] = strtol(temp.c_str(), NULL, 16);
                    temp = "";
                }
            } else {
                temp += c;
            }
        }
        if (temp.length() > 0 && byteIndex < 6) {
            mac.bytes[byteIndex] = strtol(temp.c_str(), NULL, 16);
        }
        return mac;
    }
    
    // Check if MAC is all zeros (null/invalid)
    bool isNull() const {
        for (int i = 0; i < 6; i++) {
            if (bytes[i] != 0) return false;
        }
        return true;
    }
    
    // Check if broadcast address
    bool isBroadcast() const {
        for (int i = 0; i < 6; i++) {
            if (bytes[i] != 0xFF) return false;
        }
        return true;
    }
};