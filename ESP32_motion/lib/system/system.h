#pragma once

#include <Arduino.h>
#include <Preferences.h>

inline void test_ram() {
    // 使用Arduino框架提供的PSRAM相关函数
    if (ESP.getPsramSize() > 0) {
        Serial.printf("检测到PSRAM，容量: %d KB\n", ESP.getPsramSize() / 1024);
    } else {
        Serial.println("未检测到PSRAM");
    }

    unsigned long flashSize = ESP.getFlashChipSize();
    unsigned long flashSpeed = ESP.getFlashChipSpeed();

    // 转换为更易读的单位（MB）
    float flashSizeMB = flashSize / (1024.0 * 1024.0);

    Serial.printf("Flash 总容量: %.2f MB\n", flashSizeMB);
    Serial.printf("Flash 速度: %lu Hz\n", flashSpeed);
    // // 额外：获取Flash芯片型号
    // auto flashModel = ESP.getFlashChipMode();
    // Serial.printf("Flash 型号: %s\n", flashModel);
}

inline void restart_device() {
    ESP.restart();
}

enum CommunicationType {
    WIFI,
    BLUETOOTH,
};

inline void switch_to_wifi() {
    Preferences preferences;
    preferences.begin("comtype", false);
    preferences.clear();
    preferences.putInt("type", CommunicationType::WIFI);
    preferences.end();

    delay(100);
    restart_device();
}

inline void switch_to_bluetooth() {
    Preferences preferences;
    preferences.begin("comtype", false);
    preferences.clear();
    preferences.putInt("type", CommunicationType::BLUETOOTH);
    preferences.end();

    delay(100);
    restart_device();
}

inline CommunicationType get_communication_type() {
    Preferences preferences;
    preferences.begin("comtype", true);
    int type = preferences.getInt("type", CommunicationType::WIFI);
    preferences.end();
    return static_cast<CommunicationType>(type);
}

extern void (*serial_print)(const std::string &);