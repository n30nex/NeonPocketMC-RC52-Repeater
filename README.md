<p align="center">
  <img src="https://raw.githubusercontent.com/n30nex/NeonPocketMC/main/branding/neonpocketmc-mark.png" alt="NeonPocketMC pocket mesh logo" width="140">
</p>

# NeonPocketMC-RC52-Repeater

[![RC52 Repeater Build](https://github.com/n30nex/NeonPocketMC-RC52-Repeater/actions/workflows/rc52-repeater-build.yml/badge.svg)](https://github.com/n30nex/NeonPocketMC-RC52-Repeater/actions/workflows/rc52-repeater-build.yml)

Experimental MeshCore repeater and Room Server firmware for the Heltec RadioCore RC52-L62. It is based on MeshCore `v1.17.0` at upstream commit [`727fc0512ce08bfd7b499e46daa7fca6eeec730d`](https://github.com/meshcore-dev/MeshCore/commit/727fc0512ce08bfd7b499e46daa7fca6eeec730d).

> **RC52-L62 only. Do not flash this on RCC6, RC32, T114, or another RC52 radio/display variant.**

**Guided install:** [flasher.canadaverse.org](https://flasher.canadaverse.org/)

## Choose a build

| PlatformIO environment | Purpose | TFT |
|---|---|---:|
| `heltec_rc52_repeater` | Low-overhead LoRa repeater | Off |
| `heltec_rc52_room_server_headless` | MeshCore Room Server for outdoor/headless use | Off |
| `heltec_rc52_room_server_tft` | Room Server with native 220x128 NeonPocket dashboard | On |

### TFT demo-scene startup

<p align="center">
  <img src="https://raw.githubusercontent.com/n30nex/NeonPocketMC/main/docs/images/demoscene/neonpocket-splash.gif" alt="NeonPocketMC animated demo-scene boot sequence" width="660">
</p>

The TFT Room Server uses this shared visual sequence with Room Server status text. The GIF is a direct framebuffer capture from the RCC6 implementation; the RC52 renderer uses the same geometry, palette, cadence, and effects. Headless Room Server and repeater images intentionally show no animation and keep the TFT off.

The Room Server profiles keep 32 recent posts in RAM and expose the standard MeshCore room/client protocol. A reboot clears those buffered posts. The TFT profile adds a required 56,320-byte framebuffer, 8-row delta flushing, animated NeonPocketMC startup, Home/RF/Clients/Posts/Power pages, a 60-second screen timeout, and a 16 KB post-display memory gate. Both Room Server profiles use the user button for next/wake and two-hold System OFF confirmation.

### `v1.1.0-rc.1` release files

| Role | Download this file |
|---|---|
| Repeater | `NeonPocketMC-RC52-Repeater-v1.1.0-rc.1.uf2` |
| Room Server, headless | `NeonPocketMC-RC52-Room-Server-Headless-v1.1.0-rc.1.uf2` |
| Room Server, TFT | `NeonPocketMC-RC52-Room-Server-TFT-v1.1.0-rc.1.uf2` |
| Room Server setup helper | `NeonPocketMC-RC52-Room-Server-Configurator-v1.1.0-rc.1.zip` |

[`v1.1.0-rc.1`](https://github.com/n30nex/NeonPocketMC-RC52-Repeater/releases/tag/v1.1.0-rc.1) is available now. Use only the exact files above from that release; do not substitute a similarly named Actions artifact or another RC52 image.

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

- Download and extract `NeonPocketMC-RC52-Room-Server-Configurator-v1.1.0-rc.1.zip` from the same release as the Room Server UF2.
- Windows: double-click `Configure-RC52-Room-Windows.cmd`.
- Linux: run `sh configure-rc52-room-linux.sh` from the extracted folder.
- Advanced/manual setup: open a 115200-baud USB terminal and use `set name`, `set radio`, `set tx`, `set path.hash.mode 2`, `password`, and `set guest.password`.

The compile-time `password` / `hello` values are onboarding defaults only. **Do not deploy a room server until the wizard has replaced both.** Keep the resulting credentials private. The TFT reports raw battery millivolts, warns at or below 3.45 V, and clears the warning at or above 3.60 V; it does not claim a calibrated percentage and does not automatically shut down on low voltage.

## Install any profile

1. Download the one exact `.uf2` matching the intended role from the GitHub Release table above and verify it against that release's checksum file. The `.hex` files are for development/recovery tooling and are not the normal install path.
2. Connect the RC52 over USB and double-press reset to mount its UF2 bootloader drive.
3. Copy only that `.uf2` to the drive. Wait for the copy to finish and for the RC52 to reboot.
4. Keep USB and a tuned LoRa antenna attached while completing the role-specific setup below.

Every listed UF2 is an **application-only** image starting at `0x26000`. Copying it through the existing UF2 bootloader does not erase or replace the bootloader or SoftDevice. Never perform a whole-chip erase for a normal update. Full instructions: [Flashing](docs/FLASHING.md).

### Repeater first setup

Open the USB serial console at 115200 baud. Immediately change the default admin password, set the repeater name, and configure legal regional radio parameters:

Useful first-run commands:

```text
set name <your repeater name>
password <a strong password>
set radio <frequency>,<bandwidth>,<spreading factor>,<coding rate>
set tx <legal transmit power>
powersaving on
```

Use `help` and the upstream [MeshCore CLI reference](https://docs.meshcore.io/cli_commands) for the complete radio/preset setup. Do not transmit until the frequency, bandwidth, spreading factor, coding rate, and local regulations are correct.

For either Room Server role, use the guided Room Server setup above instead. Do not deploy with the onboarding `password` / `hello` credentials.

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

[`v1.0.0-rc.1`](https://github.com/n30nex/NeonPocketMC-RC52-Repeater/releases/tag/v1.0.0-rc.1) remains the original experimental repeater-only release. [`v1.1.0-rc.1`](https://github.com/n30nex/NeonPocketMC-RC52-Repeater/releases/tag/v1.1.0-rc.1) is the current experimental prerelease and includes the repeater, both Room Server profiles, and configurator named above. Its images are exact-main CI artifacts; the new Room Server profiles were not physically re-tested on the RC52 qualification unit for this candidate.

The RC52 BLE companion firmware is a separate repository and image. Do not interchange companion, repeater, Room Server, RCC6, or alternate RC52 hardware files.

## License

This repository follows the upstream MeshCore MIT license. See [license.txt](license.txt). Hardware pin mapping and board support are derived from Heltec's published RC52 materials and compatible open-source board definitions.
