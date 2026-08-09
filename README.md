<p align="center">
  <img src="https://raw.githubusercontent.com/n30nex/NeonPocketMC/main/branding/neonpocketmc-mark.png" alt="NeonPocketMC pocket mesh logo" width="140">
</p>

# NeonPocketMC-RC52-Repeater

[![RC52 Repeater Build](https://github.com/n30nex/NeonPocketMC-RC52-Repeater/actions/workflows/rc52-repeater-build.yml/badge.svg)](https://github.com/n30nex/NeonPocketMC-RC52-Repeater/actions/workflows/rc52-repeater-build.yml)

Experimental MeshCore repeater and Room Server firmware for the Heltec RadioCore RC52-L62. It is based on MeshCore `v1.17.0` at upstream commit [`727fc0512ce08bfd7b499e46daa7fca6eeec730d`](https://github.com/meshcore-dev/MeshCore/commit/727fc0512ce08bfd7b499e46daa7fca6eeec730d).

> **RC52-L62 only. Do not flash this on RCC6, RC32, T114, or another RC52 radio/display variant.**

## Choose a build

| PlatformIO environment | Purpose | TFT |
|---|---|---:|
| `heltec_rc52_repeater` | Low-overhead LoRa repeater | Off |
| `heltec_rc52_room_server_headless` | MeshCore Room Server for outdoor/headless use | Off |
| `heltec_rc52_room_server_tft` | Room Server with native 220x128 NeonPocket dashboard | On |

The Room Server profiles keep 32 buffered posts and expose the standard MeshCore room/client protocol. The TFT profile adds a required 56,320-byte framebuffer, 8-row delta flushing, animated NeonPocketMC startup, Home/RF/Clients/Posts/Power pages, a 60-second screen timeout, and a 16 KB post-display memory gate. Both Room Server profiles use the user button for next/wake and two-hold System OFF confirmation.

## Repeater profile

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

## Room Server setup

Room Server users should configure the unit while USB and a tuned antenna are still attached. The guided setup sets the room name, a legal regional radio preset, TX power, **3-byte advert hashes**, and new admin/guest passwords; it does not retain or print the passwords.

- Windows: extract the release package and double-click `Configure-RC52-Room-Windows.cmd`.
- Linux: extract it and run `sh configure-rc52-room-linux.sh`.
- Advanced/manual setup: open a 115200-baud USB terminal and use `set name`, `set radio`, `set tx`, `set path.hash.mode 2`, `password`, and `set guest.password`.

The compile-time `password` / `hello` values are onboarding defaults only. **Do not deploy a room server until the wizard has replaced both.** Keep the resulting credentials private. The TFT reports raw battery millivolts, warns at or below 3.45 V, and clears the warning at or above 3.60 V; it does not claim a calibrated percentage and does not automatically shut down on low voltage.

## Repeater install

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

GitHub Actions builds the exact repeater plus both Room Server environments, verifies each room UF2 family and `0x26000` application address, runs the configurator self-test, and packages the Windows/Linux setup helper. The existing T114 regression remains in the repeater workflow. Release binaries are produced by Actions rather than a developer workstation.

## Status

`v1.0.0-rc.1` is the original experimental repeater release. Room Server artifacts are newer prerelease candidates until exact Actions builds and RC52 hardware qualification complete. The RC52 BLE companion firmware is a separate repository and image; do not interchange them.

## License

This repository follows the upstream MeshCore MIT license. See [license.txt](license.txt). Hardware pin mapping and board support are derived from Heltec's published RC52 materials and compatible open-source board definitions.
