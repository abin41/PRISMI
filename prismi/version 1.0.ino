#include <Adafruit_NeoPixel.h>

// -------- LED --------
#define PIN1 14
#define PIN2 12
#define NUM 60

Adafruit_NeoPixel strip1(NUM, PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(NUM, PIN2, NEO_GRB + NEO_KHZ800);

// -------- BUTTONS --------
int buttons[10] = {1,2,42,41,40,39,38,37,36,35};
bool lastState[10];

// -------- COLORS --------
String names[10] = {
  "RED","GREEN","BLUE","YELLOW",
  "CYAN","MAGENTA","ORANGE",
  "PURPLE","PINK","WHITE"
};

int targetColor;
int correctRing;
int ringColors[10];

// -------- SETUP --------
void setup() {
  Serial.begin(115200);

  strip1.begin();
  strip2.begin();

  strip1.setBrightness(255);
  strip2.setBrightness(255);

  clearAll();

  for (int i=0;i<10;i++){
    pinMode(buttons[i], INPUT_PULLUP);
    lastState[i]=HIGH;
  }

  randomSeed(analogRead(0));
}

// -------- COLOR --------
uint32_t getColor(int i){
  switch(i){
    case 0: return strip1.Color(255,0,0);
    case 1: return strip1.Color(0,255,0);
    case 2: return strip1.Color(0,0,255);
    case 3: return strip1.Color(255,255,0);
    case 4: return strip1.Color(0,255,255);
    case 5: return strip1.Color(255,0,255);
    case 6: return strip1.Color(255,100,0);
    case 7: return strip1.Color(150,0,255);
    case 8: return strip1.Color(255,20,147);
    case 9: return strip1.Color(255,255,255);
  }
  return 0;
}

// -------- LOOP --------
void loop(){

  targetColor = random(0,10);
  correctRing = random(0,10);

  Serial.print("Find: ");
  Serial.println(names[targetColor]);

  // assign colors (exclude target color)
  for(int i=0;i<10;i++){
    do{
      ringColors[i] = random(0,10);
    }while(ringColors[i] == targetColor);
  }

  // set correct ring
  ringColors[correctRing] = targetColor;

  // display all rings
  for(int i=0;i<10;i++){
    setRing(i, getColor(ringColors[i]));
  }

  bool answered=false;
  unsigned long start=millis();

  while(millis()-start < 5000){

    for(int i=0;i<10;i++){

      bool current = digitalRead(buttons[i]);

      if(lastState[i]==HIGH && current==LOW){

        if(i==correctRing){
          Serial.println("✅ CORRECT");
        }else{
          Serial.println("❌ WRONG");

          // 🔥 blink all rings target color
          for(int j=0;j<6;j++){
            setAll(getColor(targetColor));
            delay(300);
            clearAll();
            delay(300);
          }
        }

        answered=true;
        delay(800);
      }

      lastState[i]=current;
    }
  }

  if(!answered){
    Serial.println("⚠ MISS");
  }

  clearAll();
  delay(800);
}

// -------- FUNCTIONS --------
void setRing(int ringNum, uint32_t color){

  if(ringNum < 5){
    int start = ringNum * 12;
    for(int i=start;i<start+12;i++){
      strip1.setPixelColor(i,color);
    }
    strip1.show();

  }else{
    int r = ringNum - 5;
    int start = r * 12;
    for(int i=start;i<start+12;i++){
      strip2.setPixelColor(i,color);
    }
    strip2.show();
  }
}

void setAll(uint32_t color){
  for(int i=0;i<NUM;i++){
    strip1.setPixelColor(i,color);
    strip2.setPixelColor(i,color);
  }
  strip1.show();
  strip2.show();
}

void clearAll(){
  strip1.clear();
  strip2.clear();
  strip1.show();
  strip2.show();
}