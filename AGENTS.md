# AGENTS.md

RC mecanum-wheel car. Three independently-built parts (ESP32 firmware, ROS 2 host, Android app) share one `common/` header set.

## Layout
- `ESP32_motion/` — ESP32-WROOM firmware (PlatformIO + Arduino). Motor/PID/encoder control. `src/main.cpp` is the only source; boot picks one of three comm modes from NVS namespace `comtype`: WiFi micro-ROS, Bluetooth SPP (`bluetooth_slave/`), or PS3-style gamepad (`ps3_host/`).
- `ros2_master/` — ROS 2 Humble workspace (`colcon`). `control_panel` (Qt5 GUI, C++14) plus `motion_status_msgs`, `motion_settings_service`, `nodes_launch_pkg`, and submodule packages.
- `Android_app/` — Qt 6.5.3 Android app `RC_Controller` (BLE client + legacy HTTP client).
- `doc/`, `micro_ros/`, `common/` — docs/models, vendor micro-ROS tooling, shared headers.

## Shared `common/` (edit here only)
`common/settings.h|enum.h|tools.h` is symlinked into `ESP32_motion/lib/common`, `ros2_master/src/common`, `ros2_master/src/control_panel/src/common`, `Android_app/src/common`, and `nodes_launch_pkg/common`. It holds ROS node/topic/service names, WiFi credentials, the agent IP/port (8888), and `MotionService::Type` — the command enum used by both firmware and host. Changing it affects all three builds.

## Build / run
- ROS 2 (`ros2` + `colcon` are on PATH; `source /opt/ros/humble/setup.bash` first): from `ros2_master/` run `colcon build`, then `source install/setup.bash`. Full stack: `ros2 launch nodes_launch_pkg nodes_run.launch.py` (micro_ros_agent `udp4 --port 8888` + `control_panel`). For a subset use `colcon build --packages-up-to control_panel` (builds `control_panel` and its interface deps).
- Tests: there are no unit tests, only ament lint tests. `colcon test [--packages-select <pkg>]` then `colcon test-result --verbose`. No CI.
- Firmware: PlatformIO env `esp32_wroom`, `monitor_speed=115200`. `pio` is not on PATH here — normally built/flashed via the VSCode PlatformIO extension from `ESP32_motion/`.
- Android: built through Qt Creator with the Qt 6.5.3 `android_arm64_v8a` kit; no CLI build script. `Android_app/CMakeLists.txt` hardcodes `CMAKE_PREFIX_PATH=/opt/Qt/6.5.3/android_arm64_v8a` and globs `src/*` recursively, so new files are auto-included.

## Custom msgs / srv regeneration (important)
Source of truth: `ros2_master/src/motion_status_msgs/msg/MotionStatus.msg` and `ros2_master/src/motion_settings_service/srv/MotionSettingsService.srv`. Firmware links generated C sources/libs under `ESP32_motion/lib/motion_status_msgs/` and `.../motion_settings_service/`.
- The generated C sources/headers are committed; the `lib/*.a` match the root `.gitignore` `*.a` rule and are not, so a fresh clone cannot link the firmware until they are regenerated.
- The firmware `.a` must be cross-compiled for the ESP32 via the micro-ROS firmware-workspace procedure, not host `colcon build`. Procedure: `micro_ros/README.md`; its paths are stale: `ros2_code` -> `ros2_master/src`, `ESP32_code` -> `ESP32_motion`, `motion_params_service` -> `motion_settings_service`.
- After editing a `.msg`/`.srv`, regenerate the firmware copy too — the ROS 2 and firmware copies drift silently.

## Submodules
`ESP32Encoder` and `micro_ros_platformio` (under `ESP32_motion/lib/`), `micro_ros_setup` (under `micro_ros/`), `micro-ROS-Agent` and `micro_ros_msgs` (under `ros2_master/src/`). Initialize with `git submodule update --init --recursive`.

## Conventions and gotchas
- READMEs and code comments are Chinese; keep new comments/docs consistent.
- Comm mode is runtime-switched via the `SwitchToBluetooth` / `SwitchToPS3` services, which write NVS and reboot (`ESP32_motion/lib/system/system.h`). In PS3 mode the gamepad's "pairing code" is the target ESP32 MAC, passed to `Ps3.begin()`; holding `Select+Start` 2s switches back to WiFi.
- PS3 mode uses the emakefun fork `emakefun_esp32_ps3#v2.0.1` (pinned in `platformio.ini`), NOT upstream `jvpernis/esp32-ps3`; it needs Arduino-ESP32 core 2.0.17–3.3.6 (3.3.7 unsupported), hence `platform = espressif32@6.13.0` (core 2.0.17). It uses Classic BT, so it coexists with SPP only because modes are mutually exclusive and reboot on switch.
- `settings.h` `wifi_IP` is the micro-ROS agent host. `Android_app/src/http_client/httpClient.h` hardcodes a different ESP32 IP, but the firmware has no HTTP server — treat that client as unused.
- Firmware uses `huge_app.csv` partitions and PSRAM flags in `platformio.ini`; unrelated partition/PSRAM edits can break flashing.
- `ros2_master/{build,install,log}/` and `Android_app/build/` exist locally but are gitignored; never commit them.
