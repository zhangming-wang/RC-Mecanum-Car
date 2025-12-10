#include "bluetoothSlave.h"
#include "motionControl.h"
#include "motionNode.h"
#include "serialPrint.h"
#include "settings.h"
#include "tools.h"
#include <Arduino.h>

enum CommunicationType {
    WIFI,
    BLUETOOTH,
};

CommunicationType communication_type = CommunicationType::WIFI;

void (*serial_print)(const std::string &) = _serial_print;

void setup() {
    Serial.begin(115200);
    while (!Serial) { // 等待主机连接到 CDC 端口
        delay(10);
    }

    test_ram();

    if (communication_type == CommunicationType::WIFI) {
        MotionControl &motionControl = MotionControl::get_instance();
        MotionNode &motionNode = MotionNode::get_instance();

        motionControl.init();
        motionNode.init(esp32_motion_node_name, esp32_motion_node_namespace, wifi_name, wifi_password, wifi_IP, micro_ros_port);

        motionControl.start_task();
        motionNode.start_task();
    } else {
        MotionControl &motionControl = MotionControl::get_instance();
        BluetoothSlave &btSlave = BluetoothSlave::get_instance();

        motionControl.init();
        btSlave.init(esp32_bluetooth_slave_name);

        motionControl.start_task();
        btSlave.start_task();
    }
}

bool connected = false;
bool initialized = false;

void loop() {
    if (communication_type == CommunicationType::WIFI) {
        if (!initialized) {
            WiFi.mode(WIFI_STA);
            WiFi.persistent(false);
            initialized = true;
        }

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
    delay(1000);
}