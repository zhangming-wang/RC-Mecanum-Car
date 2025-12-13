#include "bluetoothSlave.h"
#include "motionControl.h"
#include "motionNode.h"
#include "settings.h"
#include "system.h"
#include <Arduino.h>

CommunicationType communication_type = CommunicationType::WIFI;
MotionControl *motionControl = nullptr;
BluetoothSlave *bluetoothSlave = nullptr;
MotionNode *motionNode = nullptr;
bool connected = false;

void setup() {
    Serial.begin(115200);
    while (!Serial) { // 等待主机连接到 CDC 端口
        delay(50);
    }

    test_ram();

    communication_type = get_communication_type();
    if (communication_type == CommunicationType::WIFI) {
        WiFi.mode(WIFI_STA);
        WiFi.persistent(false);

        motionNode = &MotionNode::get_instance();
        motionNode->init(esp32_motion_node_name, esp32_motion_node_namespace, wifi_name, wifi_password, wifi_IP, micro_ros_port);
        motionNode->start_task();
    } else {
        bluetoothSlave = &BluetoothSlave::get_instance();
        bluetoothSlave->init(esp32_bluetooth_slave_name);
        bluetoothSlave->start_task();
    }

    motionControl = &MotionControl::get_instance();
    motionControl->init();
    motionControl->start_task();
}

void loop() {
    if (communication_type == CommunicationType::WIFI) {
        if (WiFi.status() != WL_CONNECTED) {
            connected = false;

            WiFi.begin(wifi_name, wifi_password);
            Serial.printf("\nConnecting to WiFi...");
            unsigned long start = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
                delay(500);
                Serial.print(".");
            }
            Serial.println();
        } else {
            if (connected == false) {
                connected = true;
                Serial.println("===== WiFi Info =====");
                Serial.printf("IP      : %s\n", WiFi.localIP().toString().c_str());
                Serial.printf("RSSI    : %d dBm\n", WiFi.RSSI());
                Serial.printf("MAC     : %s\n", WiFi.macAddress().c_str());
                Serial.printf("SSID    : %s\n", WiFi.SSID().c_str());
                Serial.println("=====================");
                Serial.println("WiFi connect success!");
            }
        }
    }
    delay(500);
}