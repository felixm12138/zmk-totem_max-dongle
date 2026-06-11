# Totem Max ZMK

ZMK user config for Totem Max using a Prospector display dongle as the split
central. The source of truth for the matrix and base keymap is
`felixm12138/totem_max_rmk`.

## Targets

- `totem_max_left`: nice!nano-compatible left half, BLE split peripheral.
- `totem_max_right`: nice!nano-compatible right half, BLE split peripheral.
- `totem_max_dongle_prospector`: Prospector/XIAO BLE dongle, split central,
  ZMK Studio over USB, and Prospector status screen.
- `settings_reset_nice_nano` and `settings_reset_xiao_ble`: reset firmware for
  pairing recovery.

## Flashing

1. Flash `settings_reset_nice_nano` to both keyboard halves.
2. Flash `settings_reset_xiao_ble` to the Prospector dongle.
3. Flash `totem_max_left` to the left half and `totem_max_right` to the right
   half.
4. Flash `totem_max_dongle_prospector` to the Prospector.
5. Pair the left half first, then the right half. Prospector displays
   peripheral battery/connection widgets in pairing order.

The dongle is the only USB/BLE HID central. The left and right halves are built
as split peripherals and do not expose USB HID to the host.

## ZMK Studio

The dongle build uses the `studio-rpc-usb-uart` snippet and enables
`CONFIG_ZMK_STUDIO`. Studio locking is disabled so the RMK-equivalent keymap
does not need an extra unlock key.

## Local build

```sh
west init -l config
west update --fetch-opt=--filter=tree:0
west zephyr-export

west build -s zmk/app -d build/left -b nice_nano//zmk -- \
  -DSHIELD=totem_max_left -DZMK_CONFIG=$PWD/config

west build -s zmk/app -d build/right -b nice_nano//zmk -- \
  -DSHIELD=totem_max_right -DZMK_CONFIG=$PWD/config

west build -s zmk/app -d build/dongle -b xiao_ble//zmk -S studio-rpc-usb-uart -- \
  -DSHIELD="totem_max_dongle prospector_adapter" \
  -DZMK_CONFIG=$PWD/config -DCONFIG_ZMK_STUDIO=y
```
