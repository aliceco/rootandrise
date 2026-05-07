#include <FastLED.h>

// Display 
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

//LED Strips
#define DATA_PIN 11
#define CLOCK_PIN 13

// Display pins
Adafruit_PCD8544 display = Adafruit_PCD8544(6, 5, 4, 3, 2);

// Item names (index matches button value 1–6)
String itemNames[7] = {
  "",
  "Food", "Money",
  "Shelter", "Water",
  "Family", "Community"
};
//LED Strip
#define NUM_LEDS 10
CRGB leds[NUM_LEDS];

int fadeAmount = 5;
int brightness = 0;

//Display
#define NUM_TILES 16
char tiles[NUM_TILES];
int trapPositions[4];
int trapIndex;

int cols = 4;
int rows = 4;

// -------- GAME STATE --------
enum GameState {
  START,
  PLAYING,
  GAME_OVER
};

enum PressResult {
  TRAP = 1,
  HIT  = 2,
  MISS1 = 3
};

GameState state = START;

// level: 0 = level 1 unlocked, 1 = level 2, 2 = level 3
// correctCount: how many items found in the CURRENT level (0 or 1)
int correctCount = 0;
int level = 0;

// correctArray[level][0..1] = the two button IDs that are correct for that level
int correctArray[3][2] = {
  {1, 4},   // Level 1: Food, Water
  {3, 8},   // Level 2: Shelter, Money
  {10, 6},   // Level 3: Relationships, Community
};

int trapArray[4] = {2, 5, 7, 9};
bool used[11];

// --- Pins ---
const int buttonPin  = A0;

// Level indicator LEDs
const int redLamp    = A1;    // Level 1 complete
const int yellowLamp = 8;    // Level 2 complete
const int greenLamp  = 7;   // Level 3 complete / win

// RGB LED (main feedback)
const int redPin   = 11;
const int greenPin = 12;
const int bluePin  = 13;

// Levers
const int BUTTON9  = 9;
const int BUTTON10 = 10;

// --- Button IDs ---
const int BUTTON1 = 1;
const int BUTTON2 = 2;
const int BUTTON3 = 3;
const int BUTTON4 = 4;
const int BUTTON5 = 5;
const int BUTTON6 = 6;
const int BUTTON7 = 7;
const int BUTTON8 = 8;

// Analog ranges
const int BUTTON1LOW = 980; const int BUTTON1HIGH = 1024;
const int BUTTON2LOW = 950;  const int BUTTON2HIGH = 979;
const int BUTTON3LOW = 800;  const int BUTTON3HIGH = 949;
const int BUTTON4LOW = 720;  const int BUTTON4HIGH = 799;
const int BUTTON5LOW = 650;  const int BUTTON5HIGH = 719;
const int BUTTON6LOW = 600;  const int BUTTON6HIGH = 649;
const int BUTTON7LOW = 550;  const int BUTTON7HIGH = 599;
const int BUTTON8LOW = 100;    const int BUTTON8HIGH = 549;

// Debounce / state
int  buttonState       = LOW;
int  lastButtonState   = LOW;
int  button10State     = LOW;
int  lastLeverState9   = HIGH;   // INPUT_PULLUP: unpressed = HIGH
long lastDebounceTime  = 0;
long debounceDelay     = 50;


// ================================================================
void setup() {
  Serial.begin(9600);

  pinMode(buttonPin,  INPUT);
  pinMode(BUTTON9,    INPUT_PULLUP);
  pinMode(BUTTON10,   INPUT);

  pinMode(redPin,    OUTPUT);
  pinMode(greenPin,  OUTPUT);
  pinMode(bluePin,   OUTPUT);

  pinMode(redLamp,    OUTPUT);
  digitalWrite(redLamp,   LOW);
  pinMode(yellowLamp, OUTPUT);
  digitalWrite(yellowLamp,  LOW);
  pinMode(greenLamp,  OUTPUT);
  digitalWrite(greenLamp,   LOW);


  delay(100); // let pins settle after pinMode
  lastLeverState9  = digitalRead(BUTTON9);
  
  Serial.print("Lever 9 init: ");  Serial.println(lastLeverState9);
  

  display.begin();
  display.setContrast(57);
  display.clearDisplay();
  display.display();

  // Serial.println("resetting leds");
  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
  

  FastLED.setBrightness(20);
}

//LED
void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

// ================================================================
//  DISPLAY 
// ================================================================
void showMessage(String line1, String line2 = "") {
  display.clearDisplay();
  display.setTextColor(BLACK);

  if (line2 == "") {
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.println(line1);
  } else {
    display.setTextSize(2);
    display.setCursor(0, 8);
    display.println(line1);
    display.setCursor(0, 22);
    display.println(line2);
  }
  display.display();
}


// Maps button ID (1-10) to a tile index on the 4x4 grid
int buttonToTile(int buttonId) {
  //          unused  B1   B2   B3  B4  B5   B6  B7  B8  B9  B10
  int map[] = { -1,   14,  13,   8,  9,  11,   4,  1,  2,   5,   7 };
  if (buttonId < 1 || buttonId > 10) return -1;
  return map[buttonId];
}

void generateBoard() {
  for (int i = 0; i < NUM_TILES; i++) tiles[i] = ' ';

  for (int i = 0; i < 4; i++) {
    int tileIdx = buttonToTile(trapArray[i]);
    if (tileIdx >= 0 && tileIdx < NUM_TILES) {
      tiles[tileIdx] = '*';
      trapPositions[i] = tileIdx;
    }
  }
}

void showMap(int playerIndex) {
  display.clearDisplay();
  display.drawRect(0, 0, 84, 48, BLACK);

  int cellW = 84 / cols;  // = 21
  int cellH = 48 / rows;  // = 12

  int idx = trapPositions[playerIndex];
  int c = idx % cols;
  int r = idx / cols;
  int x = c * cellW + cellW / 2 - 3;
  int y = r * cellH + cellH / 2 - 4;

  display.setCursor(x, y);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.print("*");

  display.display();
}


void crankingPower(){
  showMessage("Start Cranking!");
  delay(5000);
  for (int i = 0; i <= NUM_LEDS; i++) {
       leds[i] = CRGB(255, 80, 0);
       FastLED.show();
       fadeall();
       delay(200);
   }
}

void startMap() {
  showMessage("Start  Game!");
  delay(2000);

  generateBoard(); // only once!

  for (int i = 0; i < 4; i++) {
    showMessage("Player " + String(i + 1));
    delay(1000);
    showMessage("YOU CAN LOOK");
    delay(2000);

    showMap(i); // player 0 → trap 2, player 1 → trap 5, etc.
    delay(2500);

    display.clearDisplay();
    display.display();
    delay(500);
  }

  showMessage("Find Items!");
  delay(1500);
  showMessage("GO!");
  delay(1000);
  display.clearDisplay();
  display.display();
}


// ================================================================
//  RGB HELPER
// ================================================================
void setColor(int r, int g, int b) {
  analogWrite(redPin,   r);
  analogWrite(greenPin, g);
  analogWrite(bluePin,  b);
}

// Illuminate level LEDs up to (but NOT including) the current level.
// Call after level changes so LEDs show completed levels.
void updateLevelLamps(int lvl) {
  digitalWrite(redLamp,    lvl >= 1 ? HIGH : LOW);
  digitalWrite(yellowLamp, lvl >= 2 ? HIGH : LOW);
  digitalWrite(greenLamp,  lvl >= 3 ? HIGH : LOW);
}


// ================================================================
//  BUTTON / LEVER READING
// ================================================================
int getButtonValue() {
  int currentLeverState9  = digitalRead(BUTTON9);
  int button10State       = digitalRead(BUTTON10);
  // Serial.println(currentLeverState10);

  if (currentLeverState9 != lastLeverState9) {
    lastLeverState9 = currentLeverState9;
    Serial.print("lever9");

    return 9;
  }
  if (button10State == HIGH) {
    
    Serial.print("BUTTOn 10");
    return 10;
  }

  int reading = analogRead(buttonPin);
  Serial.println(reading);
  int tmpButtonState = LOW;

  if      (reading >= BUTTON1LOW && reading <= BUTTON1HIGH) tmpButtonState = BUTTON1;
  else if (reading >= BUTTON2LOW && reading <= BUTTON2HIGH) tmpButtonState = BUTTON2;
  else if (reading >= BUTTON3LOW && reading <= BUTTON3HIGH) tmpButtonState = BUTTON3;
  else if (reading >= BUTTON4LOW && reading <= BUTTON4HIGH) tmpButtonState = BUTTON4;
  else if (reading >= BUTTON5LOW && reading <= BUTTON5HIGH) tmpButtonState = BUTTON5;
  else if (reading >= BUTTON6LOW && reading <= BUTTON6HIGH) tmpButtonState = BUTTON6;
  else if (reading >= BUTTON7LOW && reading <= BUTTON7HIGH) tmpButtonState = BUTTON7;
  else if (reading >= BUTTON8LOW && reading <= BUTTON8HIGH) tmpButtonState = BUTTON8;
  else tmpButtonState = LOW;

  if (tmpButtonState != lastButtonState) lastDebounceTime = millis();

  int result = 0;
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (tmpButtonState != buttonState) {
      buttonState = tmpButtonState;
      if (buttonState != LOW) {
        result = buttonState;
        Serial.print("Button: "); Serial.println(result);
      }
    }
  }
  lastButtonState = tmpButtonState;
  return result;
}


// ================================================================
//  PRESS CLASSIFICATION
// ================================================================
PressResult checkButtonPress(int buttonValue) {
  // Is it a correct item for the CURRENT level?
  if (correctArray[level][0] == buttonValue ||
      correctArray[level][1] == buttonValue) {
    return HIT;
  }

  // Is it a trap button?
  for (int i = 0; i < 4; i++) {
    if (trapArray[i] == buttonValue) return TRAP;
  }

  // Otherwise it's a valid item button but for the wrong level
  return MISS1;
}


// ================================================================
//  TRAP PENALTY  — lose the last obtained item
//
//  Progress is tracked as: level (0-2) + correctCount (0 or 1)
//  "Last obtained" item is the most recently collected item.
//
//  Cases:
//    correctCount == 1  → lose that item → correctCount = 0
//                         (level LED stays ON for completed levels below)
//    correctCount == 0  → lose the 2nd item of the previous level
//                         → drop to level-1, correctCount = 1
//                         → turn off that level's LED
//    If level==0 && correctCount==0 → nothing to lose → GAME OVER
// ================================================================
void applyTrapPenalty() {
  if (level == 0 && correctCount == 0) {
    // No items at all — game over
    showMessage("GAME OVER!");
    delay(2000);
    state = GAME_OVER;
    return;
  }

  if (correctCount == 1) {
    // Lose the first item found this level
    correctCount = 0;
    
    showMessage("Lost:", itemNames[correctArray[level][0]]);
  } else {
    // correctCount == 0, drop back one level, lose its 2nd item
    level--;
    correctCount = 1;
    updateLevelLamps(level);   // turn off the LED for the level we just dropped from
    showMessage("Lost:", itemNames[correctArray[level][1]]);
  }
}


// ================================================================
//  MAIN LOOP
// ================================================================
void loop() {
  // setColor(0, 0, 0);

  // ---------- START ----------
  if (state == START) {
    fill_solid(leds, 10, CRGB::Black); // turn them off
    FastLED.show();
    crankingPower();
    level        = 0;
    correctCount = 0;
    updateLevelLamps(0);
    // startMap();
    state = PLAYING;
    // return;
  }

  // ---------- PLAYING ----------
  if (state == PLAYING) {
    int buttonValue = getButtonValue();
    if (buttonValue == 0) return;


    PressResult result = checkButtonPress(buttonValue);

    // ---- TRAP ----
    if (result == TRAP) {
      Serial.println("TRAP");
      setColor(255, 0, 0);           // RGB red
      showMessage("System Barrier!");
      delay(1500);
      applyTrapPenalty();
      delay(1000);
      setColor(0, 0, 0);
    }

    // ---- HIT (correct item for current level) ----
    else if (result == HIT) {
      Serial.println("HIT: " + itemNames[buttonValue]);
      setColor(255, 255, 255);       // RGB white
      showMessage(itemNames[buttonValue], "Item!");
      delay(1500);
      showMessage("Continue");
      delay(500);

      correctCount++;

      if (correctCount == 2) {
        // Both items of this level collected → advance
        level++;
        correctCount = 0;
        updateLevelLamps(level);

        if (level == 3) {
          // WIN
          setColor(0, 255, 0);
          showMessage("YOU WIN!");
          delay(3000);
          state = GAME_OVER;
          return;
        }

        showMessage("Level " + String(level + 1) + "Unlock!");
        delay(500);
      }
      setColor(0, 0, 0);
    }

    // ---- MISS (valid item but wrong level) ----
    else if (result == MISS1) {
      Serial.println("miss");
      setColor(255, 255, 0);         // RGB yellow
      showMessage("Wrong Level!");
      delay(500);
      setColor(0, 0, 0);
      showMessage("Continue");
      delay(200);
    }

    return;
  }

  // ---------- GAME OVER ----------
  if (state == GAME_OVER) {
    level        = 0;
    correctCount = 0;
    updateLevelLamps(0);
    setColor(0, 0, 0);
    delay(2000);
    state = START;
  }
}
