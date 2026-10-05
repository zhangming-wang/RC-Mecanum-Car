# ESP32 下位机固件

ESP32-WROOM 麦克纳姆轮小车下位机固件，基于 **PlatformIO + Arduino** 框架开发：

- 四路电机（PWM）+ 四路编码器
- 电机速度 PID 闭环 + 速度规划（jerk 限制）
- 麦克纳姆轮全向运动学（前后 / 横移 / 斜行 / 原地旋转）
- 三种通信方式：WiFi（micro-ROS）、蓝牙（SPP）、PS3 蓝牙手柄

## 目录结构

| 路径 | 说明 |
| --- | --- |
| `src/main.cpp` | 唯一入口，按 NVS 命名空间 `comtype` 选择通信模式并启动对应任务 |
| `lib/motion_control/` | 运动总控：运动学、队列、速度规划、参数/配置读写 |
| `lib/motor_control/` | 单电机 + 编码器闭环（`motor` / `encoder` / `motorControl`） |
| `lib/pid_control/` | PID 控制器 |
| `lib/pwm_control/` | PWM 输出 |
| `lib/speed_plan/` | 加加速度（jerk）限制的速度规划 |
| `lib/motion_node/` | micro-ROS 节点：订阅 `cmd_vel`、发布状态、提供参数服务 |
| `lib/bluetooth_slave/` | 手机 App 蓝牙 SPP 从机 |
| `lib/ps3_host/` | PS3 蓝牙手柄主机 |
| `lib/system/` | NVS、重启、通信模式切换；生成的默认模式头 |
| `lib/motion_status_msgs/`、`lib/motion_settings_service/` | 由 ROS 2 端生成的自定义消息/服务（C 源码 + 静态库） |
| `lib/micro_ros_platformio/`、`lib/ESP32Encoder/` | 第三方库（git 子模块） |
| `lib/common/` | 指向仓库 `common/` 的软链接（协议、WiFi、指令枚举） |
| `config/default_motion.json` | 默认参数/配置（含默认通信模式 `comtype`） |
| `scripts/gen_defaults.py` | 编译前由 JSON 生成默认值头文件 |

## 通信模式与切换

模式保存在 NVS 命名空间 `comtype`（键 `type`），切换即写入并重启：

- **WiFi**：micro-ROS，与 ROS 2 上位机（`micro_ros_agent udp4 --port 8888`）通信（默认模式）。
- **蓝牙（SPP）**：广播 `esp32_bluetooth_slave`，手机 App 连接；App 指示器单击切 PS3、双击切 WiFi。
- **PS3**：emakefun ESP32 专用蓝牙手柄；手柄 `Select+Start` 长按 2 秒切回蓝牙。

进入/退出：上位机可通过服务 `SwitchToBluetooth`（mode 2）/ `SwitchToPS3`（mode 25）切换。PS3 手柄背面的“配对码”即目标 ESP32 蓝牙 MAC，在 `common/settings.h` 的 `ps3_bluetooth_mac` 配置。

## 依赖与第三方库

子模块（首次需初始化）：

```
git submodule update --init --recursive
```

- `lib/micro_ros_platformio`：https://github.com/micro-ROS/micro_ros_platformio.git
- `lib/ESP32Encoder`：https://github.com/madhephaestus/ESP32Encoder.git

`platformio.ini` 的 `lib_deps`：

- `emakefun_esp32_ps3#v2.0.1`：https://github.com/emakefun-arduino-library/emakefun_esp32_ps3.git
  （官方指定库，为 `jvpernis/esp32-ps3` 的兼容性 fork；切勿同时安装旧的 “PS3 Controller Host”。）

自定义消息/服务 `lib/motion_status_msgs`、`lib/motion_settings_service` 由 ROS 2 端的 `.msg`/`.srv` 生成；更新方法见 [`../micro_ros/README.md`](../micro_ros/README.md)。注意其中路径名已过时：`ros2_code`→`ros2_master/src`、`ESP32_code`→`ESP32_motion`、`motion_params_service`→`motion_settings_service`。生成的头文件已提交，但 `lib/*.a` 受根 `.gitignore` 的 `*.a` 规则影响不会提交，新克隆需重新生成才能链接。

## 默认参数配置

`config/default_motion.json` 保存默认参数/配置与默认通信模式。编译前 `extra_scripts`（`scripts/gen_defaults.py`）会生成：

- `lib/motion_control/defaults_generated.h`：`apply_generated_defaults(MotionControl&)`
- `lib/system/default_mode_generated.h`：`DEFAULT_COMMUNICATION_TYPE`

开机先套用 JSON 默认值，随后从 NVS 读取：**NVS 有保存值则覆盖，没有则用 JSON 默认**。修改 JSON 后重新编译即生效。

## 构建与烧录

- PlatformIO 环境：`esp32_wroom`；串口监视器波特率 `115200`。
- VSCode：用 PlatformIO 插件编译/烧录；或命令行：

```
pio run -e esp32_wroom            # 编译
pio run -e esp32_wroom -t upload  # 烧录
pio run -e esp32_wroom -t erase   # 擦除整片（含 NVS，可用于恢复 JSON 默认值）
```

- 说明：
  - 使用 `huge_app.csv` 分区与 PSRAM 编译标志；改动分区/PSRAM 相关配置可能影响烧录。
  - CP2102N 在 460800 波特率下可能掉流（`Serial data stream stopped`），必要时在 `platformio.ini` 打开 `upload_speed = 115200`。
  - 若无法自动进入下载模式：按住 **BOOT** 再点按 **RST**，然后松开 BOOT。

## 兼容性与注意事项

- Arduino-ESP32 内核要求 **2.0.17 ~ 3.3.6**，**3.3.7 不支持**，因此 `platform = espressif32@6.13.0`（core 2.0.17）。不要切换到 pioarduino 或更高内核。
- PS3 手柄切勿再与手机/电脑/其它蓝牙设备配对，也不要当作 USB 手柄使用，否则其内部配对码会被覆盖失效。
- `common/` 为三端（固件 / 上位机 / 安卓）共享头文件，统一在仓库根 `common/` 修改。
