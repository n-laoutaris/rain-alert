#pragma once

// ==========================================
// OPTION A: ESP32-C3 SUPER MINI
// ==========================================
#if defined(esp32_c3_supermini)

    // Hardware Pins
    #define PRG    9   // Tiny "Boot" button
    #define BUZZER        5
    #define LED 8
    #define RAIN_SENSOR_ANALOG 3   
    #define RAIN_SENSOR_POWER  4   
    #define BATTERY       0   

    // Calibration
    #define RAIN_THRESHOLD 3500
    #define VOLTAGE_MULTIPLIER 2.0
    #define BATTERY_OFFSET -30 // Expressed in mV

// ==========================================
// OPTION B: ESP32 STANDARD (DevKit V1)
// ==========================================
#elif defined(esp32dev)

    // Hardware Pins
    #define PRG    0   // Boot button
    #define BUZZER        22
    #define LED 2
    #define RAIN_SENSOR_ANALOG 37  
    #define RAIN_SENSOR_POWER  13
    #define BATTERY       38  

    // Calibration
    #define RAIN_THRESHOLD 3500 
    #define VOLTAGE_MULTIPLIER 2.0
    #define BATTERY_OFFSET -50 // Expressed in mV
#else
    #error "Oops! You must select a board environment in platformio.ini"
#endif