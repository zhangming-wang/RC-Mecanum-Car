#pragma once

#include "motionControl.h"
#include "system.h"
#include <Arduino.h>
#include <Ps3Controller.h>
#include <string>

class Ps3Host {
    friend void ps3_host_loop(void *args);

private:
    Ps3Host();
    ~Ps3Host();

public:
    Ps3Host(const Ps3Host &) = delete;
    Ps3Host &operator=(const Ps3Host &) = delete;

    static Ps3Host &get_instance();

    void init(const std::string &controller_mac);
    void start_task();
    void stop_task();

    bool connected();
    void handle_input();

private:
    bool enable_task_run = false;
    MotionControl *motionControl_ = nullptr;
};

void ps3_host_loop(void *args);
