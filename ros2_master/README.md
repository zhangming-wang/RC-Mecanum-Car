# ROS 2 上位机工作空间

本工作空间用于小车调试的上位机软件，包含 Qt5 图形界面（control_panel）、micro-ROS Agent，以及固件与上位机共用的自定义消息/服务。

## 环境要求
- Ubuntu 22.04
- ROS 2 Humble（`source /opt/ros/humble/setup.bash`）
- Qt5，需包含 Widgets / Charts / PrintSupport / Gamepad 组件

## 获取依赖（Git 子模块）
micro-ROS-Agent 与 micro_ros_msgs 以子模块形式管理，在仓库根目录执行：
```bash
git submodule update --init --recursive
```

## 编译
```bash
source /opt/ros/humble/setup.bash
cd ros2_master
colcon build
source install/setup.bash
```

只编译部分功能包（含依赖）：
```bash
colcon build --packages-up-to control_panel
```

## 运行
```bash
source /opt/ros/humble/setup.bash
source ros2_master/install/setup.bash
ros2 launch nodes_launch_pkg nodes_run.launch.py
```

该 launch 会启动：
- `micro_ros_agent`（`udp4`，监听 8888 端口，与 ESP32 通信）
- `control_panel`（Qt5 调试界面）

## 功能包说明
| 功能包 | 类型 | 说明 |
| --- | --- | --- |
| control_panel | ament_cmake (Qt5) | 上位机调试界面 |
| motion_status_msgs | 接口包 | 运动状态消息 `MotionStatus` |
| motion_settings_service | 接口包 | 运动参数/配置服务 `MotionSettingsService` |
| nodes_launch_pkg | ament_python | 启动文件 |
| micro-ROS-Agent | 子模块 | micro-ROS 代理 |
| micro_ros_msgs | 子模块 | micro-ROS 依赖消息 |

## 测试
```bash
colcon test
colcon test-result --verbose
```
目前仅有 ament lint 测试，无单元测试。

## 工具
### tools/dump_motion_params.py
通过 `/esp32/motion_settings_service` 分别调用 ReadParams 与 ReadConfig，导出 ESP32 运动参数/配置为 JSON（默认写入 `ESP32_motion/config/default_motion.json`）。

用法（需 ESP32 处于 WiFi 模式且 `micro_ros_agent` 已在 8888 端口运行）：
```bash
source /opt/ros/humble/setup.bash
source ros2_master/install/setup.bash
python3 ros2_master/tools/dump_motion_params.py [-o 输出路径] [--comtype wifi|bluetooth|ps3]
```
