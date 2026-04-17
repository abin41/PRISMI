# PRISMI
# 🎯 Color Hunt Game  
### ESP32 + NeoPixel + OLED Interactive Game

An engaging **reaction + color recognition game** built using **ESP32**, **NeoPixel LED rings**, and an **I2C OLED display**.

Players must quickly identify the correct LED ring that matches the color shown on the display — all within a limited time.

---

## 🚀 Overview

This project combines:
- 🎨 Visual perception (color matching)
- ⚡ Fast reaction gameplay
- 💡 LED animations + OLED feedback

Perfect for:
- Embedded systems projects  
- Hackathons  
- Interactive demos  
- Learning ESP32 + hardware integration  

---

## ✨ Features

- 🎯 Random target color every round  
- 🔵 10 independent LED rings  
- 📺 OLED instructions + results  
- ⏱️ 5-second response timer  
- ✅ Correct / ❌ Wrong / ⚠ MISS feedback  
- 💥 LED blink animation for wrong answers  
- 🔁 Continuous gameplay loop  
- ⚡ Full brightness NeoPixel output  

---

## 🧠 Game Logic

1. A **random color** is displayed on the OLED  
2. All 10 rings light up with different colors  
3. Only **one ring matches the target color**  
4. Player presses a button:

| Action | Result |
|------|--------|
| Correct button | ✅ “YOU FOUND” |
| Wrong button   | ❌ “WRONG” + LED blink |
| No input (5s)  | ⚠ “MISS” |

---

## 🛠️ Hardware Requirements

- ESP32 (ESP32-C3 / WROOM recommended)  
- 2 × NeoPixel strips (60 LEDs each)  
- 128×64 OLED Display (SSD1306, I2C)  
- 10 Push Buttons  
- Jumper wires  
- External 5V power supply ⚠  

---

## 🔌 Pin Configuration

### LED Strips
| Strip | GPIO |
|------|------|
| Strip 1 | 14 |
| Strip 2 | 12 |

### OLED (I2C)
| Signal | GPIO |
|--------|------|
| SDA    | 8    |
| SCL    | 9    |

### Buttons
GPIO: 1, 2, 42, 41, 40, 39, 38, 37, 36, 35
Mode: INPUT_PULLUP


---

## 📦 Libraries

Install using Arduino Library Manager:

- Adafruit NeoPixel  
- Adafruit GFX  
- Adafruit SSD1306  
- Wire (built-in)  

---
Open in Arduino IDE
Install required libraries
Select ESP32 board
Upload code
⚙️ Technical Details
Total Rings: 10
LEDs per Ring: 12
LEDs per Strip: 60
Brightness: 255 (Max)
Input Mode: INPUT_PULLUP
⚡ Version History
🟢 v1.0
Core gameplay system
LED + OLED integration
🔵 v1.1
Improved stability
Better display layout
Optimized input handling
📸 Future Improvements
🔊 Sound feedback (buzzer)
🧠 Difficulty levels
📊 Score system
📱 WiFi leaderboard (ESP32 feature)
🎨 Smooth LED transitions
⚠️ Important Notes
Use external power supply for NeoPixels
Always connect common GND
High brightness = high current ⚠
🤝 Contributing

Contributions are welcome!
Feel free to fork, improve, and submit pull requests.



💡 Inspiration

A simple idea turned into a fun embedded system game combining hardware + logic + interaction.


---

If you want to push this even further, I can:
- add **cool badges (build, version, etc.)**
- generate **preview images**
- design a **GitHub repo structure (folders, assets, docs)**

Just say 👍
