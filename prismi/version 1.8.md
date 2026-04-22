# PRISMI v1.8 — Autism-Friendly Color Learning Board

> An interactive, hardware-based color recognition game built for children on the autism spectrum. Ten NeoPixel rings, one OLED display, and three difficulty levels combine into a sensory-friendly, engaging learning tool.

---

## Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Wiring](#wiring)
- [Dependencies](#dependencies)
- [Configuration](#configuration)
- [Difficulty Levels](#difficulty-levels)
- [Serial Commands](#serial-commands)
- [How It Works](#how-it-works)
- [LED Strip Modes](#led-strip-modes)
- [Game Flow](#game-flow)
- [Feedback & Accessibility](#feedback--accessibility)
- [Test Mode](#test-mode)
- [License](#license)

---

## Overview

PRISMI is an Arduino/ESP32-based color learning game designed with autism-friendly sensory feedback in mind. A child is prompted (via OLED display) to press the button corresponding to a specific color among ten illuminated NeoPixel rings. The game supports three difficulty levels, adjustable round counts, buzzer feedback, and animated LED responses.

---

## Hardware Requirements

| Component | Quantity | Notes |
|---|---|---|
| ESP32 (or compatible) | 1 | GPIO 4, 5, 15, 16, 19, 20, 25, 35–42 used |
| WS2812B NeoPixel Ring (12-LED) | 10 | Chained or split 5+5 |
| SSD1306 OLED Display (128×64) | 1 | I²C, address `0x3C` or `0x3D` |
| Passive Piezo Buzzer | 1 | Connected to GPIO 25 |
| Momentary Push Buttons | 10 | One per ring, active-low (INPUT_PULLUP) |
| 5V Power Supply | 1 | Sized for LED load (10 rings × 12 LEDs) |

---

## Wiring

### Single-strip mode (default)

```
GPIO 4  →  Ring 0 DIN
Ring 0 DOUT  →  Ring 1 DIN  →  …  →  Ring 9 DOUT
```

### Two-strip mode (`TWO_STRIPS 1`)

```
GPIO 4  →  Rings 0–4  (DIN chain)
GPIO 5  →  Rings 5–9  (DIN chain)
```

### OLED (I²C)

```
OLED SDA  →  GPIO 19
OLED SCL  →  GPIO 20
```

### Buttons

```
BUTTONS[0..9]  →  GPIO 35, 36, 37, 38, 39, 40, 41, 42, 15, 16
                   (each pulled to GND when pressed)
```

### Piezo

```
GPIO 25  →  Piezo +
GND      →  Piezo –
```

---

## Dependencies

Install via the Arduino Library Manager:

| Library | Version tested |
|---|---|
| `Adafruit NeoPixel` | ≥ 1.11 |
| `Adafruit GFX Library` | ≥ 1.11 |
| `Adafruit SSD1306` | ≥ 2.5 |

---

## Configuration

All compile-time settings live at the top of `prismi.ino`:

```cpp
#define NUM_RINGS        10    // Number of NeoPixel rings
#define LEDS_PER_RING    12    // LEDs per ring
#define PIEZO_PIN        25    // Buzzer GPIO

#define OLED_SDA         19
#define OLED_SCL         20

#define TWO_STRIPS       1     // 0 = single chain, 1 = split 5+5
#define DATA_PIN_A       4
#define DATA_PIN_B       5

#define ROUND_TIMEOUT_MS 8000  // ms before timeout per round
#define DEBOUNCE_MS      50    // button debounce window

#define SERIAL_DEBUG     0     // 1 = verbose serial logging
```

**Brightness** is set to maximum (`255`) in `setup()`. Lower it if power draw is a concern:

```cpp
stripA.setBrightness(120);  // 0–255
```

---

## Difficulty Levels

| Level | Command | Layout | Description |
|---|---|---|---|
| 1 — Easy | `diff 1` | 1 ring vs 9 rings | Two colors: target color appears on either 1 or 9 rings |
| 2 — Medium | `diff 2` | 3 + 3 + 3 + 1 | Four colors: target appears on 3 or 1 ring(s) among four groups |
| 3 — Hard | `diff 3` | All unique | Every ring shows a different color; child must identify the named one |

Default difficulty on boot: **Easy (1)**.

---

## Serial Commands

Connect at **115200 baud**.

| Command | Action |
|---|---|
| `start` | Begin a new game |
| `stop` | End the current game |
| `diff 1` / `diff 2` / `diff 3` | Set difficulty (Easy / Medium / Hard) |
| `rounds N` | Set number of rounds (e.g., `rounds 5`) |
| `test` | Run LED and OLED hardware self-test |
| `help` | Print command reference |

---

## How It Works

### Colors

Ten colors are hard-coded, one per ring index:

| Index | Color | RGB |
|---|---|---|
| 0 | RED | `255, 0, 0` |
| 1 | GREEN | `0, 220, 0` |
| 2 | BLUE | `0, 120, 255` |
| 3 | YELLOW | `255, 220, 0` |
| 4 | ORANGE | `255, 100, 0` |
| 5 | PURPLE | `180, 0, 255` |
| 6 | PINK | `255, 50, 150` |
| 7 | TEAL | `0, 200, 180` |
| 8 | CYAN | `0, 220, 255` |
| 9 | LIME | `130, 255, 0` |

---

## LED Strip Modes

### Single strip (`TWO_STRIPS 0`)

All 120 LEDs are chained on `DATA_PIN_A`. Ring `i` occupies pixel indices `i*12` through `i*12+11`.

### Two strips (`TWO_STRIPS 1`)

Rings 0–4 (60 LEDs) are on `DATA_PIN_A`; rings 5–9 (60 LEDs) are on `DATA_PIN_B`. The `setPixel()` abstraction handles address mapping transparently.

---

## Game Flow

```
IDLE
 │
 └─ serial: "start"
     │
     ▼
NEW_ROUND ──────────────────────────────────────────┐
 │  - Pick target color                              │
 │  - Assign colors to rings (per difficulty)        │
 │  - Light all rings                                │
 │  - Show OLED prompt                               │
 ▼                                                   │
WAITING                                             │
 │  - Poll buttons                                   │
 │  - Check timeout (8 s default)                    │
 │                                                   │
 ├─ Correct button pressed ──► onCorrect()           │
 │    Victory sound + sweep + score++ ───────────────┘
 │
 ├─ Wrong button pressed ──► onWrong()
 │    Buzz + red flash + hint blink
 │    Rings restored → back to WAITING
 │
 └─ Timeout ──► onTimeout()
      Triple beep + blink target rings ──────────────┘

After maxRounds:
 IDLE → OLED Game Over screen → idle rainbow
```

---

## Feedback & Accessibility

PRISMI uses **multi-modal feedback** to be inclusive:

| Event | Visual | Audio |
|---|---|---|
| Correct | White victory sweep across all rings, then pressed ring flashes its color | Two ascending tones (880 Hz → 1320 Hz) |
| Wrong | Pressed ring flashes red; all rings blink twice as a hint | Single low buzz (300 Hz, 400 ms) |
| Timeout | Only target-colored rings blink three times | Three short beeps (400 Hz) |
| Game over | Idle rainbow resumes | — |

The OLED always shows the current target color name in large text, plus round number and score.

---

## Test Mode

Send `test` via serial while the game is idle to run a hardware self-test:

1. OLED displays a test message.
2. Each ring lights up individually in its assigned color.
3. A full rainbow chase animation runs across all rings.
4. LEDs turn off; OLED returns to idle screen.

---

## License
Yenepoya Incubation License — free to use, modify, and distribute with attribution.

---

*PRISMI v1.8 · Built with ❤️ for sensory-friendly learning.*
