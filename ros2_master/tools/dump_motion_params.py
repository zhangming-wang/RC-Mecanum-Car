#!/usr/bin/env python3
"""导出 ESP32 运动控制的内存参数/配置为 JSON。

通过 micro-ROS 服务 /esp32/motion_settings_service 分别调用
ReadParams(mode=19) 与 ReadConfig(mode=22)，合并后写成分组 JSON，
作为 ESP32_motion 的默认配置（NVS 未保存时使用）。

用法:
    source /opt/ros/humble/setup.bash
    source ros2_master/install/setup.bash
    python3 ros2_master/tools/dump_motion_params.py [-o 输出路径]
"""

import argparse
import datetime
import json
import sys
from pathlib import Path

import rclpy
from rclpy.node import Node

from motion_settings_service.srv import MotionSettingsService

SERVICE_NAME = "/esp32/motion_settings_service"

# MotionService::Type 中的指令值（见 common/enum.h，仅取所需）
READ_PARAMS = 19
READ_CONFIG = 22

# 仓库根目录：本文件位于 <repo>/ros2_master/tools/
REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = REPO_ROOT / "ESP32_motion" / "config" / "default_motion.json"

MOTORS = ("left_front", "left_back", "right_front", "right_back")
PID_LOOPS = ("position", "line_speed", "angle_speed")


def call_service(node, client, mode):
    request = MotionSettingsService.Request()
    request.mode = mode
    request.id = mode

    future = client.call_async(request)
    rclpy.spin_until_future_complete(node, future)
    if future.result() is None:
        raise RuntimeError(f"服务调用失败 (mode={mode})")
    return future.result()


def motor_entry(response, prefix):
    return {
        "motor_pina": int(getattr(response, f"{prefix}_motor_pina")),
        "motor_pinb": int(getattr(response, f"{prefix}_motor_pinb")),
        "motor_pinpwm": int(getattr(response, f"{prefix}_motor_pinpwm")),
        "encoder_pina": int(getattr(response, f"{prefix}_encoder_pina")),
        "encoder_pinb": int(getattr(response, f"{prefix}_encoder_pinb")),
        "wheel_diameter": float(getattr(response, f"{prefix}_motor_wheel_diameter")),
        "pluses_per_revolution": int(getattr(response, f"{prefix}_motor_pluses_per_revolution")),
        "revolutions_per_minute": int(getattr(response, f"{prefix}_motor_revolutions_per_minute")),
        "pid": {
            "p": float(getattr(response, f"{prefix}_motor_p")),
            "i": float(getattr(response, f"{prefix}_motor_i")),
            "d": float(getattr(response, f"{prefix}_motor_d")),
            "max_total_integral": float(getattr(response, f"{prefix}_motor_max_total_integral")),
        },
    }


def pid_entry(response, prefix):
    return {
        "p": float(getattr(response, f"{prefix}_p")),
        "i": float(getattr(response, f"{prefix}_i")),
        "d": float(getattr(response, f"{prefix}_d")),
        "max_total_integral": float(getattr(response, f"{prefix}_max_total_integral")),
    }


def build_document(params, config, comtype):
    return {
        "version": 1,
        "comtype": comtype,
        "source": {
            "service": SERVICE_NAME,
            "timestamp": datetime.datetime.now().isoformat(timespec="seconds"),
        },
        "motion": {
            "is_mecanum_wheel": bool(config.is_mecanum_wheel),
            "wheel_width": float(config.wheel_width),
            "track_width": float(config.track_width),
            "milliseconds": int(params.milliseconds),
            "position_loop_milliseconds_cnt": int(params.position_loop_milliseconds_cnt),
            "speed_loop_milliseconds_cnt": int(params.speed_loop_milliseconds_cnt),
            "enable_speed_plan": bool(params.enable_speed_plan),
            "motor_enable_flags": int(params.motor_enable_flags),
            "max_v": float(params.max_v),
            "max_acc": float(params.max_acc),
            "jerk": float(params.jerk),
        },
        "pid_loops": {name: pid_entry(params, name) for name in PID_LOOPS},
        "motors": {name: motor_entry(config, name) for name in MOTORS},
    }


def main():
    parser = argparse.ArgumentParser(description="导出 ESP32 运动参数/配置为 JSON")
    parser.add_argument("-o", "--output", default=str(DEFAULT_OUTPUT),
                        help=f"输出 JSON 路径（默认 {DEFAULT_OUTPUT}）")
    parser.add_argument("--comtype", choices=("wifi", "bluetooth", "ps3"), default="wifi",
                        help="默认通信模式（NVS 无 comtype 时使用，默认 wifi）")
    args = parser.parse_args()

    rclpy.init()
    node = Node("dump_motion_params")
    client = node.create_client(MotionSettingsService, SERVICE_NAME)

    try:
        if not client.wait_for_service(timeout_sec=5.0):
            print(f"[错误] 服务不可用: {SERVICE_NAME}", file=sys.stderr)
            print("       请确认 ESP32 已在 WiFi 模式、micro_ros_agent 正在 8888 端口运行。",
                  file=sys.stderr)
            return 1

        params = call_service(node, client, READ_PARAMS)
        config = call_service(node, client, READ_CONFIG)
        document = build_document(params, config, args.comtype)
    finally:
        node.destroy_node()
        rclpy.shutdown()

    text = json.dumps(document, ensure_ascii=False, indent=2)
    print(text)

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text + "\n", encoding="utf-8")
    print(f"\n[完成] 已写入 {output}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
