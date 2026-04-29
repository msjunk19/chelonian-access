#pragma once

/**
 * @file features.hpp
 * @brief Build-time feature toggles for Chelonian Access Service
 * 
 * Define these to enable/disable features at compile time:
 * - ENABLE_WIFI_AP: WiFi access point and web server
 * - ENABLE_BLE: Bluetooth Low Energy connectivity
 * 
 * Examples:
 *   - WiFi + BLE:     #define ENABLE_WIFI_AP  #define ENABLE_BLE
 *   - BLE only:       #define ENABLE_BLE
 *   - WiFi only:      #define ENABLE_WIFI_AP
 *   - Neither:        (no defines)
 */

// ============================================
// FEATURE TOGGLES - Uncomment to enable
// ============================================

#define ENABLE_WIFI_AP
#define ENABLE_BLE

// ============================================
// Validation
// ============================================

#if !defined(ENABLE_WIFI_AP) && !defined(ENABLE_BLE)
    #warning "No communication features enabled. Device will only accept RFID/physical access."
#endif