# 项目简介

本项目用于制作遥控麦克纳姆轮小车：

- **下位机**：ESP32-WROOM，基于 Arduino 框架、使用 VSCode + PlatformIO 开发，负责电机/编码器、PID 闭环、速度规划与通信。
- **上位机**：基于 Qt5 + ROS 2 (Humble) 的调试软件（micro-ROS 通信）。
- **安卓端**：基于 Qt 6.5.3 + QBluetooth 的遥控 App。

小车支持三种控制方式，可在运行时切换：

| 控制方式 | 控制端 | 进入方式 | 退出方式 |
| --- | --- | --- | --- |
| WiFi（micro-ROS） | ROS 2 上位机 | 默认模式 | 服务 `SwitchToBluetooth` / `SwitchToPS3` |
| 蓝牙（SPP） | 安卓 App（`esp32_bluetooth_slave`） | 上位机服务 / PS3 手柄组合键 | App 指示器单击切 PS3、双击切 WiFi |
| PS3 蓝牙手柄 | emakefun ESP32 专用手柄 | 服务 `SwitchToPS3`（mode 25）/ App 单击 | 手柄 `Select+Start` 长按 2 秒 → 蓝牙 |

> 说明：模式保存在 NVS 命名空间 `comtype`，切换即写入并重启。PS3 手柄背面的“配对码”即目标 ESP32 的蓝牙 MAC，可在 `common/settings.h` 的 `ps3_bluetooth_mac` 配置。

# 功能特性

- 麦克纳姆轮全向移动：前后、左右横移、四向斜行、原地旋转
- 四路编码器 + PID 闭环控制，支持速度规划（jerk 限制）
- 三种控制方式与运行时切换
- 参数/配置持久化到 NVS，支持 `ESP32_motion/config/default_motion.json` 作为默认值（NVS 未保存时使用）
- ROS 2 上位机：实时状态显示、参数/配置读写、模式切换
- 安卓 App：虚拟摇杆遥控

# 项目结构

| 目录 | 说明 |
| --- | --- |
| `ESP32_motion/` | ESP32 固件（PlatformIO + Arduino） |
| `ros2_master/` | ROS 2 Humble 工作空间（`control_panel` 上位机 + 自定义接口） |
| `Android_app/` | Qt 6.5.3 安卓 App（`RC_Controller`） |
| `common/` | 三端共享头文件（ROS 名称、WiFi/端口、指令枚举），软链接到各工程 |
| `doc/` | 文档、三维模型、电路设计、图片 |
| `micro_ros/` | micro-ROS 固件工具链与自定义消息编译说明 |

# 开发环境

- 操作系统：Ubuntu 22.04
- ROS 2：Humble
- 上位机 Qt：5.x
- 安卓 Qt：6.5.3
- 下位机：VSCode + PlatformIO 插件（Arduino 框架）
- 三维建模：FreeCAD
- 电路设计：嘉立创EDA专业版

# 快速开始

- **固件**：VSCode 打开 `ESP32_motion/`，PlatformIO 环境 `esp32_wroom`，编译/烧录。
- **上位机**：`cd ros2_master && colcon build && source install/setup.bash && ros2 launch nodes_launch_pkg nodes_run.launch.py`。
- **安卓**：Qt Creator 使用 Qt 6.5.3 `android_arm64_v8a` kit 构建。

更详细的构建、运行与注意事项见 [`AGENTS.md`](AGENTS.md)。

# 实物展示

## 小车
![小车](doc/images/麦克纳姆轮小车.jpg)

## 演示动画
![演示动画](doc/images/动画.gif)

## 电路设计和三维建模
![电路设计](doc/images/麦克纳姆轮小车模块.jpg)

## 安卓软件
![安卓软件](doc/images/麦克纳姆轮小车遥控软件.jpg)

## 上位机软件
![上位机软件](doc/images/上位机软件.png)
