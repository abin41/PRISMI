/*
 * ╔══════════════════════════════════════════════╗
 * ║        PRISMI v1.8 — Friendly         ║
 * ║     Color Learning Board · All Rings Lit     ║
 * ╠══════════════════════════════════════════════╣
 * ║  Wiring:                                     ║
 * ║   GPIO 4  → Ring 0 DIN                       ║
 * ║   Ring 0 DOUT → Ring 1 DIN → ... → Ring 9   ║
 * ║   (all rings chained on one data pin)        ║
 * ║                                              ║
 * ║   OR split 5+5:                              ║
 * ║   GPIO 4 → Rings 0–4                         ║
 * ║   GPIO 5 → Rings 5–9                         ║
 * ║   (set TWO_STRIPS 1 below)                   ║
 * ║                                              ║
 * ║   OLED SDA → GPIO 19                         ║
 * ║   OLED SCL → GPIO 20                         ║
 * ╠══════════════════════════════════════════════╣
 * ║  Serial commands (115200 baud):              ║
 * ║   start     → begin game                    ║
 * ║   stop      → end game                      ║
 * ║   diff 1|2|3→ Easy/Medium/Hard              ║
 * ║   rounds N  → set number of rounds          ║
 * ║   test      → run LED/OLED test             ║
 * ║   help      → show commands                 ║
 * ╚══════════════════════════════════════════════╝
 */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─────────────────────────────────────────────
//  CONFIGURATION
// ─────────────────────────────────────────────
#define NUM_RINGS          10
#define LEDS_PER_RING      12
#define PIEZO_PIN          25

// ── OLED ─────────────────────────────────────
#define OLED_WIDTH         128
#define OLED_HEIGHT        64
#define OLED_RESET         -1
#define OLED_SDA           19
#define OLED_SCL           20

// ── Difficulty levels (v1.7) ─────────────────
// Level 1: 1 ring colour A, 9 rings colour B. Target can be A or B.
// Level 2: 3 rings colour A, 3 rings colour B, 3 rings colour C, 1 ring colour D. Target can be any.
// Level 3: All 10 rings different (each ring shows its own colour).
int difficulty = 1;   // 1=easy, 2=medium, 3=hard

// ── Strip mode ───────────────────────────────
#define TWO_STRIPS         1   // 1 = split across two pins

#define DATA_PIN_A         4
#define DATA_PIN_B         5

// ── Buttons ──────────────────────────────────
const int BUTTONS[NUM_RINGS] = {35, 36, 37, 38, 39, 40, 41, 42, 15, 16};

// ── Timing ───────────────────────────────────
#define ROUND_TIMEOUT_MS   8000
#define DEBOUNCE_MS        50

// ── Debug ────────────────────────────────────
#define SERIAL_DEBUG       0   // set to 1 for verbose logs

// ─────────────────────────────────────────────
//  COLORS (fixed per ring)
// ─────────────────────────────────────────────
struct Color { const char* name; uint8_t r, g, b; };

Color COLORS[NUM_RINGS] = {
  { "RED",    255,   0,   0 },
  { "GREEN",    0, 220,   0 },
  { "BLUE",     0, 120, 255 },
  { "YELLOW", 255, 220,   0 },
  { "ORANGE", 255, 100,   0 },
  { "PURPLE", 180,   0, 255 },
  { "PINK",   255,  50, 150 },
  { "TEAL",     0, 200, 180 },
  { "CYAN",     0, 220, 255 },
  { "LIME",   130, 255,   0 },
};

// ─────────────────────────────────────────────
//  STRIP OBJECTS
// ─────────────────────────────────────────────
#if TWO_STRIPS
  #define LEDS_PER_STRIP (5 * LEDS_PER_RING)
  Adafruit_NeoPixel stripA(LEDS_PER_STRIP, DATA_PIN_A, NEO_GRB + NEO_KHZ800);
  Adafruit_NeoPixel stripB(LEDS_PER_STRIP, DATA_PIN_B, NEO_GRB + NEO_KHZ800);
#else
  #define TOTAL_LEDS (NUM_RINGS * LEDS_PER_RING)
  Adafruit_NeoPixel stripA(TOTAL_LEDS, DATA_PIN_A, NEO_GRB + NEO_KHZ800);
#endif

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
bool oledAvailable = false;

// ─────────────────────────────────────────────
//  GAME STATE
// ─────────────────────────────────────────────
enum State { IDLE, NEW_ROUND, WAITING, FEEDBACK };
State gameState = IDLE;

int  targetColorIdx  = 0;            // the colour the child is asked to find
int  ringColorIdx[10];               // colour index assigned to each ring this round
int  score           = 0;
int  currentRound    = 0;
int  maxRounds       = 10;
unsigned long roundStart = 0;

// ─────────────────────────────────────────────
//  FUNCTION PROTOTYPES
// ─────────────────────────────────────────────
void handleSerial();
void newRound();
void checkButtons();
void onCorrect(int pressedRing);
void onWrong(int pressed);
void onTimeout();
void lightAllRings();
void lightRing(int ringIndex);
void flashRing(int ringIndex, uint8_t r, uint8_t g, uint8_t b, int times, int delayMs = 120);
void allOff();
void setPixel(int absoluteIndex, uint8_t r, uint8_t g, uint8_t b);
void stripShow();
void stripClear();

void oledPrompt(const char* colorName, int roundNum, int maxR, int sc);
void oledFeedback(bool correct, int sc, bool showHint = true);
void oledGameOver(int sc, int total);
void oledIdle();
void oledTestScreen(const char* line1, const char* line2 = nullptr);

void victorySweep();
void wrongHintAnimation();
void idleRainbow();

void runTest();
void printHelp();

// ─────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_RINGS; i++) {
    pinMode(BUTTONS[i], INPUT_PULLUP);
  }
  pinMode(PIEZO_PIN, OUTPUT);

  stripA.begin();
  stripA.setBrightness(255);   // MAX brightness
  stripA.show();

  #if TWO_STRIPS
    stripB.begin();
    stripB.setBrightness(255);
    stripB.show();
  #endif

  randomSeed(analogRead(0));

  // ── OLED initialisation ────────────────────
  Wire.begin(OLED_SDA, OLED_SCL);
  delay(50);

  // Try common I²C addresses
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledAvailable = true;
  } else if (display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    oledAvailable = true;
  }

  if (oledAvailable) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.display();
    oledIdle();
  } else {
    Serial.println("[WARN] OLED not found – running headless.");
  }

  printHelp();
  idleRainbow();   // show something pretty on the rings
}

// ─────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────
void loop() {
  handleSerial();

  switch (gameState) {
    case IDLE:
      // Update idle rainbow every 50ms
      static unsigned long lastIdle = 0;
      if (millis() - lastIdle > 50) {
        idleRainbow();
        lastIdle = millis();
      }
      break;

    case NEW_ROUND:
      newRound();
      break;

    case WAITING:
      checkButtons();
      break;

    case FEEDBACK:
      // Handled inside feedback functions; they set state back to NEW_ROUND or IDLE
      break;
  }
}

// ─────────────────────────────────────────────
//  SERIAL COMMANDS (minimal output)
// ─────────────────────────────────────────────
void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "start") {
    if (gameState != IDLE) {
      Serial.println("Already running. 'stop' first.");
      return;
    }
    score = 0;
    currentRound = 0;
    gameState = NEW_ROUND;
    Serial.printf("Game started! %d rounds.\n", maxRounds);

  } else if (cmd == "stop") {
    gameState = IDLE;
    allOff();
    if (oledAvailable) oledIdle();
    Serial.println("Stopped.");

  } else if (cmd.startsWith("diff")) {
    int space = cmd.indexOf(' ');
    if (space == -1) {
      Serial.printf("Current difficulty: %d. Use: diff 1|2|3\n", difficulty);
      return;
    }
    int d = cmd.substring(space + 1).toInt();
    if (d >= 1 && d <= 3) {
      difficulty = d;
      const char* levelNames[] = {"Easy (1 vs 9)", "Medium (3-3-3-1)", "Hard (all unique)"};
      Serial.printf("Difficulty: %s\n", levelNames[d-1]);
    } else {
      Serial.println("Usage: diff 1|2|3");
    }

  } else if (cmd.startsWith("rounds")) {
    int n = cmd.substring(7).toInt();
    if (n > 0) { maxRounds = n; Serial.printf("Rounds set to %d\n", n); }
    else Serial.println("Usage: rounds 10");

  } else if (cmd == "test") {
    if (gameState != IDLE) {
      Serial.println("Stop the game first.");
      return;
    }
    runTest();

  } else if (cmd == "help") {
    printHelp();
  }
  #if SERIAL_DEBUG
  else if (cmd.length() > 0) {
    Serial.printf("[DEBUG] Unknown: '%s'\n", cmd.c_str());
  }
  #endif
}

// ─────────────────────────────────────────────
//  GAME LOGIC
// ─────────────────────────────────────────────
void newRound() {
  currentRound++;
  if (currentRound > maxRounds) {
    gameState = IDLE;
    allOff();
    if (oledAvailable) oledGameOver(score, maxRounds);
    Serial.println("══════════════════════");
    Serial.printf("GAME OVER! Score: %d/%d\n", score, maxRounds);
    Serial.println("══════════════════════");
    idleRainbow();
    return;
  }

  // Step 1: Decide target colour index (0-9)
  targetColorIdx = random(NUM_RINGS);

  // Step 2: Build ring colour assignments based on difficulty
  if (difficulty == 1) {
    // Level 1: 1 ring of colour A, 9 rings of colour B
    // Choose two distinct colours: targetColorIdx and a distractor
    int distractorColor;
    do {
      distractorColor = random(NUM_RINGS);
    } while (distractorColor == targetColorIdx);

    // Randomly decide if target is the singleton (1 ring) or majority (9 rings)
    bool targetIsSingleton = random(2) == 0; // 50% chance

    if (targetIsSingleton) {
      // Target colour appears on exactly one ring
      int targetRingPos = random(NUM_RINGS);
      for (int i = 0; i < NUM_RINGS; i++) {
        ringColorIdx[i] = (i == targetRingPos) ? targetColorIdx : distractorColor;
      }
    } else {
      // Target colour appears on 9 rings; one ring gets the distractor colour
      int oddRing = random(NUM_RINGS);
      for (int i = 0; i < NUM_RINGS; i++) {
        ringColorIdx[i] = (i == oddRing) ? distractorColor : targetColorIdx;
      }
    }
  }
  else if (difficulty == 2) {
    // Level 2: 3 rings of colour A, 3 of B, 3 of C, 1 of D
    // Target colour is targetColorIdx. That colour could be A, B, C, or D.
    // Pick three distractor colours distinct from target and each other
    int d1, d2, d3;
    do { d1 = random(NUM_RINGS); } while (d1 == targetColorIdx);
    do { d2 = random(NUM_RINGS); } while (d2 == targetColorIdx || d2 == d1);
    do { d3 = random(NUM_RINGS); } while (d3 == targetColorIdx || d3 == d1 || d3 == d2);

    // Decide how many times target appears: 3 or 1
    bool targetIsTriplet = random(2) == 0;
    int targetCount = targetIsTriplet ? 3 : 1;

    // Build an array of colour assignments for the 10 rings, then shuffle
    int assignments[10];
    int idx = 0;

    // Add target colour 'targetCount' times
    for (int i = 0; i < targetCount; i++) assignments[idx++] = targetColorIdx;

    // The remaining distractor colours
    if (targetCount == 3) {
      for (int i = 0; i < 3; i++) assignments[idx++] = d1;
      for (int i = 0; i < 3; i++) assignments[idx++] = d2;
      assignments[idx++] = d3; // singleton
    } else { // targetCount == 1
      for (int i = 0; i < 3; i++) assignments[idx++] = d1;
      for (int i = 0; i < 3; i++) assignments[idx++] = d2;
      for (int i = 0; i < 3; i++) assignments[idx++] = d3;
    }

    // Shuffle the assignments array
    for (int i = 0; i < 10; i++) {
      int j = random(10);
      int temp = assignments[i];
      assignments[i] = assignments[j];
      assignments[j] = temp;
    }

    // Apply to rings
    for (int i = 0; i < NUM_RINGS; i++) {
      ringColorIdx[i] = assignments[i];
    }
  }
  else { // difficulty == 3
    // Level 3: every ring shows its own unique colour
    for (int i = 0; i < NUM_RINGS; i++) {
      ringColorIdx[i] = i;
    }
  }

  // Light all rings
  lightAllRings();

  if (oledAvailable) {
    oledPrompt(COLORS[targetColorIdx].name, currentRound, maxRounds, score);
  }

  #if SERIAL_DEBUG
  Serial.printf("Round %d/%d – Find: %s\n", currentRound, maxRounds, COLORS[targetColorIdx].name);
  #endif

  roundStart = millis();
  gameState = WAITING;
}

void checkButtons() {
  for (int i = 0; i < NUM_RINGS; i++) {
    if (digitalRead(BUTTONS[i]) == LOW) {
      delay(DEBOUNCE_MS);
      if (digitalRead(BUTTONS[i]) == LOW) {
        // Validate based on colour, not specific ring
        if (ringColorIdx[i] == targetColorIdx) {
          onCorrect(i);
        } else {
          onWrong(i);
        }
        return;
      }
    }
  }

  if (millis() - roundStart > ROUND_TIMEOUT_MS) {
    onTimeout();
  }
}

void onCorrect(int pressedRing) {
  gameState = FEEDBACK;
  score++;
  #if SERIAL_DEBUG
  Serial.printf("Correct! Score: %d\n", score);
  #endif

  if (oledAvailable) oledFeedback(true, score, false);

  // Victory sound
  tone(PIEZO_PIN, 880, 120); delay(140);
  tone(PIEZO_PIN, 1320, 200); delay(220);
  noTone(PIEZO_PIN);

  // Animated sweep across all rings
  victorySweep();

  // Flash the pressed ring (which is correct) in its colour
  flashRing(pressedRing,
            COLORS[targetColorIdx].r,
            COLORS[targetColorIdx].g,
            COLORS[targetColorIdx].b, 4, 100);

  allOff();
  gameState = NEW_ROUND;
}

void onWrong(int pressed) {
  gameState = FEEDBACK;
  #if SERIAL_DEBUG
  Serial.printf("Wrong! Pressed %s, needed %s\n", COLORS[ringColorIdx[pressed]].name, COLORS[targetColorIdx].name);
  #endif

  if (oledAvailable) oledFeedback(false, score, true);

  // Buzzer: low tone
  tone(PIEZO_PIN, 300, 400); delay(450);
  noTone(PIEZO_PIN);

  // Flash the wrongly pressed ring red
  flashRing(pressed, 255, 0, 0, 2, 150);

  // Hint: blink all rings off/on twice (keeping their assigned colours)
  wrongHintAnimation();

  // Restore the round (all rings lit again)
  lightAllRings();
  if (oledAvailable) oledPrompt(COLORS[targetColorIdx].name, currentRound, maxRounds, score);
  roundStart = millis();
  gameState = WAITING;
}

void onTimeout() {
  gameState = FEEDBACK;
  #if SERIAL_DEBUG
  Serial.printf("Timeout! The color was %s\n", COLORS[targetColorIdx].name);
  #endif

  for (int i = 0; i < 3; i++) {
    tone(PIEZO_PIN, 400, 100); delay(150);
  }
  noTone(PIEZO_PIN);

  // Blink all rings that have the target colour
  for (int t = 0; t < 3; t++) {
    stripClear();
    for (int i = 0; i < NUM_RINGS; i++) {
      if (ringColorIdx[i] == targetColorIdx) {
        int start = i * LEDS_PER_RING;
        for (int j = 0; j < LEDS_PER_RING; j++) {
          setPixel(start + j, COLORS[targetColorIdx].r, COLORS[targetColorIdx].g, COLORS[targetColorIdx].b);
        }
      }
    }
    stripShow();
    delay(300);
    allOff();
    delay(200);
  }

  gameState = NEW_ROUND;
}

// ─────────────────────────────────────────────
//  LED EFFECTS
// ─────────────────────────────────────────────
void lightAllRings() {
  stripClear();
  for (int i = 0; i < NUM_RINGS; i++) {
    int colorIdx = ringColorIdx[i];
    int start = i * LEDS_PER_RING;
    for (int j = 0; j < LEDS_PER_RING; j++) {
      setPixel(start + j, COLORS[colorIdx].r, COLORS[colorIdx].g, COLORS[colorIdx].b);
    }
  }
  stripShow();
}

void lightRing(int ringIndex) {
  stripClear();
  int start = ringIndex * LEDS_PER_RING;
  int colorIdx = ringColorIdx[ringIndex];
  for (int i = 0; i < LEDS_PER_RING; i++) {
    setPixel(start + i,
             COLORS[colorIdx].r,
             COLORS[colorIdx].g,
             COLORS[colorIdx].b);
  }
  stripShow();
}

void flashRing(int ringIndex, uint8_t r, uint8_t g, uint8_t b, int times, int delayMs) {
  int start = ringIndex * LEDS_PER_RING;
  for (int t = 0; t < times; t++) {
    for (int i = 0; i < LEDS_PER_RING; i++) setPixel(start + i, r, g, b);
    stripShow();
    delay(delayMs);
    for (int i = 0; i < LEDS_PER_RING; i++) setPixel(start + i, 0, 0, 0);
    stripShow();
    if (t < times - 1) delay(delayMs / 2);
  }
}

void allOff() {
  stripClear();
  stripShow();
}

void victorySweep() {
  // Wave of white light across all rings
  for (int pos = 0; pos < NUM_RINGS * LEDS_PER_RING + LEDS_PER_RING; pos++) {
    stripClear();
    for (int i = 0; i < NUM_RINGS * LEDS_PER_RING; i++) {
      if (i <= pos && i > pos - LEDS_PER_RING) {
        setPixel(i, 255, 255, 255);
      }
    }
    stripShow();
    delay(20);
  }
  allOff();
}

void wrongHintAnimation() {
  // Blink all rings off/on twice
  for (int t = 0; t < 2; t++) {
    allOff();
    delay(150);
    lightAllRings();
    delay(250);
  }
}

void idleRainbow() {
  static unsigned long lastUpdate = 0;
  static uint16_t hue = 0;
  if (millis() - lastUpdate > 30) {
    for (int i = 0; i < NUM_RINGS * LEDS_PER_RING; i++) {
      uint32_t color = stripA.gamma32(stripA.ColorHSV(hue + i * 256));
      setPixel(i, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    }
    stripShow();
    hue += 256;
    lastUpdate = millis();
  }
}

// ─────────────────────────────────────────────
//  STRIP ABSTRACTION
// ─────────────────────────────────────────────
void setPixel(int absoluteIndex, uint8_t r, uint8_t g, uint8_t b) {
  #if TWO_STRIPS
    int half = 5 * LEDS_PER_RING;
    if (absoluteIndex < half) {
      stripA.setPixelColor(absoluteIndex, stripA.Color(r, g, b));
    } else {
      stripB.setPixelColor(absoluteIndex - half, stripB.Color(r, g, b));
    }
  #else
    stripA.setPixelColor(absoluteIndex, stripA.Color(r, g, b));
  #endif
}

void stripShow() {
  stripA.show();
  #if TWO_STRIPS
    stripB.show();
  #endif
}

void stripClear() {
  stripA.clear();
  #if TWO_STRIPS
    stripB.clear();
  #endif
}

// ─────────────────────────────────────────────
//  OLED SCREENS
// ─────────────────────────────────────────────
void oledIdle() {
  if (!oledAvailable) return;
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(18, 4);
  display.println("PRISMI");
  display.setTextSize(1);
  display.setCursor(0, 30);
  display.println("Type 'start' in");
  display.println("serial to begin.");
  display.display();
}

void oledPrompt(const char* colorName, int roundNum, int maxR, int sc) {
  if (!oledAvailable) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("R:%d/%d", roundNum, maxR);
  display.setCursor(90, 0);
  display.printf("Score:%d", sc);
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 14);
  display.println("Find the color:");

  display.setTextSize(3);
  int nameLen = strlen(colorName);
  int x = max(0, (128 - nameLen * 18) / 2);
  display.setCursor(x, 28);
  display.println(colorName);

  display.display();
}

void oledFeedback(bool correct, int sc, bool showHint) {
  if (!oledAvailable) return;
  display.clearDisplay();
  display.setTextSize(3);
  if (correct) {
    display.setCursor(22, 10);
    display.println("YES!");
  } else {
    display.setCursor(10, 10);
    display.println("NOPE");
  }
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.printf("Score: %d", sc);
  if (showHint) {
    display.setCursor(64, 50);
    display.println("Hint shown");
  }
  display.display();
}

void oledGameOver(int sc, int total) {
  if (!oledAvailable) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(22, 0);
  display.println("GAME OVER!");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  display.setTextSize(3);
  String s = String(sc) + "/" + String(total);
  int x = max(0, (128 - (int)s.length() * 18) / 2);
  display.setCursor(x, 16);
  display.println(s);
  display.setTextSize(1);
  display.setCursor(0, 52);
  float pct = (float)sc / total;
  if      (pct == 1.0f) display.println("Perfect! Amazing!");
  else if (pct >= 0.7f) display.println("Great job!");
  else if (pct >= 0.4f) display.println("Good try!");
  else                  display.println("Keep practicing!");
  display.display();
}

void oledTestScreen(const char* line1, const char* line2) {
  if (!oledAvailable) return;
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.println(line1);
  if (line2) {
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println(line2);
  }
  display.display();
}

// ─────────────────────────────────────────────
//  TEST MODE
// ─────────────────────────────────────────────
void runTest() {
  Serial.println("Running hardware test...");

  // OLED test
  if (oledAvailable) {
    oledTestScreen("OLED TEST", "If you see this, OLED works!");
    delay(1500);
  } else {
    Serial.println("OLED not detected.");
  }

  // LED test: each ring individually
  for (int i = 0; i < NUM_RINGS; i++) {
    oledTestScreen(COLORS[i].name);
    lightRing(i);
    delay(500);
  }

  // Rainbow chase
  oledTestScreen("RAINBOW", "Chase effect");
  for (int pos = 0; pos < NUM_RINGS * LEDS_PER_RING + LEDS_PER_RING; pos++) {
    stripClear();
    for (int i = 0; i < NUM_RINGS * LEDS_PER_RING; i++) {
      if (i <= pos && i > pos - LEDS_PER_RING) {
        uint32_t color = stripA.ColorHSV((i * 65536L) / (NUM_RINGS * LEDS_PER_RING));
        setPixel(i, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
      }
    }
    stripShow();
    delay(20);
  }

  allOff();
  oledIdle();
  idleRainbow();
  Serial.println("Test complete.");
}

// ─────────────────────────────────────────────
//  HELP
// ─────────────────────────────────────────────
void printHelp() {
  Serial.println();
  Serial.println("╔══════════════════════════════╗");
  Serial.println("║   PRISMI v1.8  Serial Cmd    ║");
  Serial.println("╠══════════════════════════════╣");
  Serial.println("║  start       Start game      ║");
  Serial.println("║  stop        Stop game       ║");
  Serial.println("║  rounds N    Set rounds      ║");
  Serial.println("║  diff 1|2|3  Easy/Med/Hard   ║");
  Serial.println("║  test        Run self-test   ║");
  Serial.println("║  help        This menu       ║");
  Serial.println("╚══════════════════════════════╝");
  Serial.println();
}
