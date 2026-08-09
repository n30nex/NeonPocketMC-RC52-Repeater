#!/usr/bin/env python3
"""Static guard for the RC52 TFT-only NeonPocket boot sequence."""

from pathlib import Path


root = Path(__file__).resolve().parents[1]
ui = (root / "examples/simple_room_server/UITask.cpp").read_text(encoding="utf-8")
ini = (root / "variants/heltec_rc52/platformio.ini").read_text(encoding="utf-8")

for token in (
    "#define BOOT_SCREEN_MILLIS 3200",
    "#define BOOT_FRAME_MILLIS 80",
    "drawBootGrid",
    "drawBootSparks",
    "drawPocketMesh",
    '"NEONPOCKETMC"',
    '"ROOM SERVER"',
    '"MESH READY"',
):
    assert token in ui, f"missing splash contract: {token}"

boot = ui.split("void UITask::renderBoot()", 1)[1].split(
    "void UITask::renderPowerConfirm()", 1
)[0]
for forbidden in ("malloc(", "new ", "random(", "delay("):
    assert forbidden not in boot, f"boot renderer must stay procedural: {forbidden}"

headless = ini.split("[env:heltec_rc52_room_server_headless]", 1)[1].split(
    "[env:heltec_rc52_room_server_tft]", 1
)[0]
tft = ini.split("[env:heltec_rc52_room_server_tft]", 1)[1]
assert "NEONPOCKET_ROOM_UI" not in headless
assert "DISPLAY_CLASS" not in headless
assert "-D NEONPOCKET_ROOM_UI=1" in tft
assert "-D DISPLAY_CLASS=NV3001BDisplay" in tft

print("RC52 TFT demoscene splash verifier passed")
