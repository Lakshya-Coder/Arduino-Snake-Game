#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include "bitmaps.h"
#include "pitches.h"
#include "constant.h"

Adafruit_SSD1306 lcd(screenWidth, screenHeight, &Wire, -1);

bool isWantCollectFruitSound = false;
bool isWantButtonPressedSound = false;
unsigned long lastBuzzerPlayed = 0;

struct Position {
  int x, y;  

  bool operator==(const Position& other) const {
    return x == other.x && y == other.y;
  }

  Position& operator+=(const Position& other) {
    x += other.x;
    y += other.y;
    return *this;
  }
};

void draw_square(Position pos, int color = WHITE) {
  lcd.fillRect(pos.x * 2, pos.y * 2, 2, 2, color);
}

bool test_position(Position pos) {
  return lcd.getPixel(pos.x * 2, pos.y * 2);
}

const Position kDirPos[4] = {
  {0,-1}, // Up
  {1, 0}, // Right
  {0, 1},  // Down
  {-1, 0} // Left
};

struct Player {
  Player() { 
    reset(); 
  }

  Position pos;
  unsigned char tail[maxLength];
  unsigned char direction;
  int size, moved;
  
  void reset() {
    pos = {32, 16};
    direction = 1;
    size = startLength;
    memset(tail, 0, sizeof(tail));
    moved = 0;
  }

  bool isTurningDirIsVaild(int dir) { 
    if (dir == TURN_LEFT && direction == TURN_RIGHT) {
      return false;
    } else if (dir == TURN_RIGHT && direction == TURN_LEFT) {
      return false;
    } else if (dir == TURN_DOWN && direction == TURN_UP) {
      return false;
    } else if (dir == TURN_UP && direction == TURN_DOWN) {
      return false;
    }

    return true; 
  }

  void turn(int dir) {
    if (!isTurningDirIsVaild(dir)) return;

    direction = dir;
  }

  void update() {
    for(int i = maxLength - 1; i > 0; --i) {
      tail[i] = tail[i] << 2 | ((tail[i - 1] >> 6) & 3);
    }

    tail[0] = tail[0] << 2 | ((direction + 2) % 4);
    pos += kDirPos[direction];
    
    if(moved < size) {
      moved++;
    }
  }
  
  void render() const {
    draw_square(pos);

    if(moved < size) {
      return;
    }

    Position tailpos = pos;

    for(int i = 0; i < size; ++i) {
      tailpos += kDirPos[(tail[(i >> 2)] >> ((i & 3) * 2)) & 3];
    }

    draw_square(tailpos, BLACK);
  }
} player;

struct Item {
  Position pos;
  void spawn() {
    pos.x = random(1, 63);
    pos.y = random(1, 31);
  }

  void render() const {
    draw_square(pos);
  }
} item;

void waitForInput() {
  do {
    handelSound();
  } while(digitalRead(SW_PIN) == HIGH);

  isWantButtonPressedSound = true;
  lastBuzzerPlayed = millis();
  handelSound();
}

void pushToStart() {
  lcd.setCursor(26,57);
  lcd.print(F("Push to start"));
}

void flashScreen() {
  lcd.invertDisplay(true);
  handelSound();
  delay(100);
  handelSound();
  lcd.invertDisplay(false);
  delay(200);
  handelSound();
}

void playIntro() {
  lcd.clearDisplay();
  lcd.drawBitmap(18, 0, kSplashScreen, 92, 56, WHITE);
  pushToStart();
  lcd.display();
  waitForInput();
  flashScreen();
}

void playGameover() {
  flashScreen();
  lcd.clearDisplay();
  lcd.drawBitmap(4, 0, kGameOver, 124, 38, WHITE);
  int score = player.size - startLength;
  lcd.setCursor(26, 34);
  lcd.print(F("Score: "));
  lcd.print(score);
  int hiscore;
  EEPROM.get(0, hiscore);
  if(score > hiscore) {
    EEPROM.put(0, score);
    hiscore = score;
    lcd.setCursor(4, 44);
    lcd.print(F("NEW"));
  }
  lcd.setCursor(26, 44);
  lcd.print(F("Hi-Score: "));
  lcd.print(hiscore);
  pushToStart();
  lcd.display();
  waitForInput();
  flashScreen();
}

void resetGame() {
  lcd.clearDisplay();

  for(int x = 0; x < gameWidth; ++x) {
    draw_square({x, 0});
    draw_square({x, 31});
  }
  for(int y = 0; y < gameHeight; ++y) {
    draw_square({0, y});
    draw_square({63, y});
  }
  player.reset();
  item.spawn();
}

void updateGame() {
  player.update();
  
  if(player.pos == item.pos) {
    isWantCollectFruitSound = true;
    lastBuzzerPlayed = millis();
    player.size++;
    item.spawn();
  } else if(test_position(player.pos)) {
    playGameover();
    resetGame();
  }
}

void input() {
  int x = analogRead(VRX);
  int y = analogRead(VRY);

  if (x <= 100 - 50) {
    player.turn(TURN_LEFT);
  } else if (x >= 900 + 100) {
    player.turn(TURN_RIGHT);
  } else if (y <= 100 - 50) {
    player.turn(TURN_UP);
  } else if (y >= 900 + 100) {
    player.turn(TURN_DOWN);
  }
}

void render() {
  player.render();
  item.render();
  lcd.display();
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VRX, INPUT);
  pinMode(VRY, INPUT);
  pinMode(SW_PIN, INPUT);
  analogWrite(BUZZER_PIN, 1);
  
  digitalWrite(SW_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  noTone(BUZZER_PIN);

  lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  lcd.setTextColor(WHITE);
  playIntro();
  resetGame();
}

void loop() {
  input();
  updateGame();
  handelSound();
  render();
}

void handelSound() {
  if (isWantCollectFruitSound) {
    if (millis() <= lastBuzzerPlayed + 100) {
      tone(BUZZER_PIN,NOTE_B5, 100);
    } else if (millis() <= lastBuzzerPlayed + 455) {
      tone(BUZZER_PIN,NOTE_E6, 850);
    } else {
      noTone(BUZZER_PIN);
      isWantCollectFruitSound = false;
    }
  } else if (isWantButtonPressedSound) {
    if (millis() <= lastBuzzerPlayed + 120) {
      tone(BUZZER_PIN, NOTE_AS2, 120);
    } else {
      noTone(BUZZER_PIN);
      isWantButtonPressedSound = false;
    }
  } else {
    noTone(BUZZER_PIN);
  }
}