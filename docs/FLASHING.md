# Flashing the RC52 repeater

## Before flashing

- Confirm the board is a Heltec RadioCore **RC52-L62** with the nRF52840/SX1262 hardware used by this repository.
- Download only `NeonPocketMC-RC52-Repeater.uf2` from this repository's release.
- Save any configuration you may need. The application-only UF2 is intended to preserve the bootloader, SoftDevice, identity, and settings, but backups remain sensible for experimental firmware.

## UF2 install

1. Connect the RC52 over USB.
2. Double-press reset. A UF2 mass-storage drive should appear.
3. Copy `NeonPocketMC-RC52-Repeater.uf2` to the drive.
4. Wait for the drive to disappear and the RC52 to reboot.
5. Open USB serial at 115200 baud and confirm one clean boot and a repeater ID.

Do not erase the chip, flash at offset zero, replace the bootloader, or install a full-device HEX unless you are deliberately recovering the board with vendor tooling. The release UF2 is an application image targeting `0x26000`.

## First configuration

The fresh-install defaults are deliberately generic. Change at least:

```text
set name <your repeater name>
password <a strong password>
set freq <legal regional frequency>
powersaving on
```

Configure the remaining radio parameters to match your MeshCore region and network. The default password is `password` only to preserve upstream serial setup behavior; it is not safe for deployment.

## Recovery

- Hold the user button during the first eight seconds of boot to use MeshCore CLI rescue behavior.
- A 1.5-second button hold during normal operation enters System OFF. Press the user button to wake.
- If the device does not enumerate or boot, return to UF2 mode with a double reset and restore a known-good RC52 application UF2. Do not use RCC6, RC32, or T114 firmware.
