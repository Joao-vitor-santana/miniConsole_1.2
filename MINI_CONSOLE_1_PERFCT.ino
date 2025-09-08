#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

bool spaceGameOver = false;

bool breakoutGameOver = false;
bool froggerGameOver = false;

// Estados de Game Over para todos os jogos
bool snakeGameOver = false;
bool pongGameOver = false;
bool geniusGameOver = false;
bool tttGameOver = false;
bool asteroidsGameOver = false;

// ==================== CENTIPEDE ====================
struct CentipedeSegment {
  int x, y;
  bool active;
  int direction;
  bool isHead;
};

struct CentipedeMushroom {
  int x, y;
  int hits;
  bool active;
};

struct CentipedeSpider {
  int x, y;
  int vx, vy;
  bool active;
  unsigned long lastMove;
};

struct CentipedeShooter {
  int x, y;
};

// ==================== SPACE INVADERS - REALISTA ====================
struct Invader {
  int x, y;
  int type;
  bool active;
  int animFrame;
};

struct Bullet {
  float x, y;
  float vy;
  bool active;
  bool isPlayer;
};

struct Player {
  int x, y;
  bool active;
  int lives;
};

struct Barrier {
  int x, y;
  bool pixels[16][8];  // 16x8 barrier
};

CentipedeSegment segments[30];
CentipedeMushroom mushrooms[40];
CentipedeSpider spider;
CentipedeShooter centShooter;
Bullet centBullets[8];
int centipedeScore = 0;
int centipedeLives = 3;
bool centipedeGameOver = false;
int centipedeWave = 1;
unsigned long centipedeLastMove = 0;
int centipedeSpeed = 300;


// Configuração do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Definição dos botões
const int BTN_UP_VAL = 32;
const int BTN_DOWN_VAL = 33;
const int BTN_LEFT_VAL = 14;
const int BTN_RIGHT_VAL = 12;
const int BTN_ENTER_VAL = 25;
const int BTN_BACK_VAL = 26;
const int BTN_A = 27;

// Estados do sistema
enum GameState {
  BOOT_ANIMATION,
  MENU,
  TETRIS,
  SNAKE,
  GENIUS,
  TICTACTOE,
  DINO,
  PONG,
  SPACE_INVADERS,
  BREAKOUT,
  FROGGER,
  ASTEROIDS,
  CENTIPEDE,
  OTA
};

GameState currentGame = BOOT_ANIMATION;
int selectedGame = 0;
const int NUM_GAMES = 12;

// ==================== ULTRA ANIMAÇÃO DE BOOT ====================
unsigned long bootStartTime = 0;
int bootStep = 0;
bool bootComplete = false;

// Matrix Rain Effect - melhorado
struct MatrixColumn {
  int y;
  int speed;
  int length;
  char chars[20];
  int brightness[20];
};

MatrixColumn matrixCols[16];
bool matrixInitialized = false;

// Glitch Effect
struct GlitchLine {
  int y;
  int width;
  int offset;
  unsigned long lastUpdate;
};

GlitchLine glitchLines[8];

// Partículas para efeito cyberpunk
struct CyberParticle {
  float x, y;
  float vx, vy;
  int life;
  int maxLife;
  bool active;
};

CyberParticle particles[25];

// Terminal text simulation
String terminalLines[] = {
  "INITIALIZING QUANTUM CORE...",
  "LOADING NEURAL NETWORKS...",
  "ACTIVATING HOLO-DISPLAY...",
  "SYNCHRONIZING GAME MATRIX...",
  "ESTABLISHING DATA LINK...",
  "BOOTING ENTERTAINMENT OS...",
  "READY FOR NEURAL INTERFACE"
};

// Estrutura para controle de botões
struct ButtonState {
  bool pressed = false;
  bool justPressed = false;
  bool currentState = false;
  bool lastState = false;
  unsigned long lastDebounce = 0;
  unsigned long lastRepeat = 0;
  unsigned long pressStartTime = 0;
  const unsigned long debounceDelay = 30;
  const unsigned long repeatDelay = 150;
  const unsigned long fastRepeat = 80;
  bool isRepeating = false;
};

ButtonState buttons[7];

void initBootAnimation() {
  // Inicializar Matrix Columns
  for(int i = 0; i < 16; i++) {
    matrixCols[i].y = random(-30, 0);
    matrixCols[i].speed = random(1, 4);
    matrixCols[i].length = random(5, 15);
    
    for(int j = 0; j < 20; j++) {
      matrixCols[i].chars[j] = random(2) ? '0' : '1';
      matrixCols[i].brightness[j] = 255 - (j * 20);
    }
  }
  
  // Inicializar Glitch Lines
  for(int i = 0; i < 8; i++) {
    glitchLines[i].y = random(64);
    glitchLines[i].width = random(20, 60);
    glitchLines[i].offset = 0;
    glitchLines[i].lastUpdate = 0;
  }
  
  // Inicializar Partículas
  for(int i = 0; i < 25; i++) {
    particles[i].active = false;
  }
  
  matrixInitialized = true;
}

void spawnCyberParticle(float x, float y) {
  for(int i = 0; i < 25; i++) {
    if(!particles[i].active) {
      particles[i].x = x;
      particles[i].y = y;
      particles[i].vx = random(-20, 21) / 10.0;
      particles[i].vy = random(-15, 5) / 10.0;
      particles[i].maxLife = random(30, 80);
      particles[i].life = particles[i].maxLife;
      particles[i].active = true;
      break;
    }
  }
}

void updateCyberParticles() {
  for(int i = 0; i < 25; i++) {
    if(particles[i].active) {
      particles[i].x += particles[i].vx;
      particles[i].y += particles[i].vy;
      particles[i].life--;
      
      if(particles[i].life <= 0 || particles[i].x < 0 || 
         particles[i].x > 128 || particles[i].y < 0 || particles[i].y > 64) {
        particles[i].active = false;
      }
    }
  }
}

void drawCyberParticles() {
  for(int i = 0; i < 25; i++) {
    if(particles[i].active) {
      int alpha = (particles[i].life * 255) / particles[i].maxLife;
      if(alpha > 128) {
        display.drawPixel((int)particles[i].x, (int)particles[i].y, SSD1306_WHITE);
        
        // Trails para partículas mais brilhantes
        if(alpha > 200) {
          display.drawPixel((int)particles[i].x - 1, (int)particles[i].y, SSD1306_WHITE);
          display.drawPixel((int)particles[i].x, (int)particles[i].y - 1, SSD1306_WHITE);
        }
      }
    }
  }
}

void drawMatrixRain() {
  // Atualizar e desenhar colunas da matrix
  for(int i = 0; i < 16; i++) {
    matrixCols[i].y += matrixCols[i].speed;
    
    if(matrixCols[i].y > 80) {
      matrixCols[i].y = random(-30, -5);
      matrixCols[i].speed = random(2, 5);
      
      // Regenerar caracteres
      for(int j = 0; j < matrixCols[i].length; j++) {
        matrixCols[i].chars[j] = random(2) ? '0' : '1';
      }
    }
    
    // Desenhar coluna
    for(int j = 0; j < matrixCols[i].length; j++) {
      int y = matrixCols[i].y - (j * 6);
      if(y >= 0 && y < 64) {
        int x = i * 8;
        
        // Efeito de fade
        if(j == 0) {
          // Cabeça da coluna - mais brilhante
          display.setTextSize(1);
          display.setCursor(x, y);
          display.setTextColor(SSD1306_WHITE);
          display.print(matrixCols[i].chars[j]);
          
          // Spawn partícula ocasionalmente
          if(random(100) < 3) {
            spawnCyberParticle(x + 3, y + 4);
          }
        } else if(j < 4) {
          // Cauda - menos brilhante
          if(random(100) < 80) {
            display.setCursor(x, y);
            display.print(matrixCols[i].chars[j]);
          }
        }
      }
    }
  }
}

void drawGlitchEffect() {
  unsigned long currentTime = millis();
  
  for(int i = 0; i < 8; i++) {
    if(currentTime - glitchLines[i].lastUpdate > random(50, 200)) {
      glitchLines[i].offset = random(-5, 6);
      glitchLines[i].lastUpdate = currentTime;
      
      // Ocasionalmente mudar a linha
      if(random(100) < 20) {
        glitchLines[i].y = random(64);
        glitchLines[i].width = random(20, 80);
      }
    }
    
    // Desenhar linha com glitch
    if(random(100) < 70) {
      int startX = max(0, glitchLines[i].offset);
      int endX = min(128, glitchLines[i].width + glitchLines[i].offset);
      
      if(startX < endX) {
        display.drawLine(startX, glitchLines[i].y, endX, glitchLines[i].y, SSD1306_WHITE);
      }
    }
  }
}

void drawScanLines() {
  unsigned long currentTime = millis();
  int scanPos = (currentTime / 30) % 80;
  
  // Linha de scan principal
  if(scanPos < 64) {
    display.drawLine(0, scanPos, 128, scanPos, SSD1306_WHITE);
    display.drawLine(0, scanPos + 1, 128, scanPos + 1, SSD1306_WHITE);
  }
  
  // Linhas de scan secundárias
  for(int i = 0; i < 64; i += 4) {
    if(((i + (currentTime / 50)) % 16) < 2) {
      for(int x = 0; x < 128; x += 8) {
        display.drawPixel(x, i, SSD1306_WHITE);
      }
    }
  }
}

void drawHexGrid() {
  unsigned long currentTime = millis();
  int offset = (currentTime / 100) % 20;
  
  // Grid hexagonal animado
  for(int y = -10; y < 74; y += 20) {
    for(int x = -10; x < 138; x += 15) {
      int drawX = x + (offset / 2);
      int drawY = y + offset;
      
      if(drawX > -5 && drawX < 133 && drawY > -5 && drawY < 69) {
        // Hexágono simples
        if(random(100) < 30) {
          display.drawPixel(drawX, drawY, SSD1306_WHITE);
          display.drawPixel(drawX + 1, drawY - 1, SSD1306_WHITE);
          display.drawPixel(drawX + 1, drawY + 1, SSD1306_WHITE);
          display.drawPixel(drawX - 1, drawY - 1, SSD1306_WHITE);
          display.drawPixel(drawX - 1, drawY + 1, SSD1306_WHITE);
          display.drawPixel(drawX, drawY + 2, SSD1306_WHITE);
          display.drawPixel(drawX, drawY - 2, SSD1306_WHITE);
        }
      }
    }
  }
}

void drawTerminalBoot(unsigned long elapsed) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  int lineDelay = 400;
  int currentLine = elapsed / lineDelay;
  
  for(int i = 0; i <= currentLine && i < 7; i++) {
    if(i < 7) {
      display.setCursor(2, i * 9);
      
      // Efeito de digitação
      int charDelay = 50;
      int charsToShow = (elapsed - (i * lineDelay)) / charDelay;
      String line = terminalLines[i];
      
      if(charsToShow >= line.length()) {
        display.print(line);
        display.print(" [OK]");
      } else if(charsToShow > 0) {
        display.print(line.substring(0, charsToShow));
        
        // Cursor piscante
        if((millis() / 150) % 2 == 0) {
          display.print("_");
        }
      }
    }
  }
  
  // Barra de progresso futurística
  if(elapsed > 2800) {
    int progress = ((elapsed - 2800) * 100) / 800;
    progress = min(progress, 100);
    
    display.drawRect(10, 58, 108, 4, SSD1306_WHITE);
    
    // Barra com efeito de energia
    for(int i = 0; i < (progress * 106) / 100; i += 2) {
      display.drawLine(11 + i, 59, 11 + i, 60, SSD1306_WHITE);
      
      // Pontos de energia
      if(i % 8 == 0 && random(100) < 50) {
        display.drawPixel(11 + i, 58, SSD1306_WHITE);
        display.drawPixel(11 + i, 61, SSD1306_WHITE);
      }
    }
    
    display.setCursor(50, 50);
    display.print(progress);
    display.print("%");
  }
}

void drawBootAnimation() {
  display.clearDisplay();
  unsigned long elapsed = millis() - bootStartTime;
  
  if(!matrixInitialized) {
    initBootAnimation();
  }
  
  if(elapsed < 1000) {
    // Fase 1: Matrix Digital Rain Intro (0-1s)
    drawMatrixRain();
    updateCyberParticles();
    drawCyberParticles();
    
    // Logo aparecendo gradualmente
    if(elapsed > 600) {
      display.fillRect(30, 25, 68, 15, SSD1306_BLACK);
      display.drawRect(29, 24, 70, 17, SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(35, 28);
      display.print("MINI CONSOLE");
    }
  }
  else if(elapsed < 2200) {
    // Fase 2: Glitch Transition + Matrix (1-2.2s)
    drawMatrixRain();
    drawGlitchEffect();
    updateCyberParticles();
    drawCyberParticles();
    
    // Logo com efeito glitch
    for(int i = 0; i < 3; i++) {
      int offsetX = random(-3, 4);
      int offsetY = random(-1, 2);
      
      if(i == 0) display.setTextColor(SSD1306_WHITE);
      
      display.setTextSize(2);
      display.setCursor(25 + offsetX, 15 + offsetY);
      display.print("MINI");
      display.setCursor(15 + offsetX, 35 + offsetY);
      display.print("CONSOLE");
    }
    
    drawScanLines();
  }
  else if(elapsed < 3600) {
    // Fase 3: Terminal Boot Sequence (2.2-3.6s)
    display.fillRect(0, 0, 128, 64, SSD1306_BLACK);
    drawHexGrid();
    drawTerminalBoot(elapsed - 2200);
  }
  else if(elapsed < 4400) {
    // Fase 4: Final Assembly + Cyber Effects (3.6-4.4s)
    drawMatrixRain();
    updateCyberParticles();
    drawCyberParticles();
    
    // Logo final com efeito holográfico
    display.fillRect(10, 10, 108, 44, SSD1306_BLACK);
    display.drawRect(9, 9, 110, 46, SSD1306_WHITE);
    display.drawRect(8, 8, 112, 48, SSD1306_WHITE);
    
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 18);
    display.print("MINI");
    display.setCursor(15, 38);
    display.print("CONSOLE");
    
    // Efeitos holográficos nos cantos
    int corners[][2] = {{5,5}, {120,5}, {5,56}, {120,56}};
    for(int i = 0; i < 4; i++) {
      if((elapsed / 80 + i) % 6 < 3) {
        int cx = corners[i][0];
        int cy = corners[i][1];
        
        // Crosshair holográfico
        display.drawLine(cx-4, cy, cx+4, cy, SSD1306_WHITE);
        display.drawLine(cx, cy-4, cx, cy+4, SSD1306_WHITE);
        display.drawRect(cx-2, cy-2, 5, 5, SSD1306_WHITE);
      }
    }
    
    // Hexágonos flutuantes
    for(int i = 0; i < 6; i++) {
      int x = 20 + i * 15;
      int y = 5 + sin((elapsed + i * 500) * 0.01) * 3;
      if((elapsed / 100 + i) % 8 < 4) {
        display.drawPixel(x, y, SSD1306_WHITE);
        display.drawPixel(x+1, y-1, SSD1306_WHITE);
        display.drawPixel(x+1, y+1, SSD1306_WHITE);
        display.drawPixel(x-1, y-1, SSD1306_WHITE);
        display.drawPixel(x-1, y+1, SSD1306_WHITE);
      }
    }
    
    // "NEURAL INTERFACE READY" piscando
    if((elapsed / 200) % 3 == 0) {
      display.setTextSize(1);
      display.setCursor(15, 58);
      display.print("NEURAL INTERFACE READY");
    }
  }
  else {
    // Auto-transition para o menu
    bootComplete = true;
    currentGame = MENU;
  }
  
  display.display();
}

// ==================== OTA CONFIG ====================
const char* ssid = "FAMILIA SANTOS 2.4G";
const char* password = "joaovitor123456";
bool otaMode = false;
bool wifiConnected = false;
int otaProgress = 0;
bool otaInProgress = false;
unsigned int otaCurrentBytes = 0;
unsigned int otaTotalBytes = 0;

// ==================== MENU PRINCIPAL ====================
const char* gameNames[] = {
  "1. TETRIS", "2. SNAKE", "3. GENIUS", "4. JOGO DA VELHA", "5. DINO", "6. PONG",
  "7. SPACE INVADERS", "8. BREAKOUT", "9. FROGGER", "10. ASTEROIDS", 
  "11. CENTIPEDE", "12. OTA UPDATE"
};

void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Título
  display.setCursor(35, 0);
  display.println("MINI CONSOLE");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  // Calcular scroll do menu
  int startIndex = 0;
  int maxVisible = 6;
  
  if(selectedGame >= maxVisible) {
    startIndex = selectedGame - maxVisible + 1;
  }
  
  // Lista de jogos com scroll
  for(int i = 0; i < min(NUM_GAMES, maxVisible); i++) {
    int gameIndex = startIndex + i;
    if(gameIndex >= NUM_GAMES) break;
    
    if(gameIndex == selectedGame) {
      display.fillRect(0, 15 + i*8, 128, 8, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(5, 16 + i*8);
    display.println(gameNames[gameIndex]);
  }
  
  // Barra de scroll
  if(NUM_GAMES > maxVisible) {
    int scrollHeight = (maxVisible * 48) / NUM_GAMES;
    int scrollPos = (selectedGame * (48 - scrollHeight)) / (NUM_GAMES - 1);
    display.drawLine(125, 15, 125, 63, SSD1306_WHITE);
    display.fillRect(124, 15 + scrollPos, 3, scrollHeight, SSD1306_WHITE);
  }
  
  display.display();
}

// ==================== OTA UPDATE ====================
void setupOTA() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 20);
  display.print("Conectando WiFi...");
  display.display();
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
    display.setCursor(50, 35);
    display.print(attempts);
    display.print("/20");
    display.display();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    
    ArduinoOTA.setHostname("MiniConsole");
    ArduinoOTA.setPassword("5581989");
    
    ArduinoOTA.onStart([]() {
      otaInProgress = true;
      otaProgress = 0;
      otaCurrentBytes = 0;
      otaTotalBytes = 0;
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(30, 15);
      display.print("OTA Update");
      display.setCursor(35, 25);
      display.print("Starting...");
      display.display();
    });
    
    ArduinoOTA.onEnd([]() {
      otaInProgress = false;
      display.clearDisplay();
      display.setCursor(20, 25);
      display.print("Update Complete!");
      display.setCursor(35, 35);
      display.print("Rebooting...");
      display.display();
      delay(2000);
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      otaProgress = (progress / (total / 100));
      otaCurrentBytes = progress;
      otaTotalBytes = total;
      
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(35, 5);
      display.print("OTA Update");
      
      display.drawRect(10, 18, 108, 12, SSD1306_WHITE);
      int barWidth = (otaProgress * 106) / 100;
      display.fillRect(11, 19, barWidth, 10, SSD1306_WHITE);
      
      display.setCursor(55, 35);
      display.print(otaProgress);
      display.print("%");
      
      display.setCursor(15, 45);
      display.print(otaCurrentBytes);
      display.print("/");
      display.print(otaTotalBytes);
      display.print(" bytes");
      
      display.setCursor(20, 55);
      display.print("BACK para cancelar");
      display.display();
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
      otaInProgress = false;
      display.clearDisplay();
      display.setCursor(35, 20);
      display.print("OTA Error:");
      display.setCursor(45, 30);
      if (error == OTA_AUTH_ERROR) display.print("Auth");
      else if (error == OTA_BEGIN_ERROR) display.print("Begin");
      else if (error == OTA_CONNECT_ERROR) display.print("Connect");
      else if (error == OTA_RECEIVE_ERROR) display.print("Receive");
      else if (error == OTA_END_ERROR) display.print("End");
      display.display();
      delay(3000);
    });
    
    ArduinoOTA.begin();
    otaMode = true;
  } else {
    wifiConnected = false;
  }
}

void updateOTA() {
  if (!otaMode) return;
  
  if (wifiConnected) {
    ArduinoOTA.handle();
  }
  
  if (buttons[5].justPressed) {
    otaMode = false;
    wifiConnected = false;
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    currentGame = MENU;
    return;
  }
}

void drawOTA() {
  if (otaInProgress) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  if (!wifiConnected) {
    display.setCursor(25, 15);
    display.print("WiFi Desconectado");
    display.setCursor(30, 30);
    display.print("Verifique as");
    display.setCursor(32, 40);
    display.print("credenciais");
    display.setCursor(25, 55);
    display.print("BACK para voltar");
  } else {
    display.setCursor(35, 5);
    display.print("OTA READY");
    
    display.setCursor(5, 20);
    display.print("IP: ");
    display.print(WiFi.localIP());
    
    display.setCursor(15, 32);
    display.print("Aguardando upload...");
    
    display.setCursor(10, 45);
    display.print("Use Arduino IDE:");
    display.setCursor(5, 55);
    display.print("Sketch > Upload OTA");
  }
  
  display.display();
}

// ==================== TETRIS ====================
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 16
#define BLOCK_SIZE 4

int tetrisBoard[BOARD_HEIGHT][BOARD_WIDTH];
int currentPiece[4][4];
int pieceX, pieceY, pieceType;
unsigned long tetrisLastDrop = 0;
int tetrisScore = 0;
bool tetrisGameOver = false;
int tetrisLines = 0;

const int pieces[7][4][4] = {
  {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}},
  {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
  {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
  {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
  {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
  {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
  {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}
};

void initTetris() {
  memset(tetrisBoard, 0, sizeof(tetrisBoard));
  tetrisScore = 0;
  tetrisLines = 0;
  tetrisGameOver = false;
  spawnNewPiece();
}

void spawnNewPiece() {
  pieceType = random(7);
  pieceX = BOARD_WIDTH/2 - 2;
  pieceY = 0;
  memcpy(currentPiece, pieces[pieceType], sizeof(currentPiece));
}

bool isValidPosition(int dx, int dy) {
  for(int y = 0; y < 4; y++) {
    for(int x = 0; x < 4; x++) {
      if(currentPiece[y][x]) {
        int newX = pieceX + x + dx;
        int newY = pieceY + y + dy;
        if(newX < 0 || newX >= BOARD_WIDTH || newY >= BOARD_HEIGHT || 
           (newY >= 0 && tetrisBoard[newY][newX])) {
          return false;
        }
      }
    }
  }
  return true;
}

void placePiece() {
  for(int y = 0; y < 4; y++) {
    for(int x = 0; x < 4; x++) {
      if(currentPiece[y][x] && pieceY + y >= 0) {
        tetrisBoard[pieceY + y][pieceX + x] = 1;
      }
    }
  }
  
  for(int y = BOARD_HEIGHT - 1; y >= 0; y--) {
    bool fullLine = true;
    for(int x = 0; x < BOARD_WIDTH; x++) {
      if(!tetrisBoard[y][x]) {
        fullLine = false;
        break;
      }
    }
    
    if(fullLine) {
      for(int moveY = y; moveY > 0; moveY--) {
        for(int x = 0; x < BOARD_WIDTH; x++) {
          tetrisBoard[moveY][x] = tetrisBoard[moveY-1][x];
        }
      }
      for(int x = 0; x < BOARD_WIDTH; x++) {
        tetrisBoard[0][x] = 0;
      }
      tetrisScore += 100;
      tetrisLines++;
      y++;
    }
  }
  
  spawnNewPiece();
  if(!isValidPosition(0, 0)) {
    tetrisGameOver = true;
  }
}

void rotatePiece() {
  int temp[4][4];
  for(int y = 0; y < 4; y++) {
    for(int x = 0; x < 4; x++) {
      temp[x][3-y] = currentPiece[y][x];
    }
  }
  memcpy(currentPiece, temp, sizeof(currentPiece));
  
  if(!isValidPosition(0, 0)) {
    for(int y = 0; y < 4; y++) {
      for(int x = 0; x < 4; x++) {
        temp[3-x][y] = currentPiece[y][x];
      }
    }
    memcpy(currentPiece, temp, sizeof(currentPiece));
  }
}

void updateTetris() {
  if(tetrisGameOver) return;
  
  if(millis() - tetrisLastDrop > 400) {
    if(isValidPosition(0, 1)) {
      pieceY++;
    } else {
      placePiece();
    }
    tetrisLastDrop = millis();
  }
  
  if(buttons[0].justPressed) {
    rotatePiece();
  }
  if(buttons[1].pressed) {
    if(isValidPosition(0, 1)) pieceY++;
  }
  if(buttons[2].justPressed) {
    if(isValidPosition(-1, 0)) pieceX--;
  }
  if(buttons[3].justPressed) {
    if(isValidPosition(1, 0)) pieceX++;
  }
}

void drawTetris() {
  display.clearDisplay();
  
  for(int y = 0; y < BOARD_HEIGHT; y++) {
    for(int x = 0; x < BOARD_WIDTH; x++) {
      if(tetrisBoard[y][x]) {
        display.fillRect(x * BLOCK_SIZE + 44, y * BLOCK_SIZE, 
                        BLOCK_SIZE-1, BLOCK_SIZE-1, SSD1306_WHITE);
      } else {
        display.drawPixel(x * BLOCK_SIZE + 44, y * BLOCK_SIZE, SSD1306_WHITE);
      }
    }
  }
  
  for(int y = 0; y < 4; y++) {
    for(int x = 0; x < 4; x++) {
      if(currentPiece[y][x]) {
        int drawX = (pieceX + x) * BLOCK_SIZE + 44;
        int drawY = (pieceY + y) * BLOCK_SIZE;
        if(drawY >= 0) {
          display.fillRect(drawX, drawY, BLOCK_SIZE-1, BLOCK_SIZE-1, SSD1306_WHITE);
        }
      }
    }
  }
  
  display.drawLine(43, 0, 43, 64, SSD1306_WHITE);
  display.drawLine(84, 0, 84, 64, SSD1306_WHITE);
  display.drawLine(43, 64, 84, 64, SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("TETRIS");
  
  display.setCursor(0, 15);
  display.print("Score");
  display.setCursor(0, 25);
  display.print(tetrisScore);
  
  display.setCursor(0, 40);
  display.print("Lines");
  display.setCursor(0, 50);
  display.print(tetrisLines);
  
  display.setCursor(90, 0);
  display.print("Next");
  display.drawRect(88, 10, 16, 16, SSD1306_WHITE);
  
  if(tetrisGameOver) {
    display.fillRect(45, 20, 38, 25, SSD1306_BLACK);
    display.drawRect(45, 20, 38, 25, SSD1306_WHITE);
    display.setCursor(50, 28);
    display.print("GAME");
    display.setCursor(50, 38);
    display.print("OVER");
  }
  
  display.display();
}

// ==================== SNAKE ====================
#define SNAKE_MAX_LENGTH 100

struct Point {
  int x, y;
};

Point snake[SNAKE_MAX_LENGTH];
Point food;
int snakeLength;
int snakeDir;
int snakeScore;
unsigned long snakeLastMove = 0;

void initSnake() {
  snakeLength = 3;
  snakeDir = 1;
  snakeScore = 0;
  snakeGameOver = false;
  
  snake[0] = {10, 5};
  snake[1] = {9, 5};
  snake[2] = {8, 5};
  
  spawnFood();
}

void spawnFood() {
  do {
    food.x = random(1, 31);
    food.y = random(1, 15);
  } while(isSnakePosition(food.x, food.y));
}

bool isSnakePosition(int x, int y) {
  for(int i = 0; i < snakeLength; i++) {
    if(snake[i].x == x && snake[i].y == y) {
      return true;
    }
  }
  return false;
}

void updateSnake() {
  if(snakeGameOver) return;
  
  if(buttons[0].justPressed && snakeDir != 2) {
    snakeDir = 0;
  }
  if(buttons[1].justPressed && snakeDir != 0) {
    snakeDir = 2;
  }
  if(buttons[2].justPressed && snakeDir != 1) {
    snakeDir = 3;
  }
  if(buttons[3].justPressed && snakeDir != 3) {
    snakeDir = 1;
  }
  
  if(millis() - snakeLastMove > 120) {
    Point newHead = snake[0];
    
    switch(snakeDir) {
      case 0: newHead.y--; break;
      case 1: newHead.x++; break;
      case 2: newHead.y++; break;
      case 3: newHead.x--; break;
    }
    
    if(newHead.x < 1 || newHead.x > 30 || newHead.y < 1 || newHead.y > 14 ||
       isSnakePosition(newHead.x, newHead.y)) {
      snakeGameOver = true;
      return;
    }
    
    for(int i = snakeLength - 1; i > 0; i--) {
      snake[i] = snake[i-1];
    }
    snake[0] = newHead;
    
    if(newHead.x == food.x && newHead.y == food.y) {
      snakeLength++;
      snakeScore += 10;
      spawnFood();
    }
    
    snakeLastMove = millis();
  }
}

void drawSnake() {
  display.clearDisplay();
  
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  
  for(int i = 0; i < snakeLength; i++) {
    int x = snake[i].x * 4;
    int y = snake[i].y * 4;
    if(i == 0) {
      display.fillRect(x-1, y-1, 3, 3, SSD1306_WHITE);
    } else {
      display.drawRect(x-1, y-1, 3, 3, SSD1306_WHITE);
    }
  }
  
  display.fillRect(food.x * 4 - 1, food.y * 4 - 1, 3, 3, SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 2);
  display.print("Score: ");
  display.print(snakeScore);
  
  if(snakeGameOver) {
    display.fillRect(30, 20, 70, 25, SSD1306_BLACK);
    display.drawRect(30, 20, 70, 25, SSD1306_WHITE);
    display.setCursor(40, 30);
    display.print("GAME OVER");
  }
  
  display.display();
}

// ==================== GENIUS ====================
#define SEQUENCE_MAX 20

int geniusSequence[SEQUENCE_MAX];
int geniusLevel;
int geniusStep;
bool geniusShowingSequence;
bool geniusWaitingInput;
unsigned long geniusLastBlink = 0;
int geniusCurrentBlink = -1;
int geniusButtonPressed = -1;
unsigned long geniusButtonPressTime = 0;

void initGenius() {
  geniusLevel = 0;
  geniusStep = 0;
  geniusShowingSequence = false;
  geniusWaitingInput = false;
  geniusGameOver = false;
  geniusButtonPressed = -1;
  startNewGeniusLevel();
}

void startNewGeniusLevel() {
  geniusLevel++;
  geniusStep = 0;
  geniusSequence[geniusLevel - 1] = random(4);
  geniusShowingSequence = true;
  geniusWaitingInput = false;
  geniusLastBlink = millis();
  geniusCurrentBlink = 0;
}

void updateGenius() {
  if(geniusGameOver) return;
  
  if(geniusButtonPressed != -1 && millis() - geniusButtonPressTime > 200) {
    geniusButtonPressed = -1;
  }
  
  if(geniusShowingSequence) {
    if(millis() - geniusLastBlink > 600) {
      geniusCurrentBlink++;
      if(geniusCurrentBlink > geniusLevel) {
        geniusShowingSequence = false;
        geniusWaitingInput = true;
        geniusStep = 0;
      }
      geniusLastBlink = millis();
    }
  }
  
  if(geniusWaitingInput) {
    int pressedButton = -1;
    
    if(buttons[0].justPressed) pressedButton = 0;
    if(buttons[1].justPressed) pressedButton = 1;
    if(buttons[2].justPressed) pressedButton = 2;
    if(buttons[3].justPressed) pressedButton = 3;
    
    if(pressedButton != -1) {
      geniusButtonPressed = pressedButton;
      geniusButtonPressTime = millis();
      
      if(pressedButton == geniusSequence[geniusStep]) {
        geniusStep++;
        if(geniusStep >= geniusLevel) {
          if(geniusLevel >= SEQUENCE_MAX) {
            geniusGameOver = true;
          } else {
            delay(500);
            startNewGeniusLevel();
          }
        }
      } else {
        geniusGameOver = true;
      }
    }
  }
}

void drawGenius() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(45, 5);
  display.print("GENIUS");
  display.setCursor(40, 15);
  display.print("Level: ");
  display.print(geniusLevel);
  
  bool highlight[4] = {false};
  
  if(geniusShowingSequence && geniusCurrentBlink <= geniusLevel && geniusCurrentBlink > 0) {
    int blinkButton = geniusSequence[geniusCurrentBlink - 1];
    if((millis() - geniusLastBlink) < 300) {
      highlight[blinkButton] = true;
    }
  }
  
  if(geniusButtonPressed != -1) {
    highlight[geniusButtonPressed] = true;
  }
  
  if(highlight[0]) {
    display.fillRect(50, 25, 25, 15, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRect(50, 25, 25, 15, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(58, 30);
  display.print("UP");
  
  display.setTextColor(SSD1306_WHITE);
  if(highlight[1]) {
    display.fillRect(50, 45, 25, 15, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRect(50, 45, 25, 15, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(55, 50);
  display.print("DOWN");
  
  display.setTextColor(SSD1306_WHITE);
  if(highlight[2]) {
    display.fillRect(20, 35, 25, 15, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRect(20, 35, 25, 15, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(26, 40);
  display.print("LEFT");
  
  display.setTextColor(SSD1306_WHITE);
  if(highlight[3]) {
    display.fillRect(80, 35, 30, 15, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRect(80, 35, 30, 15, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(85, 40);
  display.print("RIGHT");
  
  if(geniusGameOver) {
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(35, 58);
    if(geniusLevel >= SEQUENCE_MAX) {
      display.print("YOU WIN!");
    } else {
      display.print("GAME OVER");
    }
  }
  
  display.display();
}

// ==================== JOGO DA VELHA ====================
int tttBoard[3][3];
int tttCurrentPlayer;
int tttCursorX, tttCursorY;
int tttWinner;
int tttScoreX = 0;
int tttScoreO = 0;

void initTicTacToe() {
  memset(tttBoard, 0, sizeof(tttBoard));
  tttCurrentPlayer = 1;
  tttCursorX = 1;
  tttCursorY = 1;
  tttGameOver = false;
  tttWinner = -1;
}

int checkTicTacToeWin() {
  for(int i = 0; i < 3; i++) {
    if(tttBoard[i][0] && tttBoard[i][0] == tttBoard[i][1] && tttBoard[i][1] == tttBoard[i][2]) {
      return tttBoard[i][0];
    }
  }
  
  for(int i = 0; i < 3; i++) {
    if(tttBoard[0][i] && tttBoard[0][i] == tttBoard[1][i] && tttBoard[1][i] == tttBoard[2][i]) {
      return tttBoard[0][i];
    }
  }
  
  if(tttBoard[0][0] && tttBoard[0][0] == tttBoard[1][1] && tttBoard[1][1] == tttBoard[2][2]) {
    return tttBoard[0][0];
  }
  if(tttBoard[0][2] && tttBoard[0][2] == tttBoard[1][1] && tttBoard[1][1] == tttBoard[2][0]) {
    return tttBoard[0][2];
  }
  
  bool full = true;
  for(int i = 0; i < 3; i++) {
    for(int j = 0; j < 3; j++) {
      if(tttBoard[i][j] == 0) {
        full = false;
        break;
      }
    }
  }
  
  return full ? 0 : -1;
}

void updateTicTacToe() {
  if(tttGameOver) return;
  
  if(buttons[0].justPressed) {
    tttCursorY = (tttCursorY - 1 + 3) % 3;
  }
  if(buttons[1].justPressed) {
    tttCursorY = (tttCursorY + 1) % 3;
  }
  if(buttons[2].justPressed) {
    tttCursorX = (tttCursorX - 1 + 3) % 3;
  }
  if(buttons[3].justPressed) {
    tttCursorX = (tttCursorX + 1) % 3;
  }
  
  if(buttons[4].justPressed) {
    if(tttBoard[tttCursorY][tttCursorX] == 0) {
      tttBoard[tttCursorY][tttCursorX] = tttCurrentPlayer;
      tttCurrentPlayer = (tttCurrentPlayer == 1) ? 2 : 1;
      
      tttWinner = checkTicTacToeWin();
      if(tttWinner != -1) {
        tttGameOver = true;
        if(tttWinner == 1) tttScoreX++;
        else if(tttWinner == 2) tttScoreO++;
      }
    }
  }
}

void drawTicTacToe() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(25, 0);
  display.print("JOGO DA VELHA");
  
  display.setCursor(0, 12);
  display.print("O:");
  display.print(tttScoreO);
  
  display.setCursor(110, 12);
  display.print("X:");
  display.print(tttScoreX);
  
  display.setCursor(55, 12);
  display.print(tttCurrentPlayer == 1 ? "X" : "O");
  
  int boardX = 35;
  int boardY = 22;
  
  display.drawLine(boardX + 15, boardY, boardX + 15, boardY + 45, SSD1306_WHITE);
  display.drawLine(boardX + 30, boardY, boardX + 30, boardY + 45, SSD1306_WHITE);
  display.drawLine(boardX, boardY + 15, boardX + 45, boardY + 15, SSD1306_WHITE);
  display.drawLine(boardX, boardY + 30, boardX + 45, boardY + 30, SSD1306_WHITE);
  
  if(!tttGameOver) {
    int cursorDrawX = boardX + tttCursorX * 15;
    int cursorDrawY = boardY + tttCursorY * 15;
    display.drawRect(cursorDrawX + 1, cursorDrawY + 1, 13, 13, SSD1306_WHITE);
  }
  
  display.setTextSize(2);
  for(int i = 0; i < 3; i++) {
    for(int j = 0; j < 3; j++) {
      if(tttBoard[i][j] != 0) {
        int x = boardX + j * 15 + 4;
        int y = boardY + i * 15 + 3;
        display.setCursor(x, y);
        display.print(tttBoard[i][j] == 1 ? "X" : "O");
      }
    }
  }
  
  if(tttGameOver) {
    display.setTextSize(1);
    display.setCursor(85, 35);
    if(tttWinner == 0) {
      display.print("EMPATE!");
    } else {
      display.print("GANHOU:");
      display.setCursor(90, 45);
      display.print(tttWinner == 1 ? "X" : "O");
    }
  }
  
  display.display();
}

// ==================== DINO GAME - ULTRA CORRIGIDO ====================
struct DinoObstacle {
  int x;
  int type;
  bool active;
};

// Variáveis do jogo Dino - com espaçamento corrigido
int dinoY = 50;              
int dinoVelocityY = 0;       
bool dinoOnGround = true;    
bool dinoCrouch = false;     
DinoObstacle obstacles[3];   
int dinoScore = 0;
bool dinoGameOver = false;
unsigned long dinoLastSpawn = 0;
int dinoSpeed = 2;           
unsigned long dinoLastSpeedIncrease = 0;

// Constantes do Dino
const int DINO_GROUND_Y = 50;     
const int JUMP_FORCE = -8;        
const int GRAVITY = 1;
const int MIN_OBSTACLE_DISTANCE = 80;  // Distância mínima entre obstáculos

void initDino() {
  dinoY = DINO_GROUND_Y;
  dinoVelocityY = 0;
  dinoOnGround = true;
  dinoCrouch = false;
  dinoScore = 0;
  dinoGameOver = false;
  dinoSpeed = 2;  // Velocidade inicial mais baixa
  dinoLastSpeedIncrease = millis();
  dinoLastSpawn = 0;
  
  // Limpar obstáculos
  for(int i = 0; i < 3; i++) {
    obstacles[i].active = false;
  }
}

void drawDinoCharacter(int x, int y) {
  if(dinoCrouch) {
    display.fillRect(x, y + 6, 12, 4, SSD1306_WHITE);      
    display.fillRect(x + 10, y + 8, 4, 2, SSD1306_WHITE);  
    display.fillRect(x + 2, y + 10, 2, 2, SSD1306_WHITE);  
    display.fillRect(x + 6, y + 10, 2, 2, SSD1306_WHITE);  
    display.drawPixel(x + 11, y + 8, SSD1306_WHITE);       
  } else {
    display.fillRect(x + 2, y, 6, 4, SSD1306_WHITE);       
    display.fillRect(x, y + 4, 8, 6, SSD1306_WHITE);       
    display.fillRect(x + 8, y + 6, 2, 4, SSD1306_WHITE);   
    display.fillRect(x + 2, y + 10, 2, 3, SSD1306_WHITE);  
    display.fillRect(x + 5, y + 10, 2, 3, SSD1306_WHITE);  
    display.drawPixel(x + 4, y + 1, SSD1306_WHITE);        
    display.drawLine(x + 1, y + 2, x + 1, y + 3, SSD1306_WHITE); 
  }
}

void drawDinoObstacle(int x, int type) {
  switch(type) {
    case 0: // Cactus baixo
      display.fillRect(x, 55, 4, 8, SSD1306_WHITE);
      display.drawPixel(x - 1, 57, SSD1306_WHITE);
      display.drawPixel(x + 4, 59, SSD1306_WHITE);
      break;
    case 1: // Cactus alto
      display.fillRect(x, 50, 4, 13, SSD1306_WHITE);
      display.drawPixel(x - 1, 52, SSD1306_WHITE);
      display.drawPixel(x + 4, 54, SSD1306_WHITE);
      display.drawPixel(x - 1, 58, SSD1306_WHITE);
      break;
    case 2: // Pássaro
      display.drawLine(x, 40, x + 4, 40, SSD1306_WHITE);
      display.drawLine(x + 1, 41, x + 3, 41, SSD1306_WHITE);
      display.drawLine(x, 42, x + 4, 42, SSD1306_WHITE);
      display.drawPixel(x + 4, 41, SSD1306_WHITE);
      break;
  }
}

bool canSpawnObstacle() {
  // Verificar se há espaço suficiente entre obstáculos
  for(int i = 0; i < 3; i++) {
    if(obstacles[i].active && obstacles[i].x > (128 - MIN_OBSTACLE_DISTANCE)) {
      return false;
    }
  }
  return true;
}

void updateDino() {
  if(dinoGameOver) return;
  
  // Aumentar velocidade muito gradualmente
  if(millis() - dinoLastSpeedIncrease > 5000) {  // A cada 5 segundos
    dinoSpeed++;
    if(dinoSpeed > 6) dinoSpeed = 6;  // Velocidade máxima menor
    dinoLastSpeedIncrease = millis();
  }
  
  dinoCrouch = buttons[1].pressed;
  
  // Sistema de pulo
  if((buttons[0].justPressed || buttons[4].justPressed) && dinoOnGround && !dinoCrouch) {
    dinoVelocityY = JUMP_FORCE;  
    dinoOnGround = false;        
  }
  
  // Física do movimento vertical
  if(!dinoOnGround) {
    dinoY += dinoVelocityY;          
    dinoVelocityY += GRAVITY;        
    
    if(dinoY >= DINO_GROUND_Y) {
      dinoY = DINO_GROUND_Y;
      dinoVelocityY = 0;
      dinoOnGround = true;
    }
  }
  
  // SPAWN DE OBSTÁCULOS COM ESPAÇAMENTO CORRIGIDO
  unsigned long baseInterval = 1500;  // Intervalo base maior
  unsigned long speedReduction = min(dinoSpeed * 100, 800);
  unsigned long spawnInterval = max(baseInterval - speedReduction, 700UL);
  
  if(millis() - dinoLastSpawn > spawnInterval && canSpawnObstacle()) {
    for(int i = 0; i < 3; i++) {
      if(!obstacles[i].active) {
        obstacles[i].x = 128 + random(20, 60);  // Posição inicial mais afastada
        obstacles[i].type = random(3);
        obstacles[i].active = true;
        dinoLastSpawn = millis();
        break;
      }
    }
  }
  
  // Mover obstáculos e verificar colisões
  for(int i = 0; i < 3; i++) {
    if(obstacles[i].active) {
      obstacles[i].x -= dinoSpeed;
      
      // Detecção de colisão melhorada
      if(obstacles[i].x < 25 && obstacles[i].x > 0) {
        bool collision = false;
        
        switch(obstacles[i].type) {
          case 0: // Cactus baixo
            if(dinoY >= 47 && !dinoCrouch) collision = true;
            break;
          case 1: // Cactus alto
            if(dinoY >= 42) collision = true;
            break;
          case 2: // Pássaro
            if(dinoY <= 45 && dinoY >= 38 && !dinoCrouch) collision = true;
            break;
        }
        
        if(collision) {
          dinoGameOver = true;
          return;
        }
      }
      
      // Remover obstáculo e pontuar
      if(obstacles[i].x < -10) {
        obstacles[i].active = false;
        dinoScore += 10;
      }
    }
  }
}

void drawDino() {
  display.clearDisplay();
  
  // Desenhar chão
  display.drawLine(0, 63, 128, 63, SSD1306_WHITE);
  
  // Cenário baseado no score
  if(dinoScore > 30) {
    display.drawCircle(100, 10, 3, SSD1306_WHITE);
    display.drawCircle(96, 10, 2, SSD1306_WHITE);
    display.drawCircle(104, 10, 2, SSD1306_WHITE);
  }
  
  if(dinoScore > 60) {
    display.drawCircle(20, 8, 2, SSD1306_WHITE);
    display.drawCircle(23, 8, 2, SSD1306_WHITE);
  }
  
  // Desenhar dino
  drawDinoCharacter(15, dinoY);
  
  // Desenhar obstáculos
  for(int i = 0; i < 3; i++) {
    if(obstacles[i].active) {
      drawDinoObstacle(obstacles[i].x, obstacles[i].type);
    }
  }
  
  // HUD
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(dinoScore);
  
  display.setCursor(80, 0);
  display.print("Speed: ");
  display.print(dinoSpeed);
  
  // Tela de Game Over
  if(dinoGameOver) {
    display.fillRect(25, 15, 80, 30, SSD1306_BLACK);
    display.drawRect(25, 15, 80, 30, SSD1306_WHITE);
    display.drawRect(24, 14, 82, 32, SSD1306_WHITE);
    
    display.setTextSize(1);
    display.setCursor(45, 22);
    display.print("GAME OVER");
    display.setCursor(35, 32);
    display.print("Score: ");
    display.print(dinoScore);
    display.setCursor(30, 52);
    display.print("ENTER = Restart");
  }
  
  display.display();
}

// ==================== PONG ====================
struct Paddle {
  int x, y, width, height;
};

struct Ball {
  float x, y, vx, vy;
  int size;
};

Paddle player1, player2;
Ball ball;
int pongScore1, pongScore2;


void initPong() {
  player1.x = 5;
  player1.y = 25;
  player1.width = 3;
  player1.height = 15;
  
  player2.x = 120;
  player2.y = 25;
  player2.width = 3;
  player2.height = 15;
  
  ball.x = 64;
  ball.y = 32;
  ball.vx = random(2) ? 2 : -2;
  ball.vy = random(2) ? 1 : -1;
  ball.size = 2;
  
  pongScore1 = 0;
  pongScore2 = 0;
  pongGameOver = false;
}

void updatePong() {
  if(pongGameOver) return;
  
  if(buttons[0].pressed && player1.y > 0) {
    player1.y -= 3;
  }
  if(buttons[1].pressed && player1.y < 64 - player1.height) {
    player1.y += 3;
  }
  
  if(buttons[2].pressed && player2.y > 0) {
    player2.y -= 3;
  }
  if(buttons[3].pressed && player2.y < 64 - player2.height) {
    player2.y += 3;
  }
  
  ball.x += ball.vx;
  ball.y += ball.vy;
  
  if(ball.y <= 0 || ball.y >= 64 - ball.size) {
    ball.vy = -ball.vy;
  }
  
  if(ball.x <= player1.x + player1.width && ball.x >= player1.x &&
     ball.y + ball.size >= player1.y && ball.y <= player1.y + player1.height) {
    ball.vx = abs(ball.vx);
    float hitPos = (ball.y - player1.y) / (float)player1.height;
    ball.vy = (hitPos - 0.5) * 3;
  }
  
  if(ball.x + ball.size >= player2.x && ball.x <= player2.x + player2.width &&
     ball.y + ball.size >= player2.y && ball.y <= player2.y + player2.height) {
    ball.vx = -abs(ball.vx);
    float hitPos = (ball.y - player2.y) / (float)player2.height;
    ball.vy = (hitPos - 0.5) * 3;
  }
  
  if(ball.x < 0) {
    pongScore2++;
    ball.x = 64;
    ball.y = 32;
    ball.vx = 2;
    ball.vy = random(2) ? 1 : -1;
  }
  
  if(ball.x > 128) {
    pongScore1++;
    ball.x = 64;
    ball.y = 32;
    ball.vx = -2;
    ball.vy = random(2) ? 1 : -1;
  }
  
  if(pongScore1 >= 5 || pongScore2 >= 5) {
    pongGameOver = true;
  }
}

void drawPong() {
  display.clearDisplay();
  
  for(int i = 0; i < 64; i += 4) {
    display.drawPixel(64, i, SSD1306_WHITE);
  }
  
  display.fillRect(player1.x, player1.y, player1.width, player1.height, SSD1306_WHITE);
  display.fillRect(player2.x, player2.y, player2.width, player2.height, SSD1306_WHITE);
  
  display.fillRect((int)ball.x, (int)ball.y, ball.size, ball.size, SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 5);
  display.print(pongScore1);
  display.setCursor(95, 5);
  display.print(pongScore2);
  
  display.setCursor(45, 5);
  display.print("PONG");
  
  if(pongGameOver) {
    display.setCursor(35, 55);
    display.print("P");
    display.print(pongScore1 >= 5 ? "1" : "2");
    display.print(" WINS!");
  }
  
  display.display();
}

// ==================== SISTEMA PRINCIPAL ====================

void readButtons() {
  int pins[] = {BTN_UP_VAL, BTN_DOWN_VAL, BTN_LEFT_VAL, BTN_RIGHT_VAL, 
                BTN_ENTER_VAL, BTN_BACK_VAL, BTN_A};
  
  for(int i = 0; i < 7; i++) {
    bool reading = !digitalRead(pins[i]);
    
    buttons[i].justPressed = false;
    
    if(reading != buttons[i].lastState) {
      buttons[i].lastDebounce = millis();
    }
    
    if((millis() - buttons[i].lastDebounce) > buttons[i].debounceDelay) {
      if(reading != buttons[i].currentState) {
        buttons[i].currentState = reading;
        
        if(reading && !buttons[i].pressed) {
          buttons[i].justPressed = true;
          buttons[i].lastRepeat = millis();
          buttons[i].pressStartTime = millis();
          buttons[i].isRepeating = false;
        }
        
        buttons[i].pressed = reading;
        
        if(!reading) {
          buttons[i].isRepeating = false;
        }
      }
      
      if(buttons[i].pressed && !buttons[i].justPressed) {
        unsigned long currentTime = millis();
        unsigned long repeatTime = buttons[i].isRepeating ? buttons[i].fastRepeat : buttons[i].repeatDelay;
        
        if(currentTime - buttons[i].lastRepeat > repeatTime) {
          buttons[i].justPressed = true;
          buttons[i].lastRepeat = currentTime;
          buttons[i].isRepeating = true;
        }
      }
    }
    
    buttons[i].lastState = reading;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Desligar WiFi inicialmente
  WiFi.mode(WIFI_OFF);
  
  // Configurar pinos dos botões
  pinMode(BTN_UP_VAL, INPUT_PULLUP);
  pinMode(BTN_DOWN_VAL, INPUT_PULLUP);
  pinMode(BTN_LEFT_VAL, INPUT_PULLUP);
  pinMode(BTN_RIGHT_VAL, INPUT_PULLUP);
  pinMode(BTN_ENTER_VAL, INPUT_PULLUP);
  pinMode(BTN_BACK_VAL, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  
  // Inicializar display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha na inicialização do display OLED"));
    for(;;);
  }
  
  randomSeed(analogRead(0));
  
  // Iniciar com animação de boot ultra
  bootStartTime = millis();
  currentGame = BOOT_ANIMATION;
}

void loop() {
  readButtons();
  
  switch(currentGame) {
    case BOOT_ANIMATION:
      drawBootAnimation();
      // Pode pular a animação pressionando qualquer botão
      if(buttons[4].justPressed) {
        bootComplete = true;
        currentGame = MENU;
      }
      break;
      
    case MENU:
      if(buttons[0].justPressed) {
        selectedGame = (selectedGame - 1 + NUM_GAMES) % NUM_GAMES;
      }
      if(buttons[1].justPressed) {
        selectedGame = (selectedGame + 1) % NUM_GAMES;
      }
    if(buttons[4].justPressed) {
    switch(selectedGame) {
    case 0: currentGame = TETRIS; initTetris(); break;
    case 1: currentGame = SNAKE; initSnake(); break;
    case 2: currentGame = GENIUS; initGenius(); break;
    case 3: currentGame = TICTACTOE; initTicTacToe(); break;
    case 4: currentGame = DINO; initDino(); break;
    case 5: currentGame = PONG; initPong(); break;
    case 6: currentGame = SPACE_INVADERS; initSpaceInvaders(); break;
    case 7: currentGame = BREAKOUT; initBreakout(); break;
    case 8: currentGame = FROGGER; initFrogger(); break;
    case 9: currentGame = ASTEROIDS; initAsteroids(); break;
    case 10: currentGame = CENTIPEDE; initCentipede(); break;
    case 11: currentGame = OTA; setupOTA(); break;
  }
}
      drawMenu();
      break;
      
    case TETRIS:
      if(buttons[5].justPressed) {
        currentGame = MENU;
      }
      updateTetris();
      drawTetris();
      if(tetrisGameOver && buttons[4].justPressed) {
        initTetris();
      }
      break;
      
    case SNAKE:
      if(buttons[5].justPressed) {
        currentGame = MENU;
      }
      updateSnake();
      drawSnake();
      if(snakeGameOver && buttons[4].justPressed) {
        initSnake();
      }
      break;
      
    case GENIUS:
      if(buttons[5].justPressed) {
        currentGame = MENU;
      }
      updateGenius();
      drawGenius();
      if(geniusGameOver && buttons[4].justPressed) {
        initGenius();
      }
      break;
      
    case TICTACTOE:
      if(buttons[5].justPressed) {
        currentGame = MENU;
      }
      updateTicTacToe();
      drawTicTacToe();
      if(tttGameOver && buttons[6].justPressed) {
        initTicTacToe();
      }
      break;
      
    case DINO:
      if(buttons[5].justPressed) {
        currentGame = MENU;
      }
      updateDino();
      drawDino();
      if(dinoGameOver && buttons[4].justPressed) {
        initDino();
      }
      break;
      
    case PONG:
      if(buttons[5].justPressed) {
        currentGame = MENU;
      }
      updatePong();
      drawPong();
      if(pongGameOver && buttons[4].justPressed) {
        initPong();
      }
      break;
      
case SPACE_INVADERS:
  if(buttons[5].justPressed) {
    currentGame = MENU;
  }
  updateSpaceInvaders();
  drawSpaceInvaders();
  if(spaceGameOver && buttons[4].justPressed) {
    initSpaceInvaders();
  }
  break;

case BREAKOUT:
  if(buttons[5].justPressed) {
    currentGame = MENU;
  }
  updateBreakout();
  drawBreakout();
  if(breakoutGameOver && buttons[4].justPressed) {
    initBreakout();
  }
  break;

case FROGGER:
  if(buttons[5].justPressed) {
    currentGame = MENU;
  }
  updateFrogger();
  drawFrogger();
  if(froggerGameOver && buttons[4].justPressed) {
    initFrogger();
  }
  break;

case ASTEROIDS:
  if(buttons[5].justPressed) {
    currentGame = MENU;
  }
  updateAsteroids();
  drawAsteroids();
  if(asteroidsGameOver && buttons[4].justPressed) {
    initAsteroids();
  }
  break;

case CENTIPEDE:
  if(buttons[5].justPressed) {
    currentGame = MENU;
  }
  updateCentipede();
  drawCentipede();
  if(centipedeGameOver && buttons[4].justPressed) {
    initCentipede();
  }
  break;

case OTA:
  updateOTA();
  drawOTA();
  break;
  }
  
  delay(16); // ~60 FPS
}





Invader invaders[55];  // 11x5 formation
Bullet bullets[20];    // Player and enemy bullets
Player spacePlayer1, spacePlayer2;
Barrier barriers[4];
int spaceScore1 = 0, spaceScore2 = 0;
int spaceWave = 1;
bool spaceTwoPlayer = false;
int spaceCurrentPlayer = 1;
unsigned long spaceLastMove = 0;
unsigned long spaceLastShoot = 0;
int spaceInvaderDirection = 1;
bool spaceInvaderDropped = false;
int spaceInvaderSpeed = 800;

void initSpaceInvaders() {
  spaceGameOver = false;
  spaceScore1 = 0;
  spaceScore2 = 0;
  spaceWave = 1;
  spaceCurrentPlayer = 1;
  spaceInvaderDirection = 1;
  spaceInvaderSpeed = 800;
  
  // Inicializar jogadores
  spacePlayer1.x = 30;
  spacePlayer1.y = 58;
  spacePlayer1.active = true;
  spacePlayer1.lives = 3;
  
  spacePlayer2.x = 90;
  spacePlayer2.y = 58;
  spacePlayer2.active = spaceTwoPlayer;
  spacePlayer2.lives = spaceTwoPlayer ? 3 : 0;
  
  // Inicializar invasores (11x5 = 55)
  for(int row = 0; row < 5; row++) {
    for(int col = 0; col < 11; col++) {
      int index = row * 11 + col;
      invaders[index].x = 8 + col * 10;
      invaders[index].y = 5 + row * 8;
      invaders[index].active = true;
      invaders[index].type = row < 1 ? 2 : (row < 3 ? 1 : 0);  // 3 tipos diferentes
      invaders[index].animFrame = 0;
    }
  }
  
  // Limpar balas
  for(int i = 0; i < 20; i++) {
    bullets[i].active = false;
  }
  
  // Inicializar barreiras
  for(int i = 0; i < 4; i++) {
    barriers[i].x = 10 + i * 30;
    barriers[i].y = 45;
    
    // Criar formato de barreira clássico
    for(int y = 0; y < 8; y++) {
      for(int x = 0; x < 16; x++) {
        barriers[i].pixels[x][y] = false;
        
        // Formato de dome com abertura
        if(y < 5) {
          if(x >= 2 && x <= 13 && (x < 6 || x > 9 || y < 2)) {
            barriers[i].pixels[x][y] = true;
          }
        } else if(y < 7) {
          if(x >= 0 && x <= 4) barriers[i].pixels[x][y] = true;
          if(x >= 11 && x <= 15) barriers[i].pixels[x][y] = true;
        }
      }
    }
  }
}

void spawnPlayerBullet(int playerNum) {
  for(int i = 0; i < 20; i++) {
    if(!bullets[i].active) {
      if(playerNum == 1 && spacePlayer1.active) {
        bullets[i].x = spacePlayer1.x + 6;
        bullets[i].y = spacePlayer1.y;
      } else if(playerNum == 2 && spacePlayer2.active && spaceTwoPlayer) {
        bullets[i].x = spacePlayer2.x + 6;
        bullets[i].y = spacePlayer2.y;
      } else return;
      
      bullets[i].vy = -4;
      bullets[i].active = true;
      bullets[i].isPlayer = true;
      break;
    }
  }
}

void spawnEnemyBullet() {
  // Encontrar invasores da linha da frente para atirar
  for(int col = 0; col < 11; col++) {
    for(int row = 4; row >= 0; row--) {
      int index = row * 11 + col;
      if(invaders[index].active && random(1000) < 3) {
        for(int i = 0; i < 20; i++) {
          if(!bullets[i].active) {
            bullets[i].x = invaders[index].x + 4;
            bullets[i].y = invaders[index].y + 6;
            bullets[i].vy = 2;
            bullets[i].active = true;
            bullets[i].isPlayer = false;
            return;
          }
        }
      }
    }
  }
}

void updateSpaceInvaders() {
  if(spaceGameOver) return;
  
  // Alternar modo 1P/2P com botão A
  if(buttons[6].justPressed) {
    spaceTwoPlayer = !spaceTwoPlayer;
    initSpaceInvaders();
    return;
  }
  
  // Controles do Player 1
  if(spacePlayer1.active) {
    if(buttons[2].pressed && spacePlayer1.x > 0) {
      spacePlayer1.x -= 2;
    }
    if(buttons[3].pressed && spacePlayer1.x < 116) {
      spacePlayer1.x += 2;
    }
    if(buttons[4].justPressed) {
      spawnPlayerBullet(1);
    }
  }
  
  // Controles do Player 2 (UP/DOWN para movimento horizontal)
  if(spaceTwoPlayer && spacePlayer2.active) {
    if(buttons[0].pressed && spacePlayer2.x > 0) {
      spacePlayer2.x -= 2;
    }
    if(buttons[1].pressed && spacePlayer2.x < 116) {
      spacePlayer2.x += 2;
    }
    if(buttons[5].justPressed) {  // BACK para atirar P2
      spawnPlayerBullet(2);
    }
  }
  
  // Movimento dos invasores
  if(millis() - spaceLastMove > spaceInvaderSpeed) {
    bool hitEdge = false;
    
    for(int i = 0; i < 55; i++) {
      if(invaders[i].active) {
        if((spaceInvaderDirection > 0 && invaders[i].x >= 118) ||
           (spaceInvaderDirection < 0 && invaders[i].x <= 0)) {
          hitEdge = true;
          break;
        }
      }
    }
    
    if(hitEdge) {
      spaceInvaderDirection *= -1;
      for(int i = 0; i < 55; i++) {
        if(invaders[i].active) {
          invaders[i].y += 4;
          if(invaders[i].y > 50) {
            spaceGameOver = true;
          }
        }
      }
    } else {
      for(int i = 0; i < 55; i++) {
        if(invaders[i].active) {
          invaders[i].x += spaceInvaderDirection;
          invaders[i].animFrame = (invaders[i].animFrame + 1) % 2;
        }
      }
    }
    
    spaceLastMove = millis();
  }
  
  // Atualizar balas
  for(int i = 0; i < 20; i++) {
    if(bullets[i].active) {
      bullets[i].y += bullets[i].vy;
      
      if(bullets[i].y < 0 || bullets[i].y > 64) {
        bullets[i].active = false;
        continue;
      }
      
      // Colisão com invasores (balas do player)
      if(bullets[i].isPlayer) {
        for(int j = 0; j < 55; j++) {
          if(invaders[j].active && 
             bullets[i].x >= invaders[j].x && bullets[i].x <= invaders[j].x + 8 &&
             bullets[i].y >= invaders[j].y && bullets[i].y <= invaders[j].y + 6) {
            invaders[j].active = false;
            bullets[i].active = false;
            
            int points = (invaders[j].type == 2) ? 30 : ((invaders[j].type == 1) ? 20 : 10);
            if(spaceTwoPlayer) {
              if(bullets[i].x < 64) spaceScore1 += points;
              else spaceScore2 += points;
            } else {
              spaceScore1 += points;
            }
            break;
          }
        }
      }
      // Colisão com players (balas inimigas)
      else {
        if(spacePlayer1.active &&
           bullets[i].x >= spacePlayer1.x && bullets[i].x <= spacePlayer1.x + 12 &&
           bullets[i].y >= spacePlayer1.y && bullets[i].y <= spacePlayer1.y + 4) {
          bullets[i].active = false;
          spacePlayer1.lives--;
          if(spacePlayer1.lives <= 0) {
            spacePlayer1.active = false;
          }
        }
        
        if(spaceTwoPlayer && spacePlayer2.active &&
           bullets[i].x >= spacePlayer2.x && bullets[i].x <= spacePlayer2.x + 12 &&
           bullets[i].y >= spacePlayer2.y && bullets[i].y <= spacePlayer2.y + 4) {
          bullets[i].active = false;
          spacePlayer2.lives--;
          if(spacePlayer2.lives <= 0) {
            spacePlayer2.active = false;
          }
        }
      }
      
      // Colisão com barreiras
      for(int b = 0; b < 4; b++) {
        int relX = (int)bullets[i].x - barriers[b].x;
        int relY = (int)bullets[i].y - barriers[b].y;
        
        if(relX >= 0 && relX < 16 && relY >= 0 && relY < 8) {
          if(barriers[b].pixels[relX][relY]) {
            barriers[b].pixels[relX][relY] = false;
            // Destruir pixels adjacentes
            if(relX > 0) barriers[b].pixels[relX-1][relY] = false;
            if(relX < 15) barriers[b].pixels[relX+1][relY] = false;
            if(relY > 0) barriers[b].pixels[relX][relY-1] = false;
            if(relY < 7) barriers[b].pixels[relX][relY+1] = false;
            
            bullets[i].active = false;
            break;
          }
        }
      }
    }
  }
  
  // Spawn balas inimigas
  if(millis() - spaceLastShoot > 300) {
    spawnEnemyBullet();
    spaceLastShoot = millis();
  }
  
  // Verificar vitória/derrota
  bool allDead = true;
  for(int i = 0; i < 55; i++) {
    if(invaders[i].active) {
      allDead = false;
      break;
    }
  }
  
  if(allDead) {
    spaceWave++;
    spaceInvaderSpeed = max(200, spaceInvaderSpeed - 100);
    initSpaceInvaders();
    return;
  }
  
  if((!spaceTwoPlayer && !spacePlayer1.active) || 
     (spaceTwoPlayer && !spacePlayer1.active && !spacePlayer2.active)) {
    spaceGameOver = true;
  }
}

void drawSpaceInvader(int x, int y, int type, int frame) {
  switch(type) {
    case 0: // Invasor tipo 1
      if(frame == 0) {
        display.drawPixel(x+2, y, SSD1306_WHITE);
        display.drawPixel(x+5, y, SSD1306_WHITE);
        display.drawLine(x+1, y+1, x+6, y+1, SSD1306_WHITE);
        display.drawLine(x, y+2, x+7, y+2, SSD1306_WHITE);
        display.drawLine(x, y+3, x+2, y+3, SSD1306_WHITE);
        display.drawLine(x+5, y+3, x+7, y+3, SSD1306_WHITE);
      } else {
        display.drawPixel(x+2, y, SSD1306_WHITE);
        display.drawPixel(x+5, y, SSD1306_WHITE);
        display.drawLine(x+1, y+1, x+6, y+1, SSD1306_WHITE);
        display.drawLine(x, y+2, x+7, y+2, SSD1306_WHITE);
        display.drawPixel(x+1, y+3, SSD1306_WHITE);
        display.drawPixel(x+6, y+3, SSD1306_WHITE);
      }
      break;
    case 1: // Invasor tipo 2
      display.drawPixel(x+3, y, SSD1306_WHITE);
      display.drawLine(x+2, y+1, x+4, y+1, SSD1306_WHITE);
      display.drawLine(x+1, y+2, x+5, y+2, SSD1306_WHITE);
      display.drawLine(x, y+3, x+6, y+3, SSD1306_WHITE);
      display.drawLine(x, y+4, x+6, y+4, SSD1306_WHITE);
      break;
    case 2: // UFO
      display.drawLine(x+1, y, x+6, y, SSD1306_WHITE);
      display.drawLine(x, y+1, x+7, y+1, SSD1306_WHITE);
      display.drawPixel(x+1, y+2, SSD1306_WHITE);
      display.drawPixel(x+3, y+2, SSD1306_WHITE);
      display.drawPixel(x+4, y+2, SSD1306_WHITE);
      display.drawPixel(x+6, y+2, SSD1306_WHITE);
      break;
  }
}

void drawSpacePlayer(int x, int y, bool active) {
  if(!active) return;
  
  display.drawPixel(x+6, y, SSD1306_WHITE);
  display.drawLine(x+4, y+1, x+8, y+1, SSD1306_WHITE);
  display.drawLine(x+2, y+2, x+10, y+2, SSD1306_WHITE);
  display.drawLine(x, y+3, x+12, y+3, SSD1306_WHITE);
}

void drawSpaceInvaders() {
  display.clearDisplay();
  
  // Desenhar invasores
  for(int i = 0; i < 55; i++) {
    if(invaders[i].active) {
      drawSpaceInvader(invaders[i].x, invaders[i].y, 
                      invaders[i].type, invaders[i].animFrame);
    }
  }
  
  // Desenhar barreiras
  for(int i = 0; i < 4; i++) {
    for(int y = 0; y < 8; y++) {
      for(int x = 0; x < 16; x++) {
        if(barriers[i].pixels[x][y]) {
          display.drawPixel(barriers[i].x + x, barriers[i].y + y, SSD1306_WHITE);
        }
      }
    }
  }
  
  // Desenhar jogadores
  drawSpacePlayer(spacePlayer1.x, spacePlayer1.y, spacePlayer1.active);
  if(spaceTwoPlayer) {
    drawSpacePlayer(spacePlayer2.x, spacePlayer2.y, spacePlayer2.active);
  }
  
  // Desenhar balas
  for(int i = 0; i < 20; i++) {
    if(bullets[i].active) {
      if(bullets[i].isPlayer) {
        display.drawLine((int)bullets[i].x, (int)bullets[i].y, 
                        (int)bullets[i].x, (int)bullets[i].y + 2, SSD1306_WHITE);
      } else {
        display.drawPixel((int)bullets[i].x, (int)bullets[i].y, SSD1306_WHITE);
        display.drawPixel((int)bullets[i].x, (int)bullets[i].y + 1, SSD1306_WHITE);
      }
    }
  }
  
  // HUD
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("P1:");
  display.print(spaceScore1);
  display.print(" L:");
  display.print(spacePlayer1.lives);
  
  if(spaceTwoPlayer) {
    display.setCursor(70, 0);
    display.print("P2:");
    display.print(spaceScore2);
    display.print(" L:");
    display.print(spacePlayer2.lives);
  } else {
    display.setCursor(85, 0);
    display.print("W:");
    display.print(spaceWave);
  }
  
  display.setCursor(100, 8);
  display.print(spaceTwoPlayer ? "2P" : "1P");
  
  if(spaceGameOver) {
    display.fillRect(25, 20, 80, 25, SSD1306_BLACK);
    display.drawRect(25, 20, 80, 25, SSD1306_WHITE);
    display.setCursor(35, 28);
    display.print("GAME OVER");
    display.setCursor(40, 38);
    if(spaceTwoPlayer) {
      display.print(spaceScore1 > spaceScore2 ? "P1 WINS" : 
                   spaceScore2 > spaceScore1 ? "P2 WINS" : "TIE");
    } else {
      display.print("WAVE ");
      display.print(spaceWave);
    }
  }
  
  display.display();
}

// ==================== BREAKOUT ====================
struct BreakoutBrick {
  int x, y;
  bool active;
  int color;
};

struct BreakoutBall {
  float x, y;
  float vx, vy;
  int size;
};

struct BreakoutPaddle {
  int x, y;
  int width, height;
};

BreakoutBrick bricks[80];  // 10x8 grid
BreakoutBall breakoutBall;
BreakoutPaddle paddle;
int breakoutScore = 0;
int breakoutLives = 3;
int breakoutLevel = 1;

void initBreakout() {
  breakoutScore = 0;
  breakoutLives = 3;
  breakoutGameOver = false;
  breakoutLevel = 1;
  
  // Inicializar paddle
  paddle.x = 50;
  paddle.y = 58;
  paddle.width = 20;
  paddle.height = 3;
  
  // Inicializar ball
  breakoutBall.x = 64;
  breakoutBall.y = 50;
  breakoutBall.vx = random(2) ? 1.5 : -1.5;
  breakoutBall.vy = -2;
  breakoutBall.size = 2;
  
  // Inicializar bricks
  for(int row = 0; row < 8; row++) {
    for(int col = 0; col < 10; col++) {
      int index = row * 10 + col;
      bricks[index].x = col * 12 + 4;
      bricks[index].y = row * 4 + 8;
      bricks[index].active = true;
      bricks[index].color = row / 2;  // 4 cores diferentes
    }
  }
}

void updateBreakout() {
  if(breakoutGameOver) return;
  
  // Controle do paddle
  if(buttons[2].pressed && paddle.x > 0) {
    paddle.x -= 3;
  }
  if(buttons[3].pressed && paddle.x < 128 - paddle.width) {
    paddle.x += 3;
  }
  
  // Movimento da bola
  breakoutBall.x += breakoutBall.vx;
  breakoutBall.y += breakoutBall.vy;
  
  // Colisão com paredes
  if(breakoutBall.x <= 0 || breakoutBall.x >= 126) {
    breakoutBall.vx = -breakoutBall.vx;
  }
  if(breakoutBall.y <= 0) {
    breakoutBall.vy = -breakoutBall.vy;
  }
  
  // Colisão com paddle
  if(breakoutBall.y + breakoutBall.size >= paddle.y && 
     breakoutBall.y <= paddle.y + paddle.height &&
     breakoutBall.x + breakoutBall.size >= paddle.x && 
     breakoutBall.x <= paddle.x + paddle.width) {
    
    breakoutBall.vy = -abs(breakoutBall.vy);
    
    // Adicionar spin baseado na posição de impacto
    float hitPos = (breakoutBall.x - paddle.x) / (float)paddle.width;
    breakoutBall.vx = (hitPos - 0.5) * 4;
  }
  
  // Colisão com bricks
  for(int i = 0; i < 80; i++) {
    if(bricks[i].active &&
       breakoutBall.x + breakoutBall.size >= bricks[i].x && 
       breakoutBall.x <= bricks[i].x + 11 &&
       breakoutBall.y + breakoutBall.size >= bricks[i].y && 
       breakoutBall.y <= bricks[i].y + 3) {
      
      bricks[i].active = false;
      breakoutBall.vy = -breakoutBall.vy;
      breakoutScore += (4 - bricks[i].color) * 10;
      break;
    }
  }
  
  // Perder vida
  if(breakoutBall.y > 64) {
    breakoutLives--;
    if(breakoutLives <= 0) {
      breakoutGameOver = true;
    } else {
      breakoutBall.x = paddle.x + paddle.width/2;
      breakoutBall.y = paddle.y - 5;
      breakoutBall.vx = random(2) ? 1.5 : -1.5;
      breakoutBall.vy = -2;
    }
  }
  
  // Verificar vitória
  bool allBroken = true;
  for(int i = 0; i < 80; i++) {
    if(bricks[i].active) {
      allBroken = false;
      break;
    }
  }
  
  if(allBroken) {
    breakoutLevel++;
    initBreakout();
    breakoutBall.vx *= 1.1;
    breakoutBall.vy *= 1.1;
  }
}

void drawBreakout() {
  display.clearDisplay();
  
  // Desenhar bricks
  for(int i = 0; i < 80; i++) {
    if(bricks[i].active) {
      if(bricks[i].color == 0) {
        display.fillRect(bricks[i].x, bricks[i].y, 11, 3, SSD1306_WHITE);
      } else {
        display.drawRect(bricks[i].x, bricks[i].y, 11, 3, SSD1306_WHITE);
        if(bricks[i].color > 1) {
          display.drawPixel(bricks[i].x + 5, bricks[i].y + 1, SSD1306_WHITE);
        }
      }
    }
  }
  
  // Desenhar paddle
  display.fillRect(paddle.x, paddle.y, paddle.width, paddle.height, SSD1306_WHITE);
  
  // Desenhar ball
  display.fillRect((int)breakoutBall.x, (int)breakoutBall.y, 
                  breakoutBall.size, breakoutBall.size, SSD1306_WHITE);
  
  // HUD
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(breakoutScore);
  
  display.setCursor(90, 0);
  display.print("Lives: ");
  display.print(breakoutLives);
  
  if(breakoutGameOver) {
    display.fillRect(30, 25, 70, 20, SSD1306_BLACK);
    display.drawRect(30, 25, 70, 20, SSD1306_WHITE);
    display.setCursor(45, 32);
    display.print("GAME OVER");
  }
  
  display.display();
}

// ==================== FROGGER ====================
struct FroggerCar {
  int x, y;
  int speed;
  int width;
  bool active;
};

struct FroggerLog {
  int x, y;
  int speed;
  int width;
  bool active;
};

struct Frog {
  int x, y;
  bool onLog;
  int logSpeed;
};

FroggerCar cars[12];
FroggerLog logs[8];
Frog frog;
int froggerScore = 0;
int froggerLives = 3;
int froggerTime = 60;
unsigned long froggerLastSecond = 0;

void initFrogger() {
  froggerScore = 0;
  froggerLives = 3;
  froggerGameOver = false;
  froggerTime = 60;
  froggerLastSecond = millis();
  
  // Inicializar frog
  frog.x = 60;
  frog.y = 58;
  frog.onLog = false;
  
  // Inicializar cars
  for(int i = 0; i < 12; i++) {
    cars[i].x = random(-50, 200);
    cars[i].y = 42 + (i % 3) * 5;
    cars[i].speed = (i % 2 == 0) ? 2 : -2;
    cars[i].width = random(12, 20);
    cars[i].active = true;
  }
  
  // Inicializar logs
  for(int i = 0; i < 8; i++) {
    logs[i].x = random(-50, 200);
    logs[i].y = 15 + (i % 2) * 10;
    logs[i].speed = (i % 2 == 0) ? 1 : -1;
    logs[i].width = random(20, 35);
    logs[i].active = true;
  }
}

void updateFrogger() {
  if(froggerGameOver) return;
  
  // Timer
  if(millis() - froggerLastSecond > 1000) {
    froggerTime--;
    froggerLastSecond = millis();
    if(froggerTime <= 0) {
      froggerLives--;
      if(froggerLives <= 0) {
        froggerGameOver = true;
      } else {
        froggerTime = 60;
        frog.x = 60;
        frog.y = 58;
      }
    }
  }
  
  // Movimento do frog
  if(buttons[0].justPressed && frog.y > 5) {
    frog.y -= 5;
  }
  if(buttons[1].justPressed && frog.y < 58) {
    frog.y += 5;
  }
  if(buttons[2].justPressed && frog.x > 0) {
    frog.x -= 5;
  }
  if(buttons[3].justPressed && frog.x < 120) {
    frog.x += 5;
  }
  
  // Movimento dos carros
  for(int i = 0; i < 12; i++) {
    cars[i].x += cars[i].speed;
    
    if(cars[i].x < -30) cars[i].x = 160;
    if(cars[i].x > 160) cars[i].x = -30;
  }
  
  // Movimento dos logs
  frog.onLog = false;
  for(int i = 0; i < 8; i++) {
    logs[i].x += logs[i].speed;
    
    if(logs[i].x < -40) logs[i].x = 170;
    if(logs[i].x > 170) logs[i].x = -40;
    
    // Verificar se frog está no log
    if(frog.y >= logs[i].y && frog.y <= logs[i].y + 4 &&
       frog.x + 4 >= logs[i].x && frog.x <= logs[i].x + logs[i].width) {
      frog.onLog = true;
      frog.x += logs[i].speed;  // Mover com o log
    }
  }
  
  // Colisão com carros
  for(int i = 0; i < 12; i++) {
    if(frog.y >= cars[i].y && frog.y <= cars[i].y + 4 &&
       frog.x + 4 >= cars[i].x && frog.x <= cars[i].x + cars[i].width) {
      froggerLives--;
      if(froggerLives <= 0) {
        froggerGameOver = true;
      } else {
        frog.x = 60;
        frog.y = 58;
      }
      return;
    }
  }
  
  // Afogamento na água
  if(frog.y >= 10 && frog.y <= 30 && !frog.onLog) {
    froggerLives--;
    if(froggerLives <= 0) {
      froggerGameOver = true;
    } else {
      frog.x = 60;
      frog.y = 58;
    }
    return;
  }
  
  // Chegada ao topo
  if(frog.y <= 5) {
    froggerScore += 100 + froggerTime;
    frog.x = 60;
    frog.y = 58;
    froggerTime = 60;
  }
}

void drawFrogger() {
  display.clearDisplay();
  
  // Desenhar zona de água
  for(int y = 10; y <= 30; y += 2) {
    for(int x = 0; x < 128; x += 4) {
      display.drawPixel(x + (y % 4), y, SSD1306_WHITE);
    }
  }
  
  // Desenhar logs
  for(int i = 0; i < 8; i++) {
    if(logs[i].active) {
      display.fillRect(logs[i].x, logs[i].y, logs[i].width, 4, SSD1306_WHITE);
      // Detalhes do log
      for(int x = 0; x < logs[i].width; x += 6) {
        display.drawPixel(logs[i].x + x, logs[i].y + 1, SSD1306_BLACK);
      }
    }
  }
  
  // Desenhar estrada
  display.drawLine(0, 40, 128, 40, SSD1306_WHITE);
  display.drawLine(0, 57, 128, 57, SSD1306_WHITE);
  
  // Desenhar carros
  for(int i = 0; i < 12; i++) {
    if(cars[i].active) {
      display.fillRect(cars[i].x, cars[i].y, cars[i].width, 4, SSD1306_WHITE);
      // Faróis
      if(cars[i].speed > 0) {
        display.drawPixel(cars[i].x + cars[i].width - 1, cars[i].y + 1, SSD1306_BLACK);
        display.drawPixel(cars[i].x + cars[i].width - 1, cars[i].y + 2, SSD1306_BLACK);
      } else {
        display.drawPixel(cars[i].x, cars[i].y + 1, SSD1306_BLACK);
        display.drawPixel(cars[i].x, cars[i].y + 2, SSD1306_BLACK);
      }
    }
  }
  
  // Desenhar frog
  display.fillRect(frog.x, frog.y, 4, 4, SSD1306_WHITE);
  display.drawPixel(frog.x + 1, frog.y + 1, SSD1306_BLACK);
  display.drawPixel(frog.x + 2, frog.y + 1, SSD1306_BLACK);
  
  // HUD
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score:");
  display.print(froggerScore);
  
  display.setCursor(70, 0);
  display.print("Lives:");
  display.print(froggerLives);
  
  display.setCursor(110, 0);
  display.print(froggerTime);
  
  if(froggerGameOver) {
    display.fillRect(30, 25, 70, 20, SSD1306_BLACK);
    display.drawRect(30, 25, 70, 20, SSD1306_WHITE);
    display.setCursor(45, 32);
    display.print("GAME OVER");
  }
  
  display.display();
}

// ==================== ASTEROIDS ====================
struct Asteroid {
  float x, y;
  float vx, vy;
  float rotation;
  int size;
  bool active;
};

struct AsteroidBullet {
  float x, y;
  float vx, vy;
  int life;
  bool active;
};

struct Ship {
  float x, y;
  float vx, vy;
  float angle;
  bool thrust;
  bool active;
};

Asteroid asteroids[20];
AsteroidBullet astBullets[10];
Ship ship;
int asteroidsScore = 0;
int asteroidsLives = 3;
int asteroidsLevel = 1;
unsigned long asteroidsLastThrust = 0;

void initAsteroids() {
  asteroidsScore = 0;
  asteroidsLives = 3;
  asteroidsGameOver = false;
  asteroidsLevel = 1;
  
  // Inicializar nave
  ship.x = 64;
  ship.y = 32;
  ship.vx = 0;
  ship.vy = 0;
  ship.angle = 0;
  ship.thrust = false;
  ship.active = true;
  
  // Limpar balas
  for(int i = 0; i < 10; i++) {
    astBullets[i].active = false;
  }
  
  // Inicializar asteroides
  for(int i = 0; i < 8; i++) {
    asteroids[i].x = random(128);
    asteroids[i].y = random(64);
    
    // Não spawnar muito perto da nave
    if(abs(asteroids[i].x - ship.x) < 20 && abs(asteroids[i].y - ship.y) < 20) {
      asteroids[i].x = (asteroids[i].x > 64) ? 120 : 8;
    }
    
    asteroids[i].vx = random(-20, 21) / 10.0;
    asteroids[i].vy = random(-20, 21) / 10.0;
    asteroids[i].rotation = random(360);
    asteroids[i].size = 3;
    asteroids[i].active = true;
  }
  
  // Desativar outros asteroides
  for(int i = 8; i < 20; i++) {
    asteroids[i].active = false;
  }
}

void spawnAsteroidBullet() {
  for(int i = 0; i < 10; i++) {
    if(!astBullets[i].active) {
      float radians = ship.angle * PI / 180.0;
      
      astBullets[i].x = ship.x + cos(radians) * 8;
      astBullets[i].y = ship.y + sin(radians) * 8;
      astBullets[i].vx = cos(radians) * 4;
      astBullets[i].vy = sin(radians) * 4;
      astBullets[i].life = 60;
      astBullets[i].active = true;
      break;
    }
  }
}

void splitAsteroid(int index) {
  if(asteroids[index].size <= 1) return;
  
  // Encontrar slots vazios para os fragmentos
  for(int i = 0; i < 20; i++) {
    if(!asteroids[i].active) {
      asteroids[i].x = asteroids[index].x;
      asteroids[i].y = asteroids[index].y;
      asteroids[i].vx = asteroids[index].vx + random(-10, 11) / 5.0;
      asteroids[i].vy = asteroids[index].vy + random(-10, 11) / 5.0;
      asteroids[i].size = asteroids[index].size - 1;
      asteroids[i].rotation = random(360);
      asteroids[i].active = true;
      
      // Segundo fragmento
      for(int j = i + 1; j < 20; j++) {
        if(!asteroids[j].active) {
          asteroids[j].x = asteroids[index].x;
          asteroids[j].y = asteroids[index].y;
          asteroids[j].vx = asteroids[index].vx - random(-10, 11) / 5.0;
          asteroids[j].vy = asteroids[index].vy - random(-10, 11) / 5.0;
          asteroids[j].size = asteroids[index].size - 1;
          asteroids[j].rotation = random(360);
          asteroids[j].active = true;
          break;
        }
      }
      break;
    }
  }
}

void updateAsteroids() {
  if(asteroidsGameOver) return;
  
  if(!ship.active) return;
  
  // Controles da nave
  if(buttons[0].pressed) {  // Thrust
    ship.thrust = true;
    if(millis() - asteroidsLastThrust > 100) {
      float radians = ship.angle * PI / 180.0;
      ship.vx += cos(radians) * 0.3;
      ship.vy += sin(radians) * 0.3;
      
      // Limitar velocidade
      float speed = sqrt(ship.vx * ship.vx + ship.vy * ship.vy);
      if(speed > 3) {
        ship.vx = (ship.vx / speed) * 3;
        ship.vy = (ship.vy / speed) * 3;
      }
      asteroidsLastThrust = millis();
    }
  } else {
    ship.thrust = false;
  }
  
  if(buttons[2].pressed) {  // Girar esquerda
    ship.angle -= 8;
    if(ship.angle < 0) ship.angle += 360;
  }
  if(buttons[3].pressed) {  // Girar direita
    ship.angle += 8;
    if(ship.angle >= 360) ship.angle -= 360;
  }
  if(buttons[4].justPressed) {  // Atirar
    spawnAsteroidBullet();
  }
  
  // Movimento da nave
  ship.x += ship.vx;
  ship.y += ship.vy;
  
  // Wrap around screen
  if(ship.x < 0) ship.x = 128;
  if(ship.x > 128) ship.x = 0;
  if(ship.y < 0) ship.y = 64;
  if(ship.y > 64) ship.y = 0;
  
  // Atrito
  ship.vx *= 0.98;
  ship.vy *= 0.98;
  
  // Movimento dos asteroides
  for(int i = 0; i < 20; i++) {
    if(asteroids[i].active) {
      asteroids[i].x += asteroids[i].vx;
      asteroids[i].y += asteroids[i].vy;
      asteroids[i].rotation += 2;
      
      // Wrap around
      if(asteroids[i].x < 0) asteroids[i].x = 128;
      if(asteroids[i].x > 128) asteroids[i].x = 0;
      if(asteroids[i].y < 0) asteroids[i].y = 64;
      if(asteroids[i].y > 64) asteroids[i].y = 0;
      
      // Colisão com nave
      float dx = asteroids[i].x - ship.x;
      float dy = asteroids[i].y - ship.y;
      float distance = sqrt(dx*dx + dy*dy);
      
      if(distance < asteroids[i].size * 4 + 3) {
        asteroidsLives--;
        if(asteroidsLives <= 0) {
          asteroidsGameOver = true;
          ship.active = false;
        } else {
          ship.x = 64;
          ship.y = 32;
          ship.vx = 0;
          ship.vy = 0;
        }
      }
    }
  }
  
  // Movimento das balas
  for(int i = 0; i < 10; i++) {
    if(astBullets[i].active) {
      astBullets[i].x += astBullets[i].vx;
      astBullets[i].y += astBullets[i].vy;
      astBullets[i].life--;
      
      // Wrap around
      if(astBullets[i].x < 0) astBullets[i].x = 128;
      if(astBullets[i].x > 128) astBullets[i].x = 0;
      if(astBullets[i].y < 0) astBullets[i].y = 64;
      if(astBullets[i].y > 64) astBullets[i].y = 0;
      
      if(astBullets[i].life <= 0) {
        astBullets[i].active = false;
        continue;
      }
      
      // Colisão com asteroides
      for(int j = 0; j < 20; j++) {
        if(asteroids[j].active) {
          float dx = astBullets[i].x - asteroids[j].x;
          float dy = astBullets[i].y - asteroids[j].y;
          float distance = sqrt(dx*dx + dy*dy);
          
          if(distance < asteroids[j].size * 4) {
            asteroidsScore += (4 - asteroids[j].size) * 20;
            splitAsteroid(j);
            asteroids[j].active = false;
            astBullets[i].active = false;
            break;
          }
        }
      }
    }
  }
  
  // Verificar se todos os asteroides foram destruídos
  bool allDestroyed = true;
  for(int i = 0; i < 20; i++) {
    if(asteroids[i].active) {
      allDestroyed = false;
      break;
    }
  }
  
  if(allDestroyed) {
    asteroidsLevel++;
    initAsteroids();
    // Adicionar mais asteroides a cada level
    for(int i = 0; i < min(asteroidsLevel + 3, 12); i++) {
      if(!asteroids[i].active) {
        asteroids[i].x = random(128);
        asteroids[i].y = random(64);
        asteroids[i].vx = random(-20, 21) / 10.0;
        asteroids[i].vy = random(-20, 21) / 10.0;
        asteroids[i].size = 3;
        asteroids[i].active = true;
      }
    }
  }
}

void drawAsteroidShape(float x, float y, int size, float rotation) {
  float points[8][2] = {
    {size*4, 0}, {size*3, size*2}, {size*1, size*3}, {-size*1, size*3},
    {-size*4, size*1}, {-size*3, -size*2}, {-size*1, -size*4}, {size*2, -size*3}
  };
  
  float rad = rotation * PI / 180.0;
  float cosR = cos(rad);
  float sinR = sin(rad);
  
  for(int i = 0; i < 8; i++) {
    int next = (i + 1) % 8;
    
    float x1 = points[i][0] * cosR - points[i][1] * sinR + x;
    float y1 = points[i][0] * sinR + points[i][1] * cosR + y;
    float x2 = points[next][0] * cosR - points[next][1] * sinR + x;
    float y2 = points[next][0] * sinR + points[next][1] * cosR + y;
    
    display.drawLine((int)x1, (int)y1, (int)x2, (int)y2, SSD1306_WHITE);
  }
}

void drawSpaceship(float x, float y, float angle, bool thrust) {
  float rad = angle * PI / 180.0;
  float cosA = cos(rad);
  float sinA = sin(rad);
  
  // Pontos da nave
  float nose_x = x + cosA * 8;
  float nose_y = y + sinA * 8;
  float left_x = x + cos(rad - 2.5) * 6;
  float left_y = y + sin(rad - 2.5) * 6;
  float right_x = x + cos(rad + 2.5) * 6;
  float right_y = y + sin(rad + 2.5) * 6;
  float back_x = x - cosA * 4;
  float back_y = y - sinA * 4;
  
  // Desenhar nave
  display.drawLine((int)nose_x, (int)nose_y, (int)left_x, (int)left_y, SSD1306_WHITE);
  display.drawLine((int)nose_x, (int)nose_y, (int)right_x, (int)right_y, SSD1306_WHITE);
  display.drawLine((int)left_x, (int)left_y, (int)back_x, (int)back_y, SSD1306_WHITE);
  display.drawLine((int)right_x, (int)right_y, (int)back_x, (int)back_y, SSD1306_WHITE);
  
  // Thrust flame
  if(thrust) {
    float thrust_x = x - cosA * 10;
    float thrust_y = y - sinA * 10;
    display.drawLine((int)back_x, (int)back_y, (int)thrust_x, (int)thrust_y, SSD1306_WHITE);
  }
}

void drawAsteroids() {
  display.clearDisplay();
  
  // Desenhar asteroides
  for(int i = 0; i < 20; i++) {
    if(asteroids[i].active) {
      drawAsteroidShape(asteroids[i].x, asteroids[i].y, 
                       asteroids[i].size, asteroids[i].rotation);
    }
  }
  
  // Desenhar nave
  if(ship.active) {
    drawSpaceship(ship.x, ship.y, ship.angle, ship.thrust);
  }
  
  // Desenhar balas
  for(int i = 0; i < 10; i++) {
    if(astBullets[i].active) {
      display.drawPixel((int)astBullets[i].x, (int)astBullets[i].y, SSD1306_WHITE);
    }
  }
  
  // HUD
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score:");
  display.print(asteroidsScore);
  
  display.setCursor(90, 0);
  display.print("Lives:");
  display.print(asteroidsLives);
  
  if(asteroidsGameOver) {
    display.fillRect(25, 25, 80, 20, SSD1306_BLACK);
    display.drawRect(25, 25, 80, 20, SSD1306_WHITE);
    display.setCursor(45, 32);
    display.print("GAME OVER");
  }
  
  display.display();
}


void initCentipede() {
  centipedeScore = 0;
  centipedeLives = 3;
  centipedeGameOver = false;
  centipedeWave = 1;
  centipedeSpeed = 300;
  
  // Inicializar shooter
  centShooter.x = 60;
  centShooter.y = 58;
  
  // Limpar balas
  for(int i = 0; i < 8; i++) {
    centBullets[i].active = false;
  }
  
  // Criar centipede (12 segmentos)
  for(int i = 0; i < 12; i++) {
    segments[i].x = i * 8;
    segments[i].y = 0;
    segments[i].active = true;
    segments[i].direction = 1;
    segments[i].isHead = (i == 0);
  }
  
  // Desativar outros segmentos
  for(int i = 12; i < 30; i++) {
    segments[i].active = false;
  }
  
  // Gerar mushrooms aleatórios
  for(int i = 0; i < 25; i++) {
    mushrooms[i].x = random(0, 15) * 8;
    mushrooms[i].y = random(8, 48);
    mushrooms[i].hits = 0;
    mushrooms[i].active = true;
  }
  
  // Desativar outros mushrooms
  for(int i = 25; i < 40; i++) {
    mushrooms[i].active = false;
  }
  
  // Inicializar spider
  spider.x = 0;
  spider.y = 50;
  spider.vx = 1;
  spider.vy = 0;
  spider.active = true;
  spider.lastMove = millis();
}

void updateCentipede() {
  if(centipedeGameOver) return;
  
  // Controles do shooter
  if(buttons[2].pressed && centShooter.x > 0) {
    centShooter.x -= 2;
  }
  if(buttons[3].pressed && centShooter.x < 120) {
    centShooter.x += 2;
  }
  if(buttons[0].pressed && centShooter.y > 48) {
    centShooter.y -= 2;
  }
  if(buttons[1].pressed && centShooter.y < 58) {
    centShooter.y += 2;
  }
  if(buttons[4].justPressed) {
    // Spawn bullet
    for(int i = 0; i < 8; i++) {
      if(!centBullets[i].active) {
        centBullets[i].x = centShooter.x + 4;
        centBullets[i].y = centShooter.y;
        centBullets[i].vy = -4;
        centBullets[i].active = true;
        centBullets[i].isPlayer = true;
        break;
      }
    }
  }
  
  // Movimento da centipede
  if(millis() - centipedeLastMove > centipedeSpeed) {
    for(int i = 0; i < 30; i++) {
      if(segments[i].active) {
        bool hitMushroom = false;
        bool hitEdge = false;
        
        // Verificar colisão com mushrooms
        int nextX = segments[i].x + segments[i].direction * 8;
        for(int j = 0; j < 40; j++) {
          if(mushrooms[j].active && 
             nextX == mushrooms[j].x && segments[i].y == mushrooms[j].y) {
            hitMushroom = true;
            break;
          }
        }
        
        // Verificar borda
        if(nextX < 0 || nextX > 120) {
          hitEdge = true;
        }
        
        if(hitMushroom || hitEdge) {
          segments[i].direction *= -1;
          segments[i].y += 8;
          
          // Verificar se chegou ao fundo
          if(segments[i].y > 48) {
            centipedeLives--;
            if(centipedeLives <= 0) {
              centipedeGameOver = true;
            } else {
              initCentipede();
            }
            return;
          }
        } else {
          segments[i].x += segments[i].direction * 8;
        }
      }
    }
    centipedeLastMove = millis();
  }
  
  // Movimento da spider
  if(millis() - spider.lastMove > 200) {
    spider.x += spider.vx;
    spider.y += spider.vy;
    
    if(spider.x <= 0 || spider.x >= 120) {
      spider.vx *= -1;
      spider.vy = random(-1, 2);
    }
    
    if(spider.y < 45 || spider.y > 58) {
      spider.vy *= -1;
    }
    
    spider.lastMove = millis();
  }
  
  // Atualizar balas
  for(int i = 0; i < 8; i++) {
    if(centBullets[i].active) {
      centBullets[i].y += centBullets[i].vy;
      
      if(centBullets[i].y < 0) {
        centBullets[i].active = false;
        continue;
      }
      
      // Colisão com centipede
      for(int j = 0; j < 30; j++) {
        if(segments[j].active &&
           centBullets[i].x >= segments[j].x && centBullets[i].x <= segments[j].x + 6 &&
           centBullets[i].y >= segments[j].y && centBullets[i].y <= segments[j].y + 6) {
          
          segments[j].active = false;
          centBullets[i].active = false;
          centipedeScore += 100;
          
          // Criar mushroom onde o segmento morreu
          for(int k = 0; k < 40; k++) {
            if(!mushrooms[k].active) {
              mushrooms[k].x = segments[j].x;
              mushrooms[k].y = segments[j].y;
              mushrooms[k].hits = 0;
              mushrooms[k].active = true;
              break;
            }
          }
          break;
        }
      }
      
      // Colisão com mushrooms
      for(int j = 0; j < 40; j++) {
        if(mushrooms[j].active &&
           centBullets[i].x >= mushrooms[j].x && centBullets[i].x <= mushrooms[j].x + 6 &&
           centBullets[i].y >= mushrooms[j].y && centBullets[i].y <= mushrooms[j].y + 6) {
          
          mushrooms[j].hits++;
          centBullets[i].active = false;
          
          if(mushrooms[j].hits >= 4) {
            mushrooms[j].active = false;
            centipedeScore += 1;
          }
          break;
        }
      }
      
      // Colisão com spider
      if(spider.active &&
         centBullets[i].x >= spider.x && centBullets[i].x <= spider.x + 8 &&
         centBullets[i].y >= spider.y && centBullets[i].y <= spider.y + 6) {
        
        spider.active = false;
        centBullets[i].active = false;
        centipedeScore += 600;
        
        // Respawn spider depois de um tempo
        spider.x = random(2) ? 0 : 120;
        spider.y = random(45, 59);
        spider.vx = (spider.x == 0) ? 1 : -1;
        spider.active = true;
      }
    }
  }
  
  // Colisão shooter com centipede
  for(int i = 0; i < 30; i++) {
    if(segments[i].active &&
       centShooter.x + 8 >= segments[i].x && centShooter.x <= segments[i].x + 6 &&
       centShooter.y + 6 >= segments[i].y && centShooter.y <= segments[i].y + 6) {
      centipedeLives--;
      if(centipedeLives <= 0) {
        centipedeGameOver = true;
      } else {
        initCentipede();
      }
      return;
    }
  }
  
  // Verificar vitória
  bool allDestroyed = true;
  for(int i = 0; i < 30; i++) {
    if(segments[i].active) {
      allDestroyed = false;
      break;
    }
  }
  
  if(allDestroyed) {
    centipedeWave++;
    centipedeSpeed = max(150, centipedeSpeed - 30);
    initCentipede();
  }
}

void drawCentipede() {
  display.clearDisplay();
  
  // Desenhar mushrooms
  for(int i = 0; i < 40; i++) {
    if(mushrooms[i].active) {
      if(mushrooms[i].hits == 0) {
        display.fillRect(mushrooms[i].x, mushrooms[i].y, 6, 6, SSD1306_WHITE);
        display.drawPixel(mushrooms[i].x + 2, mushrooms[i].y + 2, SSD1306_BLACK);
        display.drawPixel(mushrooms[i].x + 4, mushrooms[i].y + 4, SSD1306_BLACK);
      } else {
        display.drawRect(mushrooms[i].x, mushrooms[i].y, 6, 6, SSD1306_WHITE);
        for(int h = 0; h < mushrooms[i].hits; h++) {
          display.drawPixel(mushrooms[i].x + 1 + h, mushrooms[i].y + 1, SSD1306_WHITE);
        }
      }
    }
  }
  
  // Desenhar centipede
  for(int i = 0; i < 30; i++) {
    if(segments[i].active) {
      if(segments[i].isHead) {
        display.fillRect(segments[i].x, segments[i].y, 8, 6, SSD1306_WHITE);
        display.drawPixel(segments[i].x + 1, segments[i].y + 1, SSD1306_BLACK);
        display.drawPixel(segments[i].x + 5, segments[i].y + 1, SSD1306_BLACK);
      } else {
        display.drawRect(segments[i].x, segments[i].y, 8, 6, SSD1306_WHITE);
        display.fillRect(segments[i].x + 2, segments[i].y + 2, 4, 2, SSD1306_WHITE);
      }
    }
  }
  
  // Desenhar spider
  if(spider.active) {
    display.drawRect(spider.x, spider.y, 8, 6, SSD1306_WHITE);
    display.drawPixel(spider.x - 1, spider.y + 2, SSD1306_WHITE);
    display.drawPixel(spider.x + 8, spider.y + 2, SSD1306_WHITE);
    display.drawPixel(spider.x - 1, spider.y + 4, SSD1306_WHITE);
    display.drawPixel(spider.x + 8, spider.y + 4, SSD1306_WHITE);
  }
  
  // Desenhar shooter
  display.fillRect(centShooter.x, centShooter.y, 8, 6, SSD1306_WHITE);
  display.drawPixel(centShooter.x + 3, centShooter.y - 1, SSD1306_WHITE);
  display.drawPixel(centShooter.x + 4, centShooter.y - 1, SSD1306_WHITE);
  
  // Desenhar balas
  for(int i = 0; i < 8; i++) {
    if(centBullets[i].active) {
      display.drawLine((int)centBullets[i].x, (int)centBullets[i].y,
                      (int)centBullets[i].x, (int)centBullets[i].y + 2, SSD1306_WHITE);
    }
  }
  
  // Linha de separação da área do player
  display.drawLine(0, 47, 128, 47, SSD1306_WHITE);
  
  // HUD
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score:");
  display.print(centipedeScore);
  
  display.setCursor(90, 0);
  display.print("Lives:");
  display.print(centipedeLives);
  
  if(centipedeGameOver) {
    display.fillRect(25, 25, 80, 20, SSD1306_BLACK);
    display.drawRect(25, 25, 80, 20, SSD1306_WHITE);
    display.setCursor(45, 32);
    display.print("GAME OVER");
  }
  
  display.display();
}