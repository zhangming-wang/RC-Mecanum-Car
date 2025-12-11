# Android_BT_Controller (Qt5 Widgets + Classic Bluetooth)

一个使用 Qt5 开发的安卓 App，提供虚拟摇杆界面，通过经典蓝牙（RFCOMM/SPP）向 ESP32 下发控制命令。

- 协议（简版）：`x,y,speed\n`，其中 `x,y` ∈ [-100,100]，`speed` ∈ [0,100]
- 连接：扫描经典蓝牙设备，选择目标（如 `ESP32-XXXX`），通过 SPP 连接

## 依赖
- Qt 5.15（含模块：Core、Gui、Widgets、Bluetooth、AndroidExtras）
- Android SDK/NDK（建议用 Qt Creator 的 Android Kit）

## 构建（推荐：Qt Creator）
1. 打开本目录 `Android_BT_Controller`。
2. 选择 Android Kit（如 `Android Qt 5.15.2 Clang arm64-v8a`）。
3. 构建并部署到设备。

Qt Creator 会自动调用 `androiddeployqt` 进行打包，`android/AndroidManifest.xml` 已包含运行所需权限。

## 命令行（高级用户）
若需命令行构建，请根据本机环境设置变量并替换路径：

```bash
# 示例变量（请按实际环境调整）
export ANDROID_SDK_ROOT=~/Android/Sdk
export ANDROID_NDK_ROOT=~/Android/Sdk/ndk/25.2.9519653
export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
export QT_DIR=~/Qt/5.15.2/android

cmake -S . -B build-android \
  -D CMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -D ANDROID_ABI=arm64-v8a \
  -D ANDROID_PLATFORM=android-23 \
  -D CMAKE_PREFIX_PATH=$QT_DIR

cmake --build build-android
# 随后请用 androiddeployqt 或 Qt Creator 完成打包安装
```

> 注：不同环境变量和路径可能有所差异，建议优先使用 Qt Creator 的 Android Kit。

## ESP32 端建议
- 开启经典蓝牙 SPP 服务（UUID: `00001101-0000-1000-8000-00805F9B34FB`）。
- 接收形如 `x,y,speed\n` 的 ASCII 文本，按需解析为速度与方向。

## 运行权限
- Android 12+ 需要 `BLUETOOTH_SCAN`、`BLUETOOTH_CONNECT` 运行时权限。
- Android <12 需要 `BLUETOOTH`、`BLUETOOTH_ADMIN`、定位权限用于扫描。
- 程序启动时会申请相应权限；如被拒，可在系统设置中手动开启。

## 目录结构
- `src/`：UI、蓝牙和摇杆代码
- `android/AndroidManifest.xml`：安卓打包权限与入口
- `CMakeLists.txt`：CMake 构建脚本
