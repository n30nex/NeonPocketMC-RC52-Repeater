#!/usr/bin/env python3
"""Guided USB setup for NeonPocketMC RC52 Room Server firmware."""

from __future__ import annotations

import argparse
import getpass
import re
import sys
import time


USB_VID = 0x239A
USB_PID = 0x8071
BAUD = 115200

RADIO_PRESETS = (
    ("USA / Canada", "910.525", "62.5", 7, 5),
    ("Australia", "915.800", "250", 10, 5),
    ("Australia narrow", "916.575", "62.5", 7, 8),
    ("Brazil", "923.125", "62.5", 8, 8),
    ("EU / UK narrow", "869.618", "62.5", 8, 8),
    ("EU 433 MHz", "433.650", "62.5", 8, 8),
    ("Netherlands", "869.618", "62.5", 7, 5),
    ("New Zealand", "917.375", "62.5", 7, 5),
    ("Switzerland", "869.618", "62.5", 8, 8),
    ("Vietnam", "920.250", "62.5", 8, 5),
)


class SetupError(RuntimeError):
    pass


class DeviceCLI:
    def __init__(self, port: str):
        try:
            import serial
        except ImportError as exc:
            raise SetupError("PySerial is missing. Use the supplied Windows or Linux launcher.") from exc

        self.serial = serial.Serial()
        self.serial.port = port
        self.serial.baudrate = BAUD
        self.serial.timeout = 0.08
        self.serial.write_timeout = 2
        self.serial.dtr = False
        self.serial.rts = False
        try:
            self.serial.open()
        except Exception as exc:
            hint = " Add your user to the dialout group and sign in again." if sys.platform != "win32" else ""
            raise SetupError(f"Could not open {port}: {exc}.{hint}") from exc
        time.sleep(1.0)
        self.serial.reset_input_buffer()

    def close(self) -> None:
        if self.serial.is_open:
            self.serial.close()

    def command(self, command: str, *, timeout: float = 4.0, secret: bool = False) -> str:
        label = command.split(" ", 1)[0] + " ********" if secret else command
        print(f"  {label:<48}", end="", flush=True)
        self.serial.reset_input_buffer()
        self.serial.write((command + "\r").encode("utf-8"))
        self.serial.flush()

        deadline = time.monotonic() + timeout
        data = bytearray()
        while time.monotonic() < deadline:
            chunk = self.serial.read(max(1, self.serial.in_waiting))
            if chunk:
                data.extend(chunk)
                text = data.decode("utf-8", errors="replace")
                replies = re.findall(r"(?:^|[\r\n])\s*->\s*([^\r\n]*)", text)
                if replies and replies[-1].strip():
                    reply = replies[-1].strip()
                    if reply.lower().startswith(("err", "unknown", "??")):
                        print("FAILED")
                        raise SetupError(f"The device rejected '{label}': {reply}")
                    print("OK")
                    return reply
            else:
                time.sleep(0.03)
        print("NO REPLY")
        raise SetupError(f"The device did not answer '{label}'. Keep USB connected and try again.")

    def reboot(self) -> None:
        print("  rebooting device")
        self.serial.reset_input_buffer()
        self.serial.write(b"reboot\r")
        self.serial.flush()
        time.sleep(0.6)


def list_ports():
    try:
        from serial.tools import list_ports
    except ImportError as exc:
        raise SetupError("PySerial is missing. Use the supplied Windows or Linux launcher.") from exc
    return sorted(list_ports.comports(), key=lambda item: item.device.lower())


def ask_number(label: str, minimum: int, maximum: int, default: int | None = None) -> int:
    while True:
        suffix = f" [{default}]" if default is not None else ""
        value = input(f"{label}{suffix}: ").strip()
        if not value and default is not None:
            return default
        try:
            parsed = int(value)
        except ValueError:
            parsed = minimum - 1
        if minimum <= parsed <= maximum:
            return parsed
        print(f"  Enter a number from {minimum} to {maximum}.")


def choose_port(requested: str | None) -> str:
    if requested:
        return requested
    ports = list_ports()
    if not ports:
        raise SetupError("No serial devices found. Connect the RC52 with a USB data cable.")
    likely = [p for p in ports if p.vid == USB_VID and p.pid == USB_PID]
    if len(likely) == 1:
        print(f"Found RC52: {likely[0].device} ({likely[0].description})")
        return likely[0].device
    print("\nConnected serial devices:")
    for index, port in enumerate(ports, 1):
        marker = "  <-- likely RC52" if port in likely else ""
        print(f"  {index:2}. {port.device:<16} {port.description}{marker}")
    return ports[ask_number("Choose the RC52 USB port", 1, len(ports)) - 1].device


def ask_text(label: str, default: str, maximum: int) -> str:
    while True:
        value = input(f"{label} [{default}]: ").strip() or default
        if len(value) > maximum:
            print(f"  Use at most {maximum} characters.")
        elif any(ch in value for ch in "[]\\:,?*"):
            print("  Do not use [ ] \\ : , ? or *.")
        else:
            return value


def ask_password(label: str) -> str:
    while True:
        first = getpass.getpass(f"{label} (8-15 characters): ")
        second = getpass.getpass("Repeat it: ")
        if first != second:
            print("  Passwords did not match.")
        elif not 8 <= len(first) <= 15:
            print("  Use 8 to 15 characters.")
        elif any(ch in first for ch in "\r\n"):
            print("  Newlines are not allowed.")
        else:
            return first


def verify_device(cli: DeviceCLI) -> None:
    version = cli.command("ver")
    board = cli.command("board")
    role = cli.command("get role")
    if "rc52" not in board.lower() or "room_server" not in role.lower():
        raise SetupError(
            f"Refusing this device. It reported version '{version}', board '{board}', role '{role}'."
        )
    print(f"Verified RC52 Room Server: {version}\n")


def configure(port: str) -> None:
    cli = DeviceCLI(port)
    try:
        verify_device(cli)
        name = ask_text("Room name", "NeonPocket RC52 Room", 31)

        print("\nRadio preset (confirm that it is legal for your location):")
        for index, preset in enumerate(RADIO_PRESETS, 1):
            print(f"  {index:2}. {preset[0]:22} {preset[1]} MHz  BW {preset[2]}  SF{preset[3]} CR{preset[4]}")
        choice = ask_number("Preset", 1, len(RADIO_PRESETS), 1)
        _, freq, bw, sf, cr = RADIO_PRESETS[choice - 1]
        tx_power = ask_number("TX power in dBm", 2, 22, 20)

        print("\nChoose new credentials. They are not saved by this script.")
        admin_password = ask_password("Admin password")
        guest_password = ask_password("Room guest password")

        print("\nWriting settings:")
        cli.command(f"set name {name}")
        cli.command(f"set radio {freq},{bw},{sf},{cr}")
        cli.command(f"set tx {tx_power}")
        cli.command("set path.hash.mode 2")
        cli.command(f"password {admin_password}", secret=True)
        cli.command(f"set guest.password {guest_password}", secret=True)
        cli.reboot()
        print("\nSETUP COMPLETE")
        print(f"  Name: {name}")
        print(f"  Radio: {freq} MHz / BW {bw} / SF{sf} / CR{cr} / {tx_power} dBm")
        print("  Advert hash mode: 3-byte")
        print("  Credentials: changed (not displayed or retained)")
        print("You may disconnect USB after the device finishes rebooting.")
    finally:
        cli.close()


def self_test() -> None:
    assert USB_VID == 0x239A and USB_PID == 0x8071
    assert RADIO_PRESETS[0][1:] == ("910.525", "62.5", 7, 5)
    assert all(150 < float(row[1]) < 1000 for row in RADIO_PRESETS)
    assert all(5 <= row[3] <= 12 and 5 <= row[4] <= 8 for row in RADIO_PRESETS)
    print("RC52 room configurator self-test passed")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="Serial port, for example COM22 or /dev/ttyACM0")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0

    print("NeonPocketMC RC52 Room Server Setup")
    print("Keep USB and a tuned LoRa antenna connected until setup completes.\n")
    try:
        configure(choose_port(args.port))
        return 0
    except (SetupError, KeyboardInterrupt) as exc:
        print(f"\nSETUP STOPPED: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
