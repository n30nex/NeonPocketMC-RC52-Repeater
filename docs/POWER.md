# Solar and battery power

## The RC52 is not a solar charger

The RC52 schematic shows an LGS4056HDA single-cell linear charger supplied by regulated `VDD_5V` from USB. It does not show MPPT, a wide-input solar regulator, or a protected raw-panel input.

Official references:

- [Heltec RC52-L62 schematic](https://resource.heltec.cn/download/RadioCore/RC52/schematic/RC52-L62_V1.0.pdf)
- [Heltec MeshSolar example](https://heltec.org/project/meshsolar/)

## Recommended architecture

```text
unregulated solar panel
  -> external MPPT charger designed for a protected 1S Li-ion/LiPo pack
  -> battery protection and load sharing
  -> protected 1S battery output
  -> RC52 VBAT and GND
```

An external controller may instead supply genuinely regulated 5 V to the RC52 `5V`/USB input, but the controller must be designed for that solar/battery power path. Do not connect the same system to both RC52 `VBAT` and `5V` unless the external controller explicitly supports that topology.

Never connect an unregulated panel directly to `VBAT`, `5V`, or USB. Never exceed the RC52 or battery manufacturer's voltage, current, temperature, and polarity limits.

## Firmware power behavior

- TFT power and backlight remain off.
- BLE, Web/AP, and companion transports are not compiled in.
- The nRF52840 waits in event sleep between work when possible.
- The SX1262 remains available for continuous repeater receive, so RF reception dominates idle energy use.
- A 1.5-second user-button hold powers down peripherals and enters nRF52 System OFF; button sense wakes the board.
- Battery voltage is measurable, but automatic low-voltage shutdown is intentionally disabled until the RC52 ADC multiplier and thresholds are calibrated against a multimeter.

Size the panel, battery, controller, enclosure, antenna, and thermal design from measured 24-hour energy use at the deployed radio settings—not from CPU sleep current alone.
