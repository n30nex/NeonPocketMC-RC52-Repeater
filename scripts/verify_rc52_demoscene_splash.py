#!/usr/bin/env python3
"""Static guard for the RC52 TFT-only NeonPocket boot sequence."""

from pathlib import Path


root = Path(__file__).resolve().parents[1]
ui = (root / "examples/simple_room_server/UITask.cpp").read_text(encoding="utf-8")
main = (root / "examples/simple_room_server/main.cpp").read_text(encoding="utf-8")
splash = (root / "examples/simple_room_server/NeonPocketSplash.h").read_text(
    encoding="utf-8"
)
ini = (root / "variants/heltec_rc52/platformio.ini").read_text(encoding="utf-8")

for token in (
    "FRAME_MILLIS = 80",
    "DURATION_MILLIS = 3200",
    "drawPocket",
    '"NEONPOCKETMC"',
    '"ROOM SERVER"',
    '"ROOM SERVICES"',
    "MAGENTA = 0xF81F",
):
    assert token in splash, f"missing splash contract: {token}"

boot = ui.split("void UITask::renderBoot(unsigned long elapsed)", 1)[1].split(
    "void UITask::renderPowerConfirm()", 1
)[0]
for forbidden in ("malloc(", "new ", "random(", "delay("):
    assert forbidden not in boot + splash, f"boot renderer must stay procedural: {forbidden}"
assert "NeonPocketSplash::draw" in boot
assert "NeonPocketSplash::frameForElapsed" in boot

headless = ini.split("[env:heltec_rc52_room_server_headless]", 1)[1].split(
    "[env:heltec_rc52_room_server_tft]", 1
)[0]
tft = ini.split("[env:heltec_rc52_room_server_tft]", 1)[1]
assert "NEONPOCKET_ROOM_UI" not in headless
assert "DISPLAY_CLASS" not in headless
assert "-D NEONPOCKET_ROOM_UI=1" in tft
assert "-D DISPLAY_CLASS=NV3001BDisplay" in tft
assert "ui_task.primeBoot(FIRMWARE_VERSION, FIRMWARE_BUILD_DATE);" in main
assert "STARTING ROOM SERVER" not in main

print("RC52 TFT demoscene splash verifier passed")
