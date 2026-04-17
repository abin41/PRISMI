# 📘 PRISMI v1.5 — Documentation

## 🧠 Overview

**PRISMI v1.5** is an interactive color-learning board built using ESP32-S3, NeoPixel rings, buttons, and an OLED display.

It is designed to:

* Teach color recognition
* Improve reaction time
* Provide adaptive difficulty
* Deliver engaging visual + audio feedback

---

## ⚙️ Features

### 🎮 Core Gameplay

* Random color selection
* Multi-ring activation with distractors
* Button-based input matching
* Score tracking
* Round-based progression

---

### 🎯 Difficulty System

| Level  | Distractors | Total Active Rings |
| ------ | ----------- | ------------------ |
| Easy   | 1           | 2                  |
| Medium | 3           | 4                  |
| Hard   | 5           | 6                  |

---

### 🌈 Visual Effects

* Breathing brightness animation
* Victory sweep animation
* Wrong hint blinking
* Idle rainbow animation
* Per-ring color identity

---

### 🔊 Audio Feedback

* Correct → ascending tones
* Wrong → low tone
* Timeout → repeated beeps

---

### 🖥 OLED Interface

* Game prompts (color to find)
* Score + round tracking
* Feedback screens
* Game over summary

---

### 🧪 Test Mode

* OLED verification
* Per-ring LED test
* Rainbow animation test

---

## 🔌 Hardware Requirements

* ESP32-S3
* 10x NeoPixel Rings (12 LEDs each)
* 10x Push Buttons
* SSD1306 OLED (I2C)
* Piezo Buzzer
* External 5V Power Supply (recommended)

---

## 🔧 Wiring

### 🔹 LED Configuration

#### Option 1 — Single Chain

```txt
GPIO 4 → Ring 0 → Ring 1 → ... → Ring 9
```

#### Option 2 — Split Mode (Recommended)

```txt
GPIO 4 → Rings 0–4  
GPIO 5 → Rings 5–9
```

Enable in code:

```cpp
#define TWO_STRIPS 1
```

---

### 🔹 Button Wiring

Each button:

```txt
GPIO → Button → GND
```

Pins:

```txt
35, 36, 37, 38, 39, 40, 41, 42, 15, 16
```

---

### 🔹 OLED (I2C)

| OLED | ESP32   |
| ---- | ------- |
| SDA  | GPIO 19 |
| SCL  | GPIO 20 |
| VCC  | 3.3V    |
| GND  | GND     |

---

### 🔹 Buzzer

```txt
+ → GPIO 25  
– → GND
```

---

### ⚡ Power Notes

* Do NOT power LEDs from ESP32
* Use external 5V supply
* Inject power at multiple points:

```txt
Ring 0, Ring 4, Ring 5, Ring 9
```

---

## 🧠 System Architecture

### State Machine

```txt
IDLE → NEW_ROUND → WAITING → FEEDBACK → NEW_ROUND
```

---

### Key Concepts

* **targetRing** → correct answer
* **activeRings[]** → includes distractors
* **COLORS[]** → fixed mapping per ring

---

## 🖥 Serial Commands

Baud rate: **115200**

| Command      | Description       |
| ------------ | ----------------- |
| `start`      | Start game        |
| `stop`       | Stop game         |
| `rounds N`   | Set total rounds  |
| `diff 1/2/3` | Difficulty level  |
| `test`       | Run hardware test |
| `help`       | Show commands     |

---

## 🎮 Gameplay Flow

1. Game starts
2. Random target ring selected
3. Additional distractor rings activated
4. OLED displays target color
5. User presses button
6. System evaluates:

   * Correct → reward + next round
   * Wrong → hint + retry
   * Timeout → reveal + next round

---

## 🧪 Debugging Guide

### ❌ Last ring not lighting

* Check data chain direction
* Verify DOUT → DIN connection
* Add power injection at last ring

---

### ❌ Buttons not working

* Ensure:

```txt
GPIO → Button → GND
```

* Avoid connecting to 3.3V

---

### ❌ OLED errors (I2C NACK)

* Check SDA/SCL wiring
* Confirm address (0x3C / 0x3D)
* Use stable 3.3V power

---

## 🚀 Performance Notes

* Uses RMT for NeoPixel timing
* Supports dual-strip architecture for scalability
* Non-blocking loop design (except controlled delays)
* Optimized for responsiveness and stability

---

## 📦 Version Notes (v1.5)

* Added dual strip support (5+5)
* Improved animation system
* Added breathing effect
* Integrated OLED feedback system
* Added structured serial control
* Introduced hardware test mode

---

## 🧨 Final Notes

This system is:

* Scalable to more rings
* Modular in design
* Ready for real-world deployment
* Stable under load (with proper power)

---

## ✍️ Author

**Altair**
Builder · Cybersecurity Student · Systems Thinker



