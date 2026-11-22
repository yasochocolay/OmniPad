# Changelog

All notable changes to OmniPad are documented in this file.

## [v1.0.0-beta2]

- Switch Pro Bluetooth mode is now allowed on **FW 13.00**. On 13.00 the Bluetooth allowlist entry is found by
  the dynamic scanner (the same code path the DualSense mode already uses there). The DualSense extended
  (78-byte) mode stays disabled on 13.00.

## [v1.0.0-beta1] - First release

### New
- **Nintendo Switch Pro Controller over Bluetooth (experimental, opt-in).** Enabled only when the empty file
  `/data/omnipad/switchpro_bt.enable` exists, and only on FW 12.50, 13.02, 13.04, 13.50 and 13.52.
  The controller is read in its default 12-byte report `0x3F` mode: buttons, sticks and D-Pad.
  No output report is ever sent to the controller (no rumble, LEDs or gyro yet).
  While enabled, DualSense Bluetooth is disabled (the kernel allowlist entry holds one identity at a time).
- Kernel allowlist patch now writes a validated identity (DualSense `054C:0CE6` or Switch Pro `057E:2009` only).
- New OmniPad icon, home background and in-app artwork (no third-party logos). Sources in `art/`.
- `build_mac.sh`: macOS / Linux build script (clang/lld 21.1.8 + OpenOrbis toolchain).

### Companion App
- Redesigned interface in the OmniPad colours (left menu kept, cleaner cards and value selectors,
  right-aligned button hints, header no longer overlaps the Input Tester / Remap screens).
- New **Bluetooth** tab: Switch Pro mode on/off (no FTP needed), firmware check, mode actually running
  in the daemon with a "restart needed" warning, connected Bluetooth controllers, pairing guide.
- New **Remap Bluetooth** tab: per-button remap for the Switch Pro and the DualSense over Bluetooth
  (18 buttons, disable option), saved in the daemon's `src = dst` format and applied live.
- New **Log** tab: reads `/data/omnipad/omnipad.log`, highlights warnings/errors, copies it to a USB drive.
- **English / French** interface (Auto follows the PS4 system language); UTF-8 text rendering.
- Desktop UI preview (`tools/ui_preview`) and desktop tests (`tests/run.sh`).

### Daemon
- Writes `/data/omnipad/omnipad.btmode` (active Bluetooth mode) for the Companion App.
- Applies Bluetooth remap rules to the Switch Pro (`057E_2009.ini`).

### Safety
- DualSense Bluetooth link-capacity kernel patch is now restricted to FW 12.50 - 13.52, where its offsets are verified.
  On other firmwares the DualSense stays in the safe 10-byte report mode.

### Notes
- Not yet tested on real hardware for the Switch Pro Bluetooth mode. Please report results with
  `/data/omnipad/omnipad.log`.
