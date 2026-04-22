/*
 ╔══════════════════════════════════════════════════════════════╗
 ║           COLOR RING GAME  —  v2.0                          ║
 ║  10 NeoPixel rings  |  OLED display  |  Serial control      ║
 ╠══════════════════════════════════════════════════════════════╣
 ║  SERIAL COMMANDS (115200 baud)                              ║
 ║   start       – start / resume the game                    ║
 ║   stop        – pause after current round                  ║
 ║   level 1     – switch to Level 1 (easy)                   ║
 ║   level 2     – switch to Level 2 (medium)                 ║
 ║   level 3     – switch to Level 3 (hard)                   ║
 ║   reset       – clear score & restart                      ║
 ║   status      – print current state                        ║
 ╚══════════════════════════════════════════════════════════════╝

 LEVEL RULES
 ───────────
 Level 1 – EASY
   • 1 ring shows the CORRECT color (target).
   • The other 9 rings all show a SINGLE random WRONG color
     that blinks continuously until the player presses a button.

 Level 2 – MEDIUM
   • 1 ring shows the CORRECT color.
   • The remaining 9 rings are split into 3 groups of 3;
     each group blinks a different random WRONG color.
     Groups are reshuffled randomly every round.

 Level 3 – HARD
   • 1 ring shows the CORRECT color.
   • Each of the other 9 rings shows its OWN random WRONG color
     (no duplicates allowed — every ring has a unique color,
     and NONE of them matches the correct color).
   • No blinking — all rings are lit steadily.
*/

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── PIN / STRIP CONFIG ──────────────────────────────────────
#define PIN1  14
#define PIN2  12
#define NUM   60          // 5 rings × 12 LEDs each, per strip

Adafruit_NeoPixel strip1(NUM, PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(NUM, PIN2, NEO_GRB + NEO_KHZ800);

// ─── OLED CONFIG ─────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ─── BUTTON CONFIG ───────────────────────────────────────────
const int BUTTON_PINS[10] = {1, 2, 42, 41, 40, 39, 38, 37, 36, 35};
bool lastButtonState[10];

// ─── COLOR DEFINITIONS ───────────────────────────────────────
const char* COLOR_NAMES[10] = {
  "RED", "GREEN", "BLUE", "YELLOW",
  "CYAN", "MAGENTA", "ORANGE",
  "PURPLE", "PINK", "WHITE"
};

uint32_t getColor(int idx) {
  switch (idx) {
    case 0: return strip1.Color(255,   0,   0);   // RED
    case 1: return strip1.Color(  0, 255,   0);   // GREEN
    case 2: return strip1.Color(  0,   0, 255);   // BLUE
    case 3: return strip1.Color(255, 255,   0);   // YELLOW
    case 4: return strip1.Color(  0, 255, 255);   // CYAN
    case 5: return strip1.Color(255,   0, 255);   // MAGENTA
    case 6: return strip1.Color(255, 100,   0);   // ORANGE
    case 7: return strip1.Color(150,   0, 255);   // PURPLE
    case 8: return strip1.Color(255,  20, 147);   // PINK
    case 9: return strip1.Color(255, 255, 255);   // WHITE
    default: return 0;
  }
}

// ─── GAME STATE ──────────────────────────────────────────────
int  currentLevel   = 1;
bool gameRunning    = false;
int  score          = 0;
int  roundNum       = 0;

// Per-round data
int  targetColor    = 0;    // index 0-9 of the correct colour
int  correctRing    = 0;    // which ring (0-9) holds the correct colour
int  ringColorIdx[10];      // colour index assigned to each ring

// Blink state for L1 / L2
bool blinkOn        = false;
unsigned long lastBlink = 0;
#define BLINK_INTERVAL 400  // ms

// Level 2: group assignments
// groups[i] = which of 3 WRONG colors (0,1,2) ring i uses
// (-1 means it is the correct ring)
int  wrongColorIdx[3];      // 3 different wrong colours for L2

// Timer
unsigned long roundStart = 0;
const unsigned long ROUND_TIME = 6000;  // 6 s

bool answered = false;

// ─── SERIAL INPUT BUFFER ─────────────────────────────────────
String serialBuf = "";

// ─── OLED HELPERS ────────────────────────────────────────────

void oledClear() {
  display.clearDisplay();
  display.setTextColor(WHITE);
}

void oledShow() {
  display.display();
}

// Big centred title line
void oledTitle(const char* txt, int y, int size) {
  display.setTextSize(size);
  int w = strlen(txt) * 6 * size;
  int x = (SCREEN_WIDTH - w) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, y);
  display.print(txt);
}

void showSplash() {
  oledClear();
  oledTitle("COLOR", 5, 2);
  oledTitle("GAME", 25, 2);

  display.setTextSize(1);
  display.setCursor(20, 48);
  display.print("Send 'start' to play");
  oledShow();
}

void showLevelBanner(int lvl) {
  oledClear();
  char buf[12];
  sprintf(buf, "LEVEL %d", lvl);
  oledTitle(buf, 10, 2);

  display.setTextSize(1);
  const char* desc =
    (lvl == 1) ? "  Easy - 1 wrong color" :
    (lvl == 2) ? "Med - 3 color groups" :
                 "Hard - all different";
  display.setCursor(4, 40);
  display.print(desc);

  char sc[20];
  sprintf(sc, "Score: %d", score);
  display.setCursor(4, 54);
  display.print(sc);

  oledShow();
  delay(1200);
}

void showFind(int targetIdx, int lvl) {
  oledClear();

  // Level badge (top-left)
  display.setTextSize(1);
  char lvlBuf[8];
  sprintf(lvlBuf, "LVL %d", lvl);
  display.setCursor(0, 0);
  display.print(lvlBuf);

  // Score (top-right)
  char scBuf[12];
  sprintf(scBuf, "SC:%d", score);
  display.setCursor(SCREEN_WIDTH - strlen(scBuf)*6, 0);
  display.print(scBuf);

  // Divider
  display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);

  // "FIND" label
  display.setTextSize(1);
  oledTitle("FIND THE COLOR", 14, 1);

  // Color name — large
  display.setTextSize(2);
  int nameLen = strlen(COLOR_NAMES[targetIdx]);
  int x = (SCREEN_WIDTH - nameLen * 12) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, 30);
  display.print(COLOR_NAMES[targetIdx]);

  // Timer bar placeholder (refreshed during blink)
  display.drawRect(0, 54, SCREEN_WIDTH, 8, WHITE);

  oledShow();
}

void updateTimerBar(unsigned long elapsed, unsigned long total) {
  int w = (int)((float)(total - elapsed) / total * SCREEN_WIDTH);
  if (w < 0) w = 0;

  // Redraw timer bar only
  display.fillRect(0, 54, SCREEN_WIDTH, 8, BLACK);
  display.drawRect(0, 54, SCREEN_WIDTH, 8, WHITE);
  display.fillRect(1, 55, w - 2, 6, WHITE);
  oledShow();
}

void showResult(int targetIdx, bool correct) {
  oledClear();

  if (correct) {
    oledTitle("CORRECT!", 4, 2);
    display.drawFastHLine(0, 22, SCREEN_WIDTH, WHITE);

    display.setTextSize(1);
    oledTitle("You found", 26, 1);

    display.setTextSize(2);
    int nameLen = strlen(COLOR_NAMES[targetIdx]);
    int x = (SCREEN_WIDTH - nameLen * 12) / 2;
    if (x < 0) x = 0;
    display.setCursor(x, 38);
    display.print(COLOR_NAMES[targetIdx]);

    char sc[16];
    sprintf(sc, "Score: %d", score);
    display.setTextSize(1);
    oledTitle(sc, 56, 1);

  } else {
    oledTitle("WRONG!", 4, 2);
    display.drawFastHLine(0, 22, SCREEN_WIDTH, WHITE);

    display.setTextSize(1);
    oledTitle("Correct was:", 28, 1);

    display.setTextSize(2);
    int nameLen = strlen(COLOR_NAMES[targetIdx]);
    int x = (SCREEN_WIDTH - nameLen * 12) / 2;
    if (x < 0) x = 0;
    display.setCursor(x, 40);
    display.print(COLOR_NAMES[targetIdx]);
  }

  oledShow();
}

void showMiss(int targetIdx) {
  oledClear();
  oledTitle("TIME UP!", 2, 2);
  display.drawFastHLine(0, 20, SCREEN_WIDTH, WHITE);

  display.setTextSize(1);
  oledTitle("Answer was:", 26, 1);

  display.setTextSize(2);
  int nameLen = strlen(COLOR_NAMES[targetIdx]);
  int x = (SCREEN_WIDTH - nameLen * 12) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, 38);
  display.print(COLOR_NAMES[targetIdx]);

  oledShow();
}

void showStopped() {
  oledClear();
  oledTitle("PAUSED", 10, 2);
  display.setTextSize(1);
  oledTitle("Send 'start'", 35, 1);
  oledTitle("to resume", 47, 1);

  char sc[16];
  sprintf(sc, "Score: %d", score);
  display.setCursor(0, 56);
  display.print(sc);

  oledShow();
}

// ─── LED HELPERS ─────────────────────────────────────────────

void setRing(int ring, uint32_t color) {
  if (ring < 5) {
    int s = ring * 12;
    for (int i = s; i < s + 12; i++) strip1.setPixelColor(i, color);
    strip1.show();
  } else {
    int s = (ring - 5) * 12;
    for (int i = s; i < s + 12; i++) strip2.setPixelColor(i, color);
    strip2.show();
  }
}

void clearAll() {
  strip1.clear(); strip2.clear();
  strip1.show();  strip2.show();
}

void setAll(uint32_t color) {
  for (int i = 0; i < NUM; i++) {
    strip1.setPixelColor(i, color);
    strip2.setPixelColor(i, color);
  }
  strip1.show(); strip2.show();
}

// ─── LEVEL SETUP ─────────────────────────────────────────────

/*
 * pickDistinct(n, exclude)
 * Returns an array of n distinct values from 0-9,
 * none of which equal 'exclude'.
 * Simple Fisher-Yates on a filtered list.
 */
void pickDistinct(int* out, int n, int exclude) {
  int pool[9];
  int poolSize = 0;
  for (int i = 0; i < 10; i++) {
    if (i != exclude) pool[poolSize++] = i;
  }
  // Shuffle
  for (int i = poolSize - 1; i > 0; i--) {
    int j = random(i + 1);
    int tmp = pool[i]; pool[i] = pool[j]; pool[j] = tmp;
  }
  for (int i = 0; i < n; i++) out[i] = pool[i];
}

void setupRound() {
  targetColor = random(0, 10);
  correctRing = random(0, 10);

  Serial.print(F("\n── Round "));
  Serial.print(++roundNum);
  Serial.print(F("  Level "));
  Serial.print(currentLevel);
  Serial.print(F("  Find: "));
  Serial.println(COLOR_NAMES[targetColor]);

  if (currentLevel == 1) {
    // ── LEVEL 1 ──────────────────────────────────────────────
    // Pick ONE wrong colour for all 9 other rings
    int wrongColor;
    do { wrongColor = random(0, 10); } while (wrongColor == targetColor);

    for (int i = 0; i < 10; i++)
      ringColorIdx[i] = (i == correctRing) ? targetColor : wrongColor;

    Serial.print(F("Wrong color: "));
    Serial.println(COLOR_NAMES[wrongColor]);

  } else if (currentLevel == 2) {
    // ── LEVEL 2 ──────────────────────────────────────────────
    // Pick 3 distinct wrong colours
    pickDistinct(wrongColorIdx, 3, targetColor);

    // Build a shuffled list of 9 wrong-ring slots → assign groups 3/3/3
    int slots[9];
    int si = 0;
    for (int i = 0; i < 10; i++) if (i != correctRing) slots[si++] = i;
    // Shuffle slots
    for (int i = 8; i > 0; i--) {
      int j = random(i + 1);
      int tmp = slots[i]; slots[i] = slots[j]; slots[j] = tmp;
    }
    // Assign colours to rings
    ringColorIdx[correctRing] = targetColor;
    for (int i = 0; i < 9; i++)
      ringColorIdx[slots[i]] = wrongColorIdx[i / 3];  // 3 per group

    Serial.print(F("Wrong groups: "));
    for (int g = 0; g < 3; g++) {
      Serial.print(COLOR_NAMES[wrongColorIdx[g]]);
      Serial.print(F(" "));
    }
    Serial.println();

  } else {
    // ── LEVEL 3 ──────────────────────────────────────────────
    // Each ring has a UNIQUE colour; the correct ring has targetColor.
    // The other 9 have 9 distinct wrong colours (all 9 remaining).
    int wrongColors[9];
    pickDistinct(wrongColors, 9, targetColor);  // all 9 others

    int wi = 0;
    for (int i = 0; i < 10; i++) {
      if (i == correctRing)
        ringColorIdx[i] = targetColor;
      else
        ringColorIdx[i] = wrongColors[wi++];
    }

    Serial.print(F("All ring colors: "));
    for (int i = 0; i < 10; i++) {
      Serial.print(i); Serial.print(F(":"));
      Serial.print(COLOR_NAMES[ringColorIdx[i]]);
      Serial.print(F(" "));
    }
    Serial.println();
  }

  // Light up all rings (steady — blink handled in loop for L1/L2)
  showStaticRings();

  // OLED
  showFind(targetColor, currentLevel);

  // Reset round timing
  roundStart = millis();
  answered   = false;
  blinkOn    = true;
  lastBlink  = millis();
}

// Show rings in their current solid state
void showStaticRings() {
  for (int i = 0; i < 10; i++) {
    if (currentLevel == 3 || i == correctRing) {
      // Level 3: all steady; or correct ring: always steady
      setRing(i, getColor(ringColorIdx[i]));
    } else {
      // L1/L2 wrong rings: start ON (blink will toggle)
      setRing(i, getColor(ringColorIdx[i]));
    }
  }
}

// ─── SERIAL COMMAND HANDLER ──────────────────────────────────

void printHelp() {
  Serial.println(F("─────────────────────────────────────────"));
  Serial.println(F(" Commands:"));
  Serial.println(F("  start     – start / resume game"));
  Serial.println(F("  stop      – pause game"));
  Serial.println(F("  level 1   – set level 1 (easy)"));
  Serial.println(F("  level 2   – set level 2 (medium)"));
  Serial.println(F("  level 3   – set level 3 (hard)"));
  Serial.println(F("  reset     – clear score & restart"));
  Serial.println(F("  status    – show current state"));
  Serial.println(F("─────────────────────────────────────────"));
}

void printStatus() {
  Serial.print(F("Running: ")); Serial.println(gameRunning ? "YES" : "NO");
  Serial.print(F("Level  : ")); Serial.println(currentLevel);
  Serial.print(F("Score  : ")); Serial.println(score);
  Serial.print(F("Round  : ")); Serial.println(roundNum);
}

void handleSerial(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "start") {
    if (!gameRunning) {
      gameRunning = true;
      Serial.println(F("▶ Game started."));
      showLevelBanner(currentLevel);
      setupRound();
    } else {
      Serial.println(F("Already running."));
    }

  } else if (cmd == "stop") {
    gameRunning = false;
    clearAll();
    showStopped();
    Serial.println(F("⏸ Game paused."));

  } else if (cmd == "level 1" || cmd == "l1") {
    currentLevel = 1;
    Serial.println(F("Level set to 1 (Easy)."));
    if (gameRunning) { showLevelBanner(1); setupRound(); }

  } else if (cmd == "level 2" || cmd == "l2") {
    currentLevel = 2;
    Serial.println(F("Level set to 2 (Medium)."));
    if (gameRunning) { showLevelBanner(2); setupRound(); }

  } else if (cmd == "level 3" || cmd == "l3") {
    currentLevel = 3;
    Serial.println(F("Level set to 3 (Hard)."));
    if (gameRunning) { showLevelBanner(3); setupRound(); }

  } else if (cmd == "reset") {
    score    = 0;
    roundNum = 0;
    gameRunning = false;
    clearAll();
    showSplash();
    Serial.println(F("🔄 Score reset. Send 'start' to play."));

  } else if (cmd == "status") {
    printStatus();

  } else if (cmd == "help" || cmd == "?") {
    printHelp();

  } else {
    Serial.print(F("Unknown command: '"));
    Serial.print(cmd);
    Serial.println(F("'  — send 'help' for command list."));
  }
}

// ─── SETUP ───────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(500);

  strip1.begin(); strip2.begin();
  strip1.setBrightness(200);
  strip2.setBrightness(200);
  clearAll();

  // OLED
  Wire.begin(8, 9);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED FAILED"));
    while (true);
  }
  display.clearDisplay();
  display.display();

  // Buttons
  for (int i = 0; i < 10; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    lastButtonState[i] = HIGH;
  }

  randomSeed(analogRead(0));

  showSplash();
  printHelp();
  Serial.println(F("Send 'start' to begin."));
}

// ─── MAIN LOOP ───────────────────────────────────────────────

void loop() {

  // ── Read serial ──────────────────────────────────────────
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuf.length() > 0) {
        handleSerial(serialBuf);
        serialBuf = "";
      }
    } else {
      serialBuf += c;
    }
  }

  if (!gameRunning) return;

  // ── Blink wrong rings (Level 1 & 2 only) ─────────────────
  if (currentLevel < 3) {
    if (millis() - lastBlink >= BLINK_INTERVAL) {
      lastBlink = millis();
      blinkOn = !blinkOn;

      for (int i = 0; i < 10; i++) {
        if (i == correctRing) {
          // Correct ring ALWAYS stays on steady — never blinks
          setRing(i, getColor(ringColorIdx[i]));
        } else {
          setRing(i, blinkOn ? getColor(ringColorIdx[i]) : (uint32_t)0);
        }
      }
    }
  }

  // ── Update timer bar on OLED ─────────────────────────────
  unsigned long elapsed = millis() - roundStart;
  if (elapsed < ROUND_TIME) {
    // Update every 100 ms to avoid flickering too much
    static unsigned long lastTimerUpdate = 0;
    if (millis() - lastTimerUpdate > 100) {
      lastTimerUpdate = millis();
      updateTimerBar(elapsed, ROUND_TIME);
    }
  }

  // ── Check buttons ─────────────────────────────────────────
  if (!answered) {
    for (int i = 0; i < 10; i++) {
      bool cur = digitalRead(BUTTON_PINS[i]);

      if (lastButtonState[i] == HIGH && cur == LOW) {
        // Button pressed
        answered = true;

        if (i == correctRing) {
          // ✅ CORRECT
          score++;
          Serial.print(F("✅ CORRECT!  Score: "));
          Serial.println(score);
          showResult(targetColor, true);

          // Flash correct ring white 3 times
          for (int f = 0; f < 3; f++) {
            setRing(correctRing, strip1.Color(255, 255, 255));
            delay(150);
            setRing(correctRing, getColor(targetColor));
            delay(150);
          }

        } else {
          // ❌ WRONG
          Serial.print(F("❌ WRONG!  Correct was ring "));
          Serial.print(correctRing);
          Serial.print(F(" → "));
          Serial.println(COLOR_NAMES[targetColor]);

          showResult(targetColor, false);

          // Blink ALL rings in the target color 4 times
          for (int b = 0; b < 4; b++) {
            setAll(getColor(targetColor));
            delay(250);
            clearAll();
            delay(250);
          }
        }

        delay(900);
      }
      lastButtonState[i] = cur;
    }
  }

  // ── Round timeout ─────────────────────────────────────────
  if (!answered && (millis() - roundStart >= ROUND_TIME)) {
    answered = true;
    Serial.print(F("⏱ TIME UP!  Correct was: "));
    Serial.println(COLOR_NAMES[targetColor]);

    showMiss(targetColor);

    // Pulse correct ring rapidly
    for (int p = 0; p < 5; p++) {
      setRing(correctRing, getColor(targetColor));
      delay(200);
      clearAll();
      delay(200);
    }

    delay(600);
  }

  // ── Start next round after answer / timeout ───────────────
  if (answered) {
    clearAll();
    delay(300);
    setupRound();   // continuous — go straight to next round
  }
}
