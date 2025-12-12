#include "bluetoothSlave.h"

BluetoothSlave &BluetoothSlave::get_instance() {
    static BluetoothSlave instance; // C++11 保证线程安全初始化
    return instance;
}

BluetoothSlave::BluetoothSlave() {
    SerialBT_ = std::make_shared<BluetoothSerial>();
    if (SerialBT_ == nullptr) {
        Serial.println("BluetoothSerial 创建失败！");
    } else {
        Serial.println("BluetoothSerial 创建成功！");
    }
}
BluetoothSlave::~BluetoothSlave() {}

void BluetoothSlave::init(const std::string &name) {
    motionControl_ = &MotionControl::get_instance();
    if (SerialBT_) {
        SerialBT_->begin(String(name.c_str()));
        Serial.println("蓝牙已启动，等待连接...");
    } else {
        Serial.println("BluetoothSerial 未初始化，无法启动蓝牙！");
    }
}

void BluetoothSlave::start_task() {
    if (enable_task_run == false && SerialBT_) {
        enable_task_run = true;
        xTaskCreatePinnedToCore(bluetooth_slave_loop, "bluetooth_slave_loop", 8192, this, 0, NULL, 0);
    } else {
        Serial.println("BluetoothSerial 未初始化，无法启动蓝牙任务！");
    }
}

void BluetoothSlave::stop_task() {
    if (enable_task_run) {
        enable_task_run = false;
    }
}

void BluetoothSlave::handle_cmd() {
    if (SerialBT_->available()) {
        String cmdStr = SerialBT_->readStringUntil('\n');
        cmdStr.trim(); // 去除空格/换行 读取单个指令字符
        // Serial.print("收到控制指令：");
        // Serial.println(cmdStr);

        std::map<std::string, std::string> cmd_map;
        for (int begin_pos = 0, split_pos = 0; begin_pos < cmdStr.length(); begin_pos++) {
            std::string valid_cmd;
            if (cmdStr[begin_pos] == ',') {
                valid_cmd = std::string(cmdStr.substring(split_pos, begin_pos).c_str());
                split_pos = begin_pos + 1;
            }
            if (begin_pos == cmdStr.length() - 1) {
                valid_cmd = std::string(cmdStr.substring(split_pos, begin_pos + 1).c_str());
            }
            if (!valid_cmd.empty()) {
                int pos = valid_cmd.find(':');
                if (pos != -1) {
                    cmd_map[valid_cmd.substr(0, pos)] = valid_cmd.substr(pos + 1, valid_cmd.size() - pos - 1);
                }
            }
        }

        if (motionControl_) {
            double v_percent = -1.0;
            geometry_msgs__msg__Twist twist;
            for (auto cmd : cmd_map) {
                if (cmd.first == "x") {
                    twist.linear.x = std::stod(cmd.second) * motionControl_->get_max_speed();
                } else if (cmd.first == "y") {
                    twist.linear.y = std::stod(cmd.second) * motionControl_->get_max_speed();
                } else if (cmd.first == "z") {
                    twist.angular.z = std::stod(cmd.second) * motionControl_->get_max_angular();
                } else if (cmd.first == "v") {
                    v_percent = std::stod(cmd.second);
                }
            }

            if (v_percent < 0.0) {
                motionControl_->start_move(twist);
            } else {
                motionControl_->set_speed_percent(v_percent);
            }
        }
    }
}

void bluetooth_slave_loop(void *args) {
    BluetoothSlave *bluetoothSlave = static_cast<BluetoothSlave *>(args);
    while (bluetoothSlave->enable_task_run) {
        if (bluetoothSlave->SerialBT_->connected()) {
            if (bluetoothSlave->is_connected == false) {
                bluetoothSlave->is_connected = true;
                Serial.println("蓝牙设备已连接");
            }
            bluetoothSlave->handle_cmd();
            // vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            if (bluetoothSlave->is_connected == true) {
                bluetoothSlave->is_connected = false;
                Serial.println("蓝牙设备已断开连接");
            }
            vTaskDelay(pdMS_TO_TICKS(1000)); // 等待1000ms再检查连接状态
        }
    }
    vTaskDelete(NULL);
}