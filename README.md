# SwitchWarden

A touchscreen front panel for a triple-PC bench running on one shared water-cooling loop. Three PCs
each have their own power supply, and a fourth PSU drives the loop's pumps and fans. SwitchWarden
starts and stops all four from one 480x320 LCD, and enforces the rule that makes a shared loop safe:
**no PC runs without cooling.**

Firmware for a Teensy 4.0 on the `teensy40_480x320_lcd` board — ILI9488 display, FT6206 capacitive
touch — built on the [EmbeddedGFX](https://github.com/BrandonVP/EmbeddedGFX) library.

## What it does

The Power tab shows one card per supply with a live run-status pill and a START / STOP button:

| Supply | Control style |
| --- | --- |
| PC 1, PC 2, PC 3 | Momentary — pulses the motherboard PWR_SW like a finger on the case button |
| Cooling | Level — holds PS_ON asserted for as long as the loop should run |

Each supply also has a **run monitor** input, so the panel shows what the hardware is actually doing
rather than what it was told to do. A PC shut down from its own OS flips back to START on the panel
by itself.

### Cooling interlocks

The loop is shared, so the cooling PSU is not an independently switchable thing while anything is
generating heat:

- **Starting a PC starts cooling first.** The press is deferred until the cooling supply's monitor
  reports it actually running. If it never comes up within 20 s, the start is abandoned and a fault
  latches.
- **Cooling locks while any PC is active.** Its button reads LOCKED and ignores taps.
- **Cooling auto-stops 30 s after the last PC.** Only when cooling was auto-started by a PC —
  cooling you started by hand stays on until you stop it.
- **Loss of cooling force-stops the PCs.** If a PC is running while cooling isn't, the firmware
  re-asserts cooling and starts a 10 s grace window. If cooling still isn't running when it expires,
  every running PC gets a long PWR_SW hold (ATX hard power-off) and a fault latches.

A latched fault turns the cooling row red, and the UI jumps to the Power tab from whatever tab was
showing so the alert can't be missed. The cooling button becomes ACK to clear it.

"Active" means running *or* waiting on cooling to start — never a stale intent — so an external
shutdown or a disconnected monitor lead releases the lock immediately.

## Hardware

| Part | Role in the build |
| --- | --- |
| [Copperhill Teensy 4.0 CAN / CAN FD board with 480x320 3.5" touch LCD](https://copperhilltech.com/teensy-4-0-classic-can-can-fd-board-with-480x320-3-5-touch-lcd/) | The panel itself — Teensy 4.0, ILI9488 display, FT6206 touch, and the J7 header the harness plugs into. The CAN transceivers go unused here. |
| [Optical isolation module](https://www.amazon.com/dp/B0GGBTTF49) | One channel per PSU, sensing +12V to tell the Teensy whether that supply is actually running. Keeps each machine's ground off the panel. |
| [ULN2003 darlington array board](https://www.amazon.com/dp/B08CHHLPL9) | Low-side driver on the four start outputs. A HIGH from the Teensy sinks the channel to ground — grounding a motherboard's PWR_SW header, or the cooling PSU's PS_ON. |
| [ATX PSU switch / jumper](https://www.amazon.com/dp/B09XTYKHV5) | Manual override and bench-test switch for the cooling supply. |

Four supplies total: one ATX PSU per PC, plus a fourth running the loop's pumps and fans.

## Wiring

Connector **J7** (`Conn_01x10`) on the LCD board breaks out Teensy pins 0–8 (J7.10 is GND):

| PSU | Start out | Run monitor in |
| --- | --- | --- |
| PC 1 | Teensy 0 (J7.9) | Teensy 4 (J7.5) |
| PC 2 | Teensy 1 (J7.8) | Teensy 5 (J7.4) |
| PC 3 | Teensy 2 (J7.7) | Teensy 6 (J7.3) |
| Cooling | Teensy 3 (J7.6) | Teensy 7 (J7.2) |

Pin 8 (J7.1) is spare. Display and touch are fixed by the board: CS=10, DC=9, MOSI=11, MISO=12,
SCK=13, BL=14, RST=15, FT6206 on SDA0=18 / SCL0=19. See `teensy40_480x320_lcd.pdf` for the schematic.

Run monitoring goes through an inverting opto-isolator module sensing each PSU's +12V — it pulls its
output LOW when the supply is up. **Teensy 4.0 pins are not 5V tolerant**, so power the opto's output
side from 3.3V. Polarity constants (`START_ACTIVE_HIGH`, `RUNNING_ACTIVE_HIGH`), the pin map, and the
pulse timings all live at the top of
[PowerControl.cpp](SwitchWarden/PowerControl.cpp) if your interface hardware differs.

## Building

Clone with submodules — EmbeddedGFX is one:

```
git clone --recurse-submodules https://github.com/BrandonVP/SwitchWarden.git
```

Open `SwitchWarden.sln` (Visual Micro) or `SwitchWarden/SwitchWarden.ino` in the Arduino IDE with
Teensyduino. Board: **Teensy 4.0**. Libraries: `ILI9488_t3`, `Adafruit_FT6206`, and EmbeddedGFX from
`SwitchWarden/Libraries/`.

## Layout

| File | Purpose |
| --- | --- |
| `SwitchWarden.ino` | Setup, menu bar, app registration, main loop |
| `PowerControl.cpp/.h` | Hardware layer — pin map, pulse engine, cooling interlocks and faults |
| `PowerApp.cpp/.h` | Power tab UI — PSU cards, status pills, START/STOP/LOCKED/ACK buttons |
| `MonitorApp.cpp/.h` | Monitor tab — placeholder for future loop telemetry |
| `AboutApp.cpp/.h` | Settings > About — version and project info |
| `appConfig.h` | Screen size overrides plus the menu/app id enums |
| `ILI9488Adapter.h`, `FT6206Adapter.h` | EmbeddedGFX `IDisplay` / `ITouch` implementations |

Settings > Themes is the library's theme picker; the panel defaults to a light "frost" palette set in
`applyFrostPalette()`. Themes are RAM-only and reset on power cycle.

Version is defined in `Version.h` (currently 1.0.0) and shown on the About page.
