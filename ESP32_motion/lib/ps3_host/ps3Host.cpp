#include "ps3Host.h"
#include <cmath>
#include <cstdlib>

Ps3Host &Ps3Host::get_instance() {
    static Ps3Host instance;
    return instance;
}

Ps3Host::Ps3Host() {}
Ps3Host::~Ps3Host() {}

void Ps3Host::init(const std::string &controller_mac) {
    motionControl_ = &MotionControl::get_instance();
    if (!Ps3.begin(controller_mac.c_str())) {
        Serial.println("[PS3] Ps3.begin 失败");
        return;
    }
    Serial.printf("[PS3] 本机蓝牙地址: %s\n", Ps3.getAddress().c_str());
    Serial.println("[PS3] 等待手柄连接，按手柄 PS 键...");
}

bool Ps3Host::connected() {
    return Ps3.isConnected();
}

void Ps3Host::start_task() {
    if (enable_task_run == false) {
        enable_task_run = true;
        xTaskCreatePinnedToCore(ps3_host_loop, "ps3_host_loop", 4096, this, 0, NULL, 0);
    }
}

void Ps3Host::stop_task() {
    enable_task_run = false;
}

void Ps3Host::handle_input() {
    static const char *button_names[] = {
        "select", "l3", "r3", "start", "up", "right", "down", "left",
        "l2", "r2", "l1", "r1", "triangle", "circle", "cross", "square", "ps"};
    static bool last_button[17] = {false};

    uint8_t button_values[17] = {
        Ps3.data.button.select, Ps3.data.button.l3, Ps3.data.button.r3,
        Ps3.data.button.start, Ps3.data.button.up, Ps3.data.button.right,
        Ps3.data.button.down, Ps3.data.button.left, Ps3.data.button.l2,
        Ps3.data.button.r2, Ps3.data.button.l1, Ps3.data.button.r1,
        Ps3.data.button.triangle, Ps3.data.button.circle, Ps3.data.button.cross,
        Ps3.data.button.square, Ps3.data.button.ps};

    for (int i = 0; i < 17; i++) {
        bool pressed = button_values[i] != 0;
        if (pressed != last_button[i]) {
            last_button[i] = pressed;
            Serial.printf("[PS3] 按键 %-8s %s\n", button_names[i], pressed ? "按下" : "松开");
        }
    }

    int8_t lx = Ps3.data.analog.stick.lx;
    int8_t ly = Ps3.data.analog.stick.ly;
    int8_t rx = Ps3.data.analog.stick.rx;
    int8_t ry = Ps3.data.analog.stick.ry;
    static int8_t last_lx = 0, last_ly = 0, last_rx = 0, last_ry = 0;
    const int8_t threshold = 5;
    if (abs(lx - last_lx) >= threshold || abs(ly - last_ly) >= threshold ||
        abs(rx - last_rx) >= threshold || abs(ry - last_ry) >= threshold) {
        last_lx = lx;
        last_ly = ly;
        last_rx = rx;
        last_ry = ry;
        Serial.printf("[PS3] 摇杆 左(%4d,%4d) 右(%4d,%4d)\n", lx, ly, rx, ry);
    }

    if (motionControl_ == nullptr) {
        return;
    }

    // 旋转：l1(左旋) / r1(右旋) 优先，否则用右摇杆 X（左为+）
    float z_norm = -static_cast<float>(rx) / 128.0f;
    if (Ps3.data.button.l1) {
        z_norm = 1.0f;
    } else if (Ps3.data.button.r1) {
        z_norm = -1.0f;
    }
    z_norm = constrain(z_norm, -1.0f, 1.0f);

    // 平移优先级：面键对(斜移) > 单个方向键(直行) > 左摇杆
    float tx = 0.0f, ty = 0.0f;
    bool face_vertical = Ps3.data.button.triangle || Ps3.data.button.cross;
    bool face_horizontal = Ps3.data.button.square || Ps3.data.button.circle;
    bool face_conflict = (Ps3.data.button.triangle && Ps3.data.button.cross) ||
                         (Ps3.data.button.square && Ps3.data.button.circle);
    int dpad_count = (Ps3.data.button.up ? 1 : 0) + (Ps3.data.button.down ? 1 : 0) +
                     (Ps3.data.button.left ? 1 : 0) + (Ps3.data.button.right ? 1 : 0);

    if (face_vertical && face_horizontal && !face_conflict) {
        // △+□=左前, △+○=右前, ✕+□=左后, ✕+○=右后（对角归一化）
        tx = (Ps3.data.button.circle ? 1.0f : 0.0f) - (Ps3.data.button.square ? 1.0f : 0.0f);
        ty = (Ps3.data.button.triangle ? 1.0f : 0.0f) - (Ps3.data.button.cross ? 1.0f : 0.0f);
        const float inv_sqrt2 = 0.70710678f;
        tx *= inv_sqrt2;
        ty *= inv_sqrt2;
    } else if (dpad_count == 1) {
        tx = (Ps3.data.button.right ? 1.0f : 0.0f) - (Ps3.data.button.left ? 1.0f : 0.0f);
        ty = (Ps3.data.button.up ? 1.0f : 0.0f) - (Ps3.data.button.down ? 1.0f : 0.0f);
    } else {
        tx = static_cast<float>(lx) / 128.0f;
        ty = -static_cast<float>(ly) / 128.0f;
    }
    tx = constrain(tx, -1.0f, 1.0f);
    ty = constrain(ty, -1.0f, 1.0f);

    geometry_msgs__msg__Twist twist;
    twist.linear.x = tx * motionControl_->get_max_speed();
    twist.linear.y = ty * motionControl_->get_max_speed();
    twist.linear.z = 0;
    twist.angular.x = 0;
    twist.angular.y = 0;
    twist.angular.z = z_norm * motionControl_->get_max_angular();
    motionControl_->start_move(twist);

    // 速度档：l2 减 10%，r2 加 10%（边沿触发）
    static bool last_l2 = false, last_r2 = false;
    if (Ps3.data.button.l2 && !last_l2) {
        float sp = motionControl_->get_speed_percent() - 0.1f;
        if (sp < 0.0f) {
            sp = 0.0f;
        }
        motionControl_->set_speed_percent(sp);
        Serial.printf("[PS3] 速度: %.0f%%\n", motionControl_->get_speed_percent() * 100.0f);
    }
    if (Ps3.data.button.r2 && !last_r2) {
        float sp = motionControl_->get_speed_percent() + 0.1f;
        if (sp > 1.0f) {
            sp = 1.0f;
        }
        motionControl_->set_speed_percent(sp);
        Serial.printf("[PS3] 速度: %.0f%%\n", motionControl_->get_speed_percent() * 100.0f);
    }
    last_l2 = Ps3.data.button.l2;
    last_r2 = Ps3.data.button.r2;
}

void ps3_host_loop(void *args) {
    Ps3Host *ps3Host = static_cast<Ps3Host *>(args);
    bool is_connected = false;
    uint32_t combo_start = 0;
    while (ps3Host->enable_task_run) {
        if (Ps3.isConnected()) {
            if (is_connected == false) {
                is_connected = true;
                Serial.printf("[PS3] 手柄已连接，电量等级: %d\n", Ps3.data.status.battery);
            }

            if (Ps3.data.button.select && Ps3.data.button.start) {
                if (combo_start == 0) {
                    combo_start = millis();
                } else if (millis() - combo_start > 2000) {
                    Serial.println("[PS3] Select+Start 长按，切换到手机蓝牙模式...");
                    ps3Host->motionControl_->stop_move();
                    switch_to_bluetooth();
                }
            } else {
                combo_start = 0;
            }

            ps3Host->handle_input();
        } else {
            if (is_connected == true) {
                is_connected = false;
                ps3Host->motionControl_->stop_move();
                Serial.println("[PS3] 手柄已断开连接");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    vTaskDelete(NULL);
}
