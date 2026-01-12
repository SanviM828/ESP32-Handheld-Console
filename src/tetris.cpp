#include <FastLED.h>


// --- HARDWARE CONFIGURATION ---
// Power: Toggle Switch (Hardware)
// Controls:
#define LEFT_BUTTON_PIN   2
#define RIGHT_BUTTON_PIN  3
#define ROTATE_BUTTON_PIN 4
#define HARD_DROP_PIN     14  // The "Action" button
#define PAUSE_BUTTON_PIN  18  // The "Menu" button


// --- LED MATRIX SETUP ---
#define LED_PIN      25
#define NUM_LEDS     64  
#define BRIGHTNESS   20
#define LED_TYPE     WS2812B
#define COLOR_ORDER  GRB
#define MATRIX_WIDTH  8
#define MATRIX_HEIGHT 8
#define BUZZER_PIN    23


// --- GAME SETTINGS ---
#define INITIAL_GAME_SPEED 600
#define SPEED_INCREASE 0      
#define MIN_GAME_SPEED 2
#define DEBOUNCE_TIME 150


// --- GLOBALS ---
CRGB leds[NUM_LEDS];
#define BLACK     CRGB(0, 0, 0)
#define RED       CRGB(255, 0, 0)
#define GREEN     CRGB(0, 255, 0)
#define BLUE      CRGB(0, 0, 255)
#define YELLOW    CRGB(255, 255, 0)
#define CYAN      CRGB(0, 255, 255)
#define MAGENTA   CRGB(255, 0, 255)
#define ORANGE    CRGB(255, 165, 0)


// Tetromino Definitions
typedef struct {
  byte shapes[4][4][2];
  byte numCells;
  CRGB color;
} Tetromino;


Tetromino tetrominos[7] = {
  { {{{0,0}, {1,0}, {2,0}, {3,0}}, {{0,0}, {0,1}, {0,2}, {0,3}}, {{0,0}, {1,0}, {2,0}, {3,0}}, {{0,0}, {0,1}, {0,2}, {0,3}}}, 4, CYAN }, // I
  { {{{0,0}, {1,0}, {0,1}, {1,1}}, {{0,0}, {1,0}, {0,1}, {1,1}}, {{0,0}, {1,0}, {0,1}, {1,1}}, {{0,0}, {1,0}, {0,1}, {1,1}}}, 4, YELLOW }, // O
  { {{{0,0}, {1,0}, {2,0}, {1,1}}, {{1,0}, {0,1}, {1,1}, {1,2}}, {{1,0}, {0,1}, {1,1}, {2,1}}, {{0,0}, {0,1}, {0,2}, {1,1}}}, 4, MAGENTA }, // T
  { {{{1,0}, {2,0}, {0,1}, {1,1}}, {{0,0}, {0,1}, {1,1}, {1,2}}, {{1,0}, {2,0}, {0,1}, {1,1}}, {{0,0}, {0,1}, {1,1}, {1,2}}}, 4, GREEN },   // S
  { {{{0,0}, {1,0}, {1,1}, {2,1}}, {{1,0}, {0,1}, {1,1}, {0,2}}, {{0,0}, {1,0}, {1,1}, {2,1}}, {{1,0}, {0,1}, {1,1}, {0,2}}}, 4, RED },     // Z
  { {{{0,0}, {0,1}, {1,1}, {2,1}}, {{1,0}, {2,0}, {1,1}, {1,2}}, {{0,0}, {1,0}, {2,0}, {2,1}}, {{0,0}, {0,1}, {0,2}, {1,0}}}, 4, BLUE },    // J
  { {{{2,0}, {0,1}, {1,1}, {2,1}}, {{0,0}, {1,0}, {1,1}, {1,2}}, {{0,0}, {1,0}, {2,0}, {0,1}}, {{0,0}, {0,1}, {0,2}, {1,2}}}, 4, ORANGE }   // L
};


bool gameBoard[MATRIX_WIDTH][MATRIX_HEIGHT] = {0};
CRGB boardColors[MATRIX_WIDTH][MATRIX_HEIGHT];


byte currentPiece = 0;
byte currentRotation = 0;
int currentX = 3;
int currentY = 0;
unsigned long lastFallTime = 0;
unsigned long gameSpeed = INITIAL_GAME_SPEED;
boolean gameOver = false;
boolean isPaused = false;
unsigned int score = 0;
unsigned int level = 1;


// Button State Tracking
bool leftPressed = false;
bool rightPressed = false;
bool rotatePressed = false;
bool dropPressed = false;
bool pausePressed = false;


unsigned long lastButtonCheckTime = 0;


void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());
 
  pinMode(BUZZER_PIN, OUTPUT);
 
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  clearDisplay();
 
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ROTATE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(HARD_DROP_PIN, INPUT_PULLUP);
  pinMode(PAUSE_BUTTON_PIN, INPUT_PULLUP);
 
  Serial.println("ESP32 Mini Tetris - 5 Button Mode (With Pause)");
  displayStartAnimation();
  spawnNewPiece();
}


void loop() {
  // Always check for Pause button, even if paused
  checkPauseButton();


  if (isPaused) {
    // If paused, do nothing but wait (and maybe blink a light if you wanted)
    return;
  }


  if (gameOver) {
    displayGameOver();
    if (checkAnyButtonPressed()) {
      delay(500);
      resetGame();
    }
    return;
  }
 
  checkGameButtons();
 
  // Gravity Logic
  if (millis() - lastFallTime > gameSpeed) {
    if (!movePieceDown()) {
      placePiece();
      clearLines();
      if (!spawnNewPiece()) {
        gameOver = true;
      }
     
      if (gameSpeed > MIN_GAME_SPEED) gameSpeed -= SPEED_INCREASE;
      if (score > 0 && score % 500 == 0) {
        level++;
        playLevelUpSound();
      }
    }
    lastFallTime = millis();
  }
 
  updateDisplay();
}


void checkPauseButton() {
  if (millis() - lastButtonCheckTime > DEBOUNCE_TIME) {
    bool pauseState = (digitalRead(PAUSE_BUTTON_PIN) == LOW);
    if (pauseState && !pausePressed) {
      pausePressed = true;
      isPaused = !isPaused; // Toggle Pause
      if(isPaused) {
        tone(BUZZER_PIN, 1000, 100); // Pause Sound
      } else {
        tone(BUZZER_PIN, 2000, 100); // Resume Sound
      }
      lastButtonCheckTime = millis();
    } else if (!pauseState) {
      pausePressed = false;
    }
  }
}


void checkGameButtons() {
  if (millis() - lastButtonCheckTime > DEBOUNCE_TIME) {
   
    // LEFT
    bool leftState = (digitalRead(LEFT_BUTTON_PIN) == LOW);
    if (leftState && !leftPressed) {
      leftPressed = true;
      movePieceLeft();
      lastButtonCheckTime = millis();
    } else if (!leftState) leftPressed = false;
   
    // RIGHT
    bool rightState = (digitalRead(RIGHT_BUTTON_PIN) == LOW);
    if (rightState && !rightPressed) {
      rightPressed = true;
      movePieceRight();
      lastButtonCheckTime = millis();
    } else if (!rightState) rightPressed = false;
   
    // ROTATE
    bool rotateState = (digitalRead(ROTATE_BUTTON_PIN) == LOW);
    if (rotateState && !rotatePressed) {
      rotatePressed = true;
      rotatePiece();
      lastButtonCheckTime = millis();
    } else if (!rotateState) rotatePressed = false;
   
    // HARD DROP (Pin 14)
    bool dropState = (digitalRead(HARD_DROP_PIN) == LOW);
    if (dropState && !dropPressed) {
      dropPressed = true;
      int dropDistance = 0;
      while (movePieceDown()) {
        dropDistance++;
      }
      score += dropDistance * 2;
      placePiece();
      clearLines();
      if (!spawnNewPiece()) gameOver = true;
      lastButtonCheckTime = millis();
    } else if (!dropState) dropPressed = false;
  }
}


bool checkAnyButtonPressed() {
  return (digitalRead(LEFT_BUTTON_PIN) == LOW ||
          digitalRead(RIGHT_BUTTON_PIN) == LOW ||
          digitalRead(ROTATE_BUTTON_PIN) == LOW ||
          digitalRead(HARD_DROP_PIN) == LOW ||
          digitalRead(PAUSE_BUTTON_PIN) == LOW);
}


// --- DISPLAY FUNCTIONS ---


int getPixelIndex(int x, int y) {
  return y * MATRIX_WIDTH + x;
}


void clearDisplay() {
  fill_solid(leds, NUM_LEDS, BLACK);
  FastLED.show();
}


void updateDisplay() {
  fill_solid(leds, NUM_LEDS, BLACK);
 
  // Draw Board
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      if (gameBoard[x][y]) leds[getPixelIndex(x, y)] = boardColors[x][y];
    }
  }
 
  // Draw Current Piece
  for (int i = 0; i < tetrominos[currentPiece].numCells; i++) {
    int x = currentX + tetrominos[currentPiece].shapes[currentRotation][i][0];
    int y = currentY + tetrominos[currentPiece].shapes[currentRotation][i][1];
    if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
      leds[getPixelIndex(x, y)] = tetrominos[currentPiece].color;
    }
  }
  FastLED.show();
}


// --- GAME LOGIC ---


bool isValidPosition(int pieceIndex, int rotation, int posX, int posY) {
  for (int i = 0; i < tetrominos[pieceIndex].numCells; i++) {
    int x = posX + tetrominos[pieceIndex].shapes[rotation][i][0];
    int y = posY + tetrominos[pieceIndex].shapes[rotation][i][1];


    if (x < 0 || x >= MATRIX_WIDTH) return false;
    if (y >= MATRIX_HEIGHT) return false;
    if (y >= 0 && gameBoard[x][y]) return false;
  }
  return true;
}


bool movePieceLeft() {
  if (isValidPosition(currentPiece, currentRotation, currentX - 1, currentY)) {
    currentX--; return true;
  }
  return false;
}


bool movePieceRight() {
  if (isValidPosition(currentPiece, currentRotation, currentX + 1, currentY)) {
    currentX++; playMoveSound(); return true;
  }
  return false;
}


bool movePieceDown() {
  if (isValidPosition(currentPiece, currentRotation, currentX, currentY + 1)) {
    currentY++; return true;
  }
  return false;
}


bool rotatePiece() {
  byte nextRotation = (currentRotation + 1) % 4;
  if (isValidPosition(currentPiece, nextRotation, currentX, currentY)) {
    currentRotation = nextRotation; playRotateSound(); return true;
  }
  // Basic Wall Kicks
  if (isValidPosition(currentPiece, nextRotation, currentX - 1, currentY)) {
    currentX--; currentRotation = nextRotation; playRotateSound(); return true;
  }
  if (isValidPosition(currentPiece, nextRotation, currentX + 1, currentY)) {
    currentX++; currentRotation = nextRotation; playRotateSound(); return true;
  }
  return false;
}


void placePiece() {
  for (int i = 0; i < tetrominos[currentPiece].numCells; i++) {
    int x = currentX + tetrominos[currentPiece].shapes[currentRotation][i][0];
    int y = currentY + tetrominos[currentPiece].shapes[currentRotation][i][1];
    if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
      gameBoard[x][y] = true;
      boardColors[x][y] = tetrominos[currentPiece].color;
    }
  }
  playLandSound();
}


int getPieceWidth(byte pieceIndex, byte rotation) {
  int minX = 10, maxX = -10;
  for (int i = 0; i < tetrominos[pieceIndex].numCells; i++) {
    int x = tetrominos[pieceIndex].shapes[rotation][i][0];
    if (x < minX) minX = x;
    if (x > maxX) maxX = x;
  }
  return (maxX - minX + 1);
}


bool spawnNewPiece() {
  static byte lastPiece = random(0, 7);
  byte newPiece;
  do { newPiece = random(0, 7); } while (newPiece == lastPiece && random(0, 100) < 70);
  lastPiece = newPiece;
  currentPiece = newPiece;
  currentRotation = 0;
  int pieceWidth = getPieceWidth(currentPiece, currentRotation);
  currentX = (MATRIX_WIDTH - pieceWidth) / 2;
  currentY = 0;
  return isValidPosition(currentPiece, currentRotation, currentX, currentY);
}


void clearLines() {
  int linesCleared = 0;
  for (int y = MATRIX_HEIGHT - 1; y >= 0; y--) {
    bool fullLine = true;
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      if (!gameBoard[x][y]) { fullLine = false; break; }
    }
    if (fullLine) {
      linesCleared++;
      for (int row = y; row > 0; row--) {
        for (int col = 0; col < MATRIX_WIDTH; col++) {
          gameBoard[col][row] = gameBoard[col][row - 1];
          boardColors[col][row] = boardColors[col][row - 1];
        }
      }
      for (int col = 0; col < MATRIX_WIDTH; col++) {
        gameBoard[col][0] = false;
        boardColors[col][0] = BLACK;
      }
      y++;
    }
  }
  if (linesCleared > 0) {
    playClearLineSound(linesCleared);
    int points = 0;
    switch (linesCleared) {
      case 1: points = 20 * level; break;
      case 2: points = 50 * level; break;
      case 3: points = 150 * level; break;
      case 4: points = 600 * level; break;
    }
    score += points;
    Serial.println("Lines: " + String(linesCleared) + " Score: " + String(score));
  }
}


void resetGame() {
  playStartSound();
  clearDisplay();
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      gameBoard[x][y] = false;
    }
  }
  gameSpeed = INITIAL_GAME_SPEED;
  gameOver = false;
  score = 0;
  level = 1;
  displayStartAnimation();
  spawnNewPiece();
}


void displayStartAnimation() {
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  delay(200);
  clearDisplay();
  delay(200);
}


void displayGameOver() {
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(500);
  clearDisplay();
  delay(500);
  Serial.println("GAME OVER! Score: " + String(score));
}


// --- SOUND EFFECTS ---
void playMoveSound() { tone(BUZZER_PIN, 1200, 30); }
void playRotateSound() { tone(BUZZER_PIN, 1000, 25); delay(25); tone(BUZZER_PIN, 1500, 25); }
void playLandSound() { tone(BUZZER_PIN, 800, 100); }
void playLevelUpSound() { tone(BUZZER_PIN, 2000, 100); delay(100); tone(BUZZER_PIN, 2500, 200); }
void playClearLineSound(int lines) {
  if(lines==4) { tone(BUZZER_PIN, 1500, 80); delay(80); tone(BUZZER_PIN, 2500, 300); }
  else { tone(BUZZER_PIN, 1000 + (lines*200), 100); }
}
void playStartSound() { tone(BUZZER_PIN, 1000, 80); delay(80); tone(BUZZER_PIN, 2000, 200); }

