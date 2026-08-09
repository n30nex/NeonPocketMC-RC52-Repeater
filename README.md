# NeonPocketMC-RC52-Repeater

[![RC52 Repeater Build](https://github.com/n30nex/NeonPocketMC-RC52-Repeater/actions/workflows/rc52-repeater-build.yml/badge.svg)](https://github.com/n30nex/NeonPocketMC-RC52-Repeater/actions/workflows/rc52-repeater-build.yml)

Experimental, headless MeshCore repeater firmware for the Heltec RadioCore RC52-L62. It is based on MeshCore repeater `v1.17.0` at upstream commit [`727fc0512ce08bfd7b499e46daa7fca6eeec730d`](https://github.com/meshcore-dev/MeshCore/commit/727fc0512ce08bfd7b499e46daa7fca6eeec730d).

> **RC52-L62 only. Do not flash this on RCC6, RC32, T114, or another RC52 radio/display variant.**

## What this build does

- Runs the standard MeshCore repeater on the RC52 nRF52840 and SX1262.
- Leaves the attached TFT powered off; there is no display, framebuffer, BLE, Web, or USB companion stack.
- Enables nRF event sleep by default while keeping LoRa receive active.
- Powers down the TFT, radio/FEM controls, and enters nRF52 System OFF after a 1.5-second user-button hold. Press the user button to wake.
- Mounts internal storage fail-closed so a mount error does not silently replace the saved identity or preferences.
- Keeps automatic low-voltage shutdown disabled until RC52 battery ADC calibration is physically verified.

This is low-overhead repeater firmware, not a deep-sleep sensor. Continuous LoRa receive remains the main energy cost.

## Solar power boundary

The RC52 does **not** contain an MPPT controller or a charger suitable for an unregulated solar panel. Its onboard LGS4056HDA is a one-cell linear charger supplied from regulated 5 V/USB.

Use this power chain:

```text
solar panel -> external 1S Li-ion MPPT charger with protection/load sharing
            -> protected 1S battery -> RC52 VBAT + GND
```

Never connect an unregulated panel directly to RC52 `VBAT`, `5V`, or USB. See [Solar and battery power](docs/POWER.md).

## Install

1. Download `NeonPocketMC-RC52-Repeater.uf2` from the latest release.
2. Double-press reset to mount the RC52 UF2 bootloader drive.
3. Copy the UF2 to that drive and wait for the device to reboot.
4. Open its USB serial console at 115200 baud.
5. Immediately change the default admin password, set the repeater name, and configure the legal regional radio parameters.

The release UF2 starts at application address `0x26000`. It does not replace the bootloader or SoftDevice. Full instructions: [Flashing](docs/FLASHING.md).

Useful first-run commands:

```text
set name <your repeater name>
password <a strong password>
set freq <legal regional frequency>
powersaving on
```

Use `help` and the upstream [MeshCore CLI reference](https://docs.meshcore.io/cli_commands) for the complete radio/preset setup. Do not transmit until the frequency, bandwidth, spreading factor, coding rate, and local regulations are correct.

## Hardware mapping

| Function | RC52 pin |
|---|---:|
| SX1262 CS / DIO1 / BUSY / RESET / RXEN | 13 / 11 / 24 / 32 / 39 |
| SX1262 SCK / MISO / MOSI | 25 / 14 / 22 |
| FEM / VFEM controls | 26 / 16 |
| User button | 42, active-low |
| Battery sense control / ADC | 4 / 31 |
| TFT power / backlight | 45 active-low / 9 active-high |

The radio uses DIO2 RF switching, a 1.8 V DIO3 TCXO, DC-DC mode, and boosted RX gain.

## Build

GitHub Actions builds the exact `heltec_rc52_repeater` environment, runs the upstream native tests, verifies the UF2 family/address, and regression-builds the existing headless T114 repeater target. Release binaries are produced by Actions rather than a developer workstation.

## Status

`v1.0.0-rc.1` is an experimental first release. The RC52 BLE companion firmware is a separate repository and image; do not interchange them.

## License

This repository follows the upstream MeshCore LGPL-2.1 license. See [license.txt](license.txt). Hardware pin mapping and board support are derived from Heltec's published RC52 materials and compatible open-source board definitions.
