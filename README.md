# Totem Max — ZMK Firmware (with Prospector Dongle)

ZMK 固件仓库 for **Totem Max** 分体键盘 + **Prospector** 加密狗。

| 组件 | 主控 | 角色 |
|------|------|------|
| 左半 | nRF52840 (Nice!Nano v2) | BLE Peripheral |
| 右半 | nRF52840 (Nice!Nano v2) | BLE Peripheral |
| Prospector 加密狗 | Seeed Studio XIAO nRF52840 BLE | BLE Central + LCD 屏幕 |

## 架构

```
┌──────────────────────┐     BLE      ┌──────────────────────┐
│  Totem Max 左半      │◄────────────►│  Prospector (中央)   │
│  (Peripheral)        │              │  · Central 角色      │
│                      │              │  · LCD 显示          │
│                      │     BLE      │  · ZMK Studio        │
│  Totem Max 右半      │◄────────────►│                      │
│  (Peripheral)        │              └──────────────────────┘
└──────────────────────┘
```

Prospector 作为 **BLE 中央设备**，同时连接左右两个分体，在屏幕上显示当前层、电池电量、连接状态等信息。

## GitHub Actions 编译

1. Fork 本仓库，进入 **Actions** → **Build ZMK Firmware** → 手动运行
2. 编译完成后下载 3 个 `.uf2` 文件：

| Artifact | 刷入设备 |
|----------|---------|
| `totem_max_left_nice_nano_v2.uf2` | 左半 Nice!Nano v2 |
| `totem_max_right_nice_nano_v2.uf2` | 右半 Nice!Nano v2 |
| `totem_max_dongle_seeeduino_xiao_ble.uf2` | Prospector (XIAO nRF52840 BLE) |

### 配对顺序

刷写完成后，按以下顺序配对：

1. **先刷写左右半**和**加密狗**的所有固件
2. **加密狗上电**（Prospector 会进入等待配对状态）
3. **左半上电** → 自动配对到加密狗
4. **右半上电** → 自动配对到加密狗
5. 如果连接失败，需要按 `&bt BT_CLR` 清除配对记录后重试

> ⚠️ 配对顺序：**先左后右**，Prospector 的电池电量显示依赖于配对的顺序。

## 本地编译

```bash
# 初始化
west init -l config
west update
west zephyr-export

# 编译左侧
west build -b nice_nano_v2 -s zmk/app -d build/left -- \
  -DSHIELD=totem_max_left -DZMK_CONFIG="${PWD}/config"

# 编译右侧
west build -b nice_nano_v2 -s zmk/app -d build/right -- \
  -DSHIELD=totem_max_right -DZMK_CONFIG="${PWD}/config"

# 编译 Prospector 加密狗（多 shield 合并）
west build -b seeeduino_xiao_ble -s zmk/app -d build/dongle -- \
  -DSHIELD="totem_max_dongle prospector_adapter" \
  -DZMK_CONFIG="${PWD}/config"
```

## 层分布

| 层 | 名称 | 用途 |
|----|------|------|
| 0 | Base | QWERTY 主键位层 |
| 1-5 | L1-L5 | 透明层（通过 ZMK Studio 自定义） |

Prospector LCD 屏幕会显示当前激活层的名称（"Base" / "L1" / "L2" 等）。

## ZMK Studio

本固件支持 **ZMK Studio**，可通过 [studio.zmk.dev](https://studio.zmk.dev) 在浏览器中实时修改键位映射。加密狗需要连接 USB 后使用 Studio。
