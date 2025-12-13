#pragma once

#include "BluetoothSerial.h"
#include "motionControl.h"
#include "system.h"
#include <Arduino.h>
#include <map>
#include <memory>
#include <string>

class BluetoothSlave {
    friend void bluetooth_slave_loop(void *args);

private:
    BluetoothSlave();
    ~BluetoothSlave();

public:
    BluetoothSlave(const BluetoothSlave &) = delete;
    BluetoothSlave &operator=(const BluetoothSlave &) = delete;

    static BluetoothSlave &get_instance();

    void init(const std::string &name);
    void start_task();
    void stop_task();

    bool connected();
    void handle_cmd();

private:
    std::shared_ptr<BluetoothSerial> SerialBT_;
    bool enable_task_run = false;

    MotionControl *motionControl_ = nullptr;
};

void bluetooth_slave_loop(void *args);
