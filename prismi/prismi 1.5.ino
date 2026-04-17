/*
 * ╔══════════════════════════════════════════════╗
 * ║        PRISMI V1.5 — Advanced Edition        ║
 * ║     Color Learning Board · Single Strip      ║
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

// ── Difficulty ───────────────────────────────
#define DISTRACTORS_EASY   1   // 2 rings lit total
#define DISTRACTORS_MEDIUM 3   // 4 rings lit total
#define DISTRACTORS_HARD   5   // 6 rings lit total
int numDistractors = DISTRACTORS_EASY;

// ── Strip mode ───────────────────────────────
#define TWO_STRIPS         1   // 1 = split across two pins

#define DATA_PIN_A         4
#define DATA_PIN_B         5

// ── Buttons ──────────────────────────────────
const int BUTTONS[NUM_RINGS] = {35, 36, 37, 38, 39, 40, 41, 42, 15, 16};

// ── Timing ───────────────────────────────────
#define ROUND_TIMEOUT_MS   8000
#define DEBOUNCE_MS        50
#define BREATHE_STEP_MS    15

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

int  targetRing      = 0;
int  activeRings[10];
int  activeCount     = 0;
int  score           = 0;
int  currentRound    = 0;
int  maxRounds       = 10;
unsigned long roundStart = 0;

// For breathing effect
unsigned long lastBreathe = 0;
int  breatheBrightness = 180;
int  breatheDir = -3;

// ─────────────────────────────────────────────
//  FUNCTION PROTOTYPES
// ─────────────────────────────────────────────
void handleSerial();
void newRound();
void checkButtons();
void onCorrect();
void onWrong(int pressed);
void onTimeout();
void lightActiveRings(uint8_t brightness = 180);
void lightRing(int ringIndex, uint8_t brightness = 180);
void flashRing(int ringIndex, uint8_t r, uint8_t g, uint8_t b, int times, int delayMs = 120);
void allOff();
void setPixel(int absoluteIndex, uint8_t r, uint8_t g, uint8_t b);
void stripShow();
void stripClear();
void setGlobalBrightness(uint8_t b);

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
  stripA.setBrightness(180);
  stripA.show();

  #if TWO_STRIPS
    stripB.begin();
    stripB.setBrightness(180);
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
      // Breathing effect on active rings
      if (millis() - lastBreathe > BREATHE_STEP_MS) {
        breatheBrightness += breatheDir;
        if (breatheBrightness <= 100 || breatheBrightness >= 220) {
          breatheDir = -breatheDir;
        }
        setGlobalBrightness(breatheBrightness);
        lastBreathe = millis();
      }
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
      Serial.printf("Current distractors: %d. Use: diff 1|2|3\n", numDistractors);
      return;
    }
    int d = cmd.substring(space + 1).toInt();
    if (d == 1)      { numDistractors = DISTRACTORS_EASY;   Serial.println("Difficulty: Easy"); }
    else if (d == 2) { numDistractors = DISTRACTORS_MEDIUM; Serial.println("Difficulty: Medium"); }
    else if (d == 3) { numDistractors = DISTRACTORS_HARD;   Serial.println("Difficulty: Hard"); }
    else Serial.println("Usage: diff 1|2|3");

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

  targetRing = random(NUM_RINGS);

  bool used[NUM_RINGS] = {false};
  used[targetRing] = true;
  activeRings[0] = targetRing;
  activeCount = 1;

  int attempts = 0;
  while (activeCount <= numDistractors && attempts < 50) {
    int r = random(NUM_RINGS);
    if (!used[r]) {
      used[r] = true;
      activeRings[activeCount++] = r;
    }
    attempts++;
  }

  // Light all active rings at full brightness initially
  lightActiveRings(220);
  breatheBrightness = 220;  // start breathing from here

  if (oledAvailable) {
    oledPrompt(COLORS[targetRing].name, currentRound, maxRounds, score);
  }

  #if SERIAL_DEBUG
  Serial.printf("Round %d/%d – Find: %s\n", currentRound, maxRounds, COLORS[targetRing].name);
  #endif

  roundStart = millis();
  gameState = WAITING;
}

void checkButtons() {
  for (int i = 0; i < NUM_RINGS; i++) {
    if (digitalRead(BUTTONS[i]) == LOW) {
      delay(DEBOUNCE_MS);
      if (digitalRead(BUTTONS[i]) == LOW) {
        if (i == targetRing) onCorrect();
        else                 onWrong(i);
        return;
      }
    }
  }

  if (millis() - roundStart > ROUND_TIMEOUT_MS) {
    onTimeout();
  }
}

void onCorrect() {
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

  // Flash target ring in its own colour
  flashRing(targetRing,
            COLORS[targetRing].r,
            COLORS[targetRing].g,
            COLORS[targetRing].b, 4, 100);

  allOff();
  gameState = NEW_ROUND;
}

void onWrong(int pressed) {
  gameState = FEEDBACK;
  #if SERIAL_DEBUG
  Serial.printf("Wrong! Pressed %s, needed %s\n", COLORS[pressed].name, COLORS[targetRing].name);
  #endif

  if (oledAvailable) oledFeedback(false, score, true);

  // Buzzer: low tone
  tone(PIEZO_PIN, 300, 400); delay(450);
  noTone(PIEZO_PIN);

  // Flash the wrongly pressed ring red
  flashRing(pressed, 255, 0, 0, 2, 150);

  // Hint: blink all active rings twice
  wrongHintAnimation();

  // Restore the round (active rings lit again, breathing)
  lightActiveRings(220);
  breatheBrightness = 220;
  if (oledAvailable) oledPrompt(COLORS[targetRing].name, currentRound, maxRounds, score);
  roundStart = millis();
  gameState = WAITING;
}

void onTimeout() {
  gameState = FEEDBACK;
  #if SERIAL_DEBUG
  Serial.printf("Timeout! The color was %s\n", COLORS[targetRing].name);
  #endif

  for (int i = 0; i < 3; i++) {
    tone(PIEZO_PIN, 400, 100); delay(150);
  }
  noTone(PIEZO_PIN);

  // Blink all rings red three times
  for (int t = 0; t < 3; t++) {
    for (int i = 0; i < NUM_RINGS; i++) {
      int start = i * LEDS_PER_RING;
      for (int j = 0; j < LEDS_PER_RING; j++) setPixel(start + j, 200, 0, 0);
    }
    stripShow();
    delay(200);
    allOff();
    delay(150);
  }

  // Show correct ring briefly
  lightRing(targetRing, 200);
  delay(800);
  allOff();

  gameState = NEW_ROUND;
}

// ─────────────────────────────────────────────
//  LED EFFECTS
// ─────────────────────────────────────────────
void lightActiveRings(uint8_t brightness) {
  stripClear();
  for (int a = 0; a < activeCount; a++) {
    int ring = activeRings[a];
    int start = ring * LEDS_PER_RING;
    uint8_t r = (COLORS[ring].r * brightness) / 255;
    uint8_t g = (COLORS[ring].g * brightness) / 255;
    uint8_t b = (COLORS[ring].b * brightness) / 255;
    for (int i = 0; i < LEDS_PER_RING; i++) {
      setPixel(start + i, r, g, b);
    }
  }
  stripShow();
}

void lightRing(int ringIndex, uint8_t brightness) {
  stripClear();
  int start = ringIndex * LEDS_PER_RING;
  uint8_t r = (COLORS[ringIndex].r * brightness) / 255;
  uint8_t g = (COLORS[ringIndex].g * brightness) / 255;
  uint8_t b = (COLORS[ringIndex].b * brightness) / 255;
  for (int i = 0; i < LEDS_PER_RING; i++) {
    setPixel(start + i, r, g, b);
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

void setGlobalBrightness(uint8_t b) {
  // Adjust brightness of currently lit pixels (active rings)
  for (int a = 0; a < activeCount; a++) {
    int ring = activeRings[a];
    int start = ring * LEDS_PER_RING;
    uint8_t r = (COLORS[ring].r * b) / 255;
    uint8_t g = (COLORS[ring].g * b) / 255;
    uint8_t bl = (COLORS[ring].b * b) / 255;
    for (int i = 0; i < LEDS_PER_RING; i++) {
      setPixel(start + i, r, g, bl);
    }
  }
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
  // Blink all active rings twice
  for (int t = 0; t < 2; t++) {
    allOff();
    delay(150);
    lightActiveRings(220);
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
    lightRing(i, 200);
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
  Serial.println("║   PRISMI v2.0  Serial Cmd    ║");
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
