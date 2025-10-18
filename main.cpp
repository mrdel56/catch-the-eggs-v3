// catch_the_eggs.cpp
// Menu -> Game playable "Catch the Eggs" prototype
// MODIFIED based on user requests (Airflow, New Perks/Damage, Help Menu)
// This version preserves the user's original code structure.
//
// Requirements: stb_image.h in same folder; menu_background.png and game_background.png accessible.
// Compile (MinGW/Windows):
//   g++ catch_the_eggs.cpp -o catch_the_eggs.exe -lopengl32 -lglu32 -lfreeglut -lwinmm
//
// To add sound, you must link an audio library. On Windows, -lwinmm for PlaySound.
// You also need sound files: sounds/collect_egg.wav, sounds/collect_gem.wav, sounds/poop.wav, sounds/perk.wav

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <ctime>
#include <fstream>      // Added for high score
#include <iostream>     // Added for high score
#include <sstream>      // Added for text formatting
#include <algorithm>    // For std::min/max

#include <GL/glut.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WIN32
  #include <direct.h>
  #define GetCurrentDir _getcwd
  #include <Windows.h> // For PlaySound
  #include <mmsystem.h> // <--- FIX 1: Added this header for sound functions
  #pragma comment(lib, "winmm.lib") // Link winmm.lib
#else
  #include <unistd.h>
  #define GetCurrentDir getcwd
#endif

// -----------------------------------------------------------------------------
// Configuration / window
const int WIN_W = 1024;
const int WIN_H = 768;

// Provide absolute paths here if you like; if left empty we'll use FALLBACK_* names in working dir
const char* IMG_PATH = "";         // menu image optional absolute path
const char* GAME_BG_PATH = "";     // game image optional absolute path

const char* FALLBACK_MENU = "C:/Users/MR_DEL/Documents/catch-the-eggs-v3/menu_background.png";
const char* FALLBACK_GAME = "C:/Users/MR_DEL/Documents/catch-the-eggs-v3/game_background.png";

const char* HIGHSCORE_FILE = "catch_eggs_highscore.txt";

// -----------------------------------------------------------------------------
// Textures
GLuint texMenu = 0;
GLuint texGame = 0;

// Scenes
enum Scene { SCENE_MENU, SCENE_GAME, SCENE_HELP }; // <-- NEW: Added Help Scene
Scene currentScene = SCENE_MENU;

// Menu click zones (tuned to your background)
struct ClickZone { float x1,y1,x2,y2; const char* name; };
ClickZone startZone = {770.0f, 440.0f, 970.0f, 520.0f, "Start"};
ClickZone helpZone  = {770.0f, 340.0f, 970.0f, 420.0f, "Help"}; // <-- NEW: Help Button
ClickZone quitZone  = {770.0f, 240.0f, 970.0f, 320.0f, "Quit"}; // <-- MODIFIED: Moved down
// In-game UI buttons
ClickZone pauseZone = {WIN_W - 280.0f, WIN_H - 40.0f, WIN_W - 190.0f, WIN_H - 12.0f, "Pause"};
ClickZone menuZone  = {WIN_W - 100.0f, WIN_H - 40.0f, WIN_W - 20.0f, WIN_H - 12.0f, "Menu"};
// Help screen button
ClickZone backZone  = {WIN_W/2.0f - 100.0f, 100.0f, WIN_W/2.0f + 100.0f, 150.0f, "Back"}; // <-- NEW: Back button

// -----------------------------------------------------------------------------
// Game data: eggs, perks, basket, chicken
enum ItemType { ITEM_EGG_NORMAL, ITEM_EGG_BLUE, ITEM_EGG_GOLD, ITEM_POOP, ITEM_EGG_ROTTEN, ITEM_PERK, ITEM_PERK_MULTIPLIER }; // <-- NEW: Rotten Egg and Multiplier
enum PerkType { PERK_NONE, PERK_BIG_BASKET, PERK_SLOW_FALL, PERK_EXTRA_TIME, PERK_SCORE_MULTIPLIER, PERK_SMALL_BASKET }; // <-- NEW: Multiplier and Small Basket

struct Item {
    ItemType type;
    float x, y;
    float vx; // <-- NEW: Added horizontal velocity for wind
    float vy;
    bool active;
    PerkType perk; // only used if type == ITEM_PERK or similar
};

std::vector<Item> items;

// basket
float basketX = WIN_W/2.0f;
float basketY = 90.0f;
float basketBaseHalfW = 60.0f; // half width
float basketHalfW = basketBaseHalfW; // may change by perks

// chicken on wire
struct Chicken {
    float x;
    float speed;
    int dir; // 1 = right, -1 = left
    float wireY; // Y position for this chicken's wire
};
std::vector<Chicken> chickens;

// Y positions for the wires/chickens
const float CHICKEN_WIRE_Y_1 = WIN_H - 160.0f;
const float CHICKEN_WIRE_Y_2 = WIN_H - 220.0f; // Second wire, lower
const float CHICKEN_WIRE_Y_3 = WIN_H - 280.0f; // Third wire, even lower

// game rules
int scoreVal = 0;
int scoreMultiplier = 1; // <-- NEW: Score multiplier
int highScore = 0; // Added for high score
int gameTime = 60; // seconds remaining
bool gameRunning = false;
bool gamePaused = false;

// spawn timers
float spawnAccumulator = 0.0f;      // seconds
float spawnInterval = 1.2f;        // seconds (modified by slow perk)

// fall speed
float baseFallSpeed = 180.0f; // px/sec
float fallSpeedMultiplier = 1.0f;

// perk timers
float perk_big_basket_timer = 0.0f;
float perk_small_basket_timer = 0.0f; // <-- NEW: Small basket timer
float perk_slow_fall_timer = 0.0f;
float perk_multiplier_timer = 0.0f; // <-- NEW: Multiplier timer

// --- NEW: Airflow/Wind ---
float windSpeed = 0.0f;     // Current wind speed (positive=right, negative=left)
float windTimer = 5.0f;     // Time until next wind change
struct WindLine { float x, y; };
std::vector<WindLine> windLines;

// -----------------------------------------------------------------------------
// Utility: High Score IO
void loadHighScore() {
    std::ifstream file(HIGHSCORE_FILE);
    if (file.is_open()) {
        file >> highScore;
        file.close();
    } else {
        std::cout << "Could not read high score file. Defaulting to 0." << std::endl;
        highScore = 0;
    }
}

void saveHighScore(int newScore) {
    if (newScore > highScore) {
        highScore = newScore;
        std::ofstream file(HIGHSCORE_FILE);
        if (file.is_open()) {
            file << highScore;
            file.close();
        } else {
            std::cerr << "Error: Could not save high score to file!" << std::endl;
        }
    }
}

// -----------------------------------------------------------------------------
// Utility: Sound
void playSound(const char* soundId) {
#ifdef _WIN32
    std::string path = "sounds/";
    path += soundId;
    path += ".wav";
    // Use SND_FILENAME and SND_ASYNC (defined in mmsystem.h)
    PlaySound(path.c_str(), NULL, SND_FILENAME | SND_ASYNC);
#else
    // Placeholder for other OS. You'd integrate an audio library like OpenAL/SDL_mixer here.
    // printf("DEBUG: PlaySound(%s)\n", soundId);
#endif
}

// -----------------------------------------------------------------------------
// Utility: load texture file (stb_image)
GLuint loadTextureFromFile(const char* path) {
    int w=0,h=0,comp=0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &comp, STBI_rgb_alpha);
    if (!data) {
        fprintf(stderr, "stbi_load failed for '%s' -> %s\n", path, stbi_failure_reason());
        return 0;
    }
    GLuint tid = 0;
    glGenTextures(1,&tid);
    glBindTexture(GL_TEXTURE_2D, tid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tid;
}

GLuint tryLoadTexture(const char* configuredPath, const char* fallbackName) {
    if (configuredPath && configuredPath[0] != '\0') {
        GLuint t = loadTextureFromFile(configuredPath);
        if (t) return t;
    }
    char cwd[1024];
    if (GetCurrentDir(cwd, sizeof(cwd))) printf("CWD: %s\n", cwd);
    GLuint t = loadTextureFromFile(fallbackName);
    if (t) return t;
    // try parent
    if (GetCurrentDir(cwd, sizeof(cwd))) {
        char p[1100]; snprintf(p, sizeof(p), "%s/../%s", cwd, fallbackName);
        t = loadTextureFromFile(p);
        if (t) return t;
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Drawing helpers
inline float invy(int y) { return (float)(WIN_H - y); }

void drawFullScreenTexture(GLuint t) {
    if (!t) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, t);
    glColor3f(1,1,1);
    glBegin(GL_QUADS);
      glTexCoord2f(0,0); glVertex2f(0,0);
      glTexCoord2f(1,0); glVertex2f(WIN_W,0);
      glTexCoord2f(1,1); glVertex2f(WIN_W,WIN_H);
      glTexCoord2f(0,1); glVertex2f(0,WIN_H);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void drawText(const char* s, float x, float y, void* font = GLUT_BITMAP_HELVETICA_18, float r=0, float g=0, float b=0) {
    glColor3f(r,g,b);
    glRasterPos2f(x,y);
    for (const char* p=s; *p; ++p) glutBitmapCharacter(font, *p);
}

void drawChickens() {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricDrawStyle(quad, GLU_FILL);

    for (const auto& ch : chickens) {
        float x = ch.x;
        float y = ch.wireY; // Use individual chicken's wire Y
        int dir = ch.dir;   // 1 for right, -1 for left

        // Base color for chicken body/head
        float bodyR = 1.0f, bodyG = 0.85f, bodyB = 0.2f; // Bright yellow-orange

        // --- Body ---
        glPushMatrix();
        glTranslatef(x, y - 10, 0); // Adjust Y slightly for better head placement
        glScalef(1.2f, 1.0f, 1.0f); // Make body a wider oval

        // Body main fill
        glColor3f(bodyR, bodyG, bodyB);
        gluDisk(quad, 0, 35, 32, 1);

        // Body outline
        glColor3f(0.3f, 0.2f, 0.05f); // Dark brown/orange outline
        glLineWidth(3.0f);
        gluDisk(quad, 34, 35, 32, 1);
        glLineWidth(1.0f);
        glPopMatrix();

        // --- Head ---
        float headOffsetX = dir * 25.0f; // More pronounced head offset
        float headOffsetY = 30.0f;

        glPushMatrix();
        glTranslatef(x + headOffsetX, y + headOffsetY, 0);

        // Head main fill
        glColor3f(bodyR, bodyG, bodyB);
        gluDisk(quad, 0, 20, 32, 1);

        // Head outline
        glColor3f(0.3f, 0.2f, 0.05f); // Dark brown/orange outline
        glLineWidth(3.0f);
        gluDisk(quad, 19, 20, 32, 1);
        glLineWidth(1.0f);
        glPopMatrix();

        // --- Comb (on head) ---
        glColor3f(0.9f, 0.1f, 0.1f); // Vibrant red comb
        glLineWidth(2.0f); // Thicker outline for comb
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(x + headOffsetX, y + headOffsetY + 20); // Top of head
            glVertex2f(x + headOffsetX + dir * 8, y + headOffsetY + 30);
            glVertex2f(x + headOffsetX + dir * 20, y + headOffsetY + 25);
            glVertex2f(x + headOffsetX + dir * 15, y + headOffsetY + 18);
        glEnd();
        // Comb outline
        glColor3f(0.5f, 0.05f, 0.05f); // Darker red outline
        glBegin(GL_LINE_LOOP);
            glVertex2f(x + headOffsetX, y + headOffsetY + 20);
            glVertex2f(x + headOffsetX + dir * 8, y + headOffsetY + 30);
            glVertex2f(x + headOffsetX + dir * 20, y + headOffsetY + 25);
            glVertex2f(x + headOffsetX + dir * 15, y + headOffsetY + 18);
        glEnd();
        glLineWidth(1.0f);

        // --- Beak ---
        glColor3f(1.0f, 0.7f, 0.1f); // Brighter orange beak
        glBegin(GL_TRIANGLES);
            glVertex2f(x + headOffsetX + dir * 15, y + headOffsetY + 3);
            glVertex2f(x + headOffsetX + dir * 30, y + headOffsetY);
            glVertex2f(x + headOffsetX + dir * 15, y + headOffsetY - 3);
        glEnd();
        // Beak outline
        glColor3f(0.7f, 0.4f, 0.0f); // Darker orange outline
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(x + headOffsetX + dir * 15, y + headOffsetY + 3);
            glVertex2f(x + headOffsetX + dir * 30, y + headOffsetY);
            glVertex2f(x + headOffsetX + dir * 15, y + headOffsetY - 3);
        glEnd();
        glLineWidth(1.0f);


        // --- Wattle (under chin) ---
        glColor3f(0.9f, 0.1f, 0.1f); // Vibrant red wattle
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(x + headOffsetX + dir * 10, y + headOffsetY - 8);
            glVertex2f(x + headOffsetX + dir * 5, y + headOffsetY - 18);
            glVertex2f(x + headOffsetX + dir * 15, y + headOffsetY - 18);
        glEnd();
        // Wattle outline
        glColor3f(0.5f, 0.05f, 0.05f); // Darker red outline
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(x + headOffsetX + dir * 10, y + headOffsetY - 8);
            glVertex2f(x + headOffsetX + dir * 5, y + headOffsetY - 18);
            glVertex2f(x + headOffsetX + dir * 15, y + headOffsetY - 18);
        glEnd();
        glLineWidth(1.0f);


        // --- Eyes ---
        // Eye White
        glColor3f(1.0f, 1.0f, 1.0f); // White
        glPushMatrix();
        glTranslatef(x + headOffsetX + dir * 10, y + headOffsetY + 10, 0);
        gluDisk(quad, 0, 8, 32, 1);
        glPopMatrix();
        // Eye Outline
        glColor3f(0.3f, 0.2f, 0.05f); // Dark outline
        glLineWidth(2.0f);
        glPushMatrix();
        glTranslatef(x + headOffsetX + dir * 10, y + headOffsetY + 10, 0);
        gluDisk(quad, 7, 8, 32, 1);
        glPopMatrix();
        glLineWidth(1.0f);

        // Pupil
        glColor3f(0.0f, 0.0f, 0.0f); // Black
        glPushMatrix();
        glTranslatef(x + headOffsetX + dir * 12, y + headOffsetY + 11, 0); // Offset pupil slightly
        gluDisk(quad, 0, 4, 16, 1);
        glPopMatrix();

        // Eye Highlight
        glColor3f(1.0f, 1.0f, 1.0f); // White highlight
        glPushMatrix();
        glTranslatef(x + headOffsetX + dir * 13, y + headOffsetY + 13, 0); // Position highlight
        gluDisk(quad, 0, 2, 8, 1);
        glPopMatrix();

        // --- Cheeks (blush) ---
        glColor4f(1.0f, 0.6f, 0.6f, 0.7f); // Pinkish blush, slightly transparent
        glPushMatrix();
        glTranslatef(x + headOffsetX + dir * 10, y + headOffsetY - 5, 0);
        glScalef(1.0f, 0.7f, 1.0f); // Oval blush
        gluDisk(quad, 0, 7, 16, 1);
        glPopMatrix();


        // --- Wings ---
        glColor3f(bodyR * 0.9f, bodyG * 0.9f, bodyB * 0.9f); // Slightly darker yellow-orange for wings
        glPushMatrix();
        glTranslatef(x + dir * 25, y - 5, 0);
        glScalef(1.0f, 0.7f, 1.0f); // Oval shape
        glRotatef(dir * 15, 0, 0, 1); // Slight rotation
        gluDisk(quad, 0, 20, 32, 1);
        glPopMatrix();
        // Wing outline
        glColor3f(0.3f, 0.2f, 0.05f); // Dark brown/orange outline
        glLineWidth(3.0f);
        glPushMatrix();
        glTranslatef(x + dir * 25, y - 5, 0);
        glScalef(1.0f, 0.7f, 1.0f);
        glRotatef(dir * 15, 0, 0, 1);
        gluDisk(quad, 19, 20, 32, 1);
        glPopMatrix();
        glLineWidth(1.0f);
    }
    gluDeleteQuadric(quad);
}


// draw "cartoon" basket
void drawBasket() {
    float hx = basketHalfW;
    float hy = 24.0f;
    float rimH = 8.0f;

    // Main basket (bucket part)
    glColor3f(0.5f, 0.3f, 0.05f); // Brown
    glBegin(GL_QUADS);
      glVertex2f(basketX - hx*0.8f, basketY - hy*0.5f);
      glVertex2f(basketX + hx*0.8f, basketY - hy*0.5f);
      glVertex2f(basketX + hx,      basketY + hy*0.5f);
      glVertex2f(basketX - hx,      basketY + hy*0.5f);
    glEnd();

    // Rim
    glColor3f(0.6f, 0.35f, 0.1f); // Lighter brown
    glBegin(GL_QUADS);
      glVertex2f(basketX - hx,      basketY + hy*0.5f);
      glVertex2f(basketX + hx,      basketY + hy*0.5f);
      glVertex2f(basketX + hx,      basketY + hy*0.5f + rimH);
      glVertex2f(basketX - hx,      basketY + hy*0.5f + rimH);
    glEnd();

    // Handle (simple arc)
    glColor3f(0.4f, 0.2f, 0.0f); // Darker brown
    glLineWidth(5.0f);
    glBegin(GL_LINE_STRIP);
      glVertex2f(basketX - hx*0.9f, basketY + hy*0.5f + rimH*0.5f);
      glVertex2f(basketX - hx*0.9f, basketY + hy*2.0f);
      glVertex2f(basketX + hx*0.9f, basketY + hy*2.0f);
      glVertex2f(basketX + hx*0.9f, basketY + hy*0.5f + rimH*0.5f);
    glEnd();
    glLineWidth(1.0f);
}

// draw items (eggs/poop/perk)
void drawItems() {
    for (auto &it : items) {
        if (!it.active) continue;

        float r = 0.0f, g = 0.0f, b = 0.0f;

        switch (it.type) {
          case ITEM_EGG_NORMAL: r=1.0f; g=1.0f; b=0.9f; break; // pale yellow
          case ITEM_EGG_BLUE:   r=0.4f; g=0.7f; b=1.0f; break;
          case ITEM_EGG_GOLD:   r=1.0f; g=0.86f; b=0.2f; break;
          case ITEM_POOP:       r=0.4f; g=0.2f; b=0.05f; break;
          case ITEM_EGG_ROTTEN: r=0.4f; g=0.5f; b=0.3f; break; // <-- NEW: Greenish
          case ITEM_PERK:       r=0.8f; g=0.4f; b=0.9f; break; // Pink for old perks
          case ITEM_PERK_MULTIPLIER: r=0.9f; g=0.1f; b=0.9f; break; // <-- NEW: Purple for multiplier
        }
        glColor3f(r,g,b);

        // draw oval for egg/poop, diamond for gems, star for perk
        if (it.type == ITEM_EGG_BLUE || it.type == ITEM_EGG_GOLD) {
            // Gem (diamond shape)
            float s = 12.0f;
            glBegin(GL_QUADS);
              glVertex2f(it.x, it.y-s);
              glVertex2f(it.x+s, it.y);
              glVertex2f(it.x, it.y+s);
              glVertex2f(it.x-s, it.y);
            glEnd();
            // Gem highlight
            glColor3f(r*1.2f+0.1f, g*1.2f+0.1f, b*1.2f+0.1f);
            glBegin(GL_TRIANGLES);
              glVertex2f(it.x, it.y+s);
              glVertex2f(it.x-s, it.y);
              glVertex2f(it.x-s*0.5f, it.y+s*0.5f);
            glEnd();

        } else if (it.type == ITEM_PERK || it.type == ITEM_PERK_MULTIPLIER) {
            // Star shape
            float r1 = 16.0f; float r2 = 7.0f;
            glBegin(GL_TRIANGLE_FAN);
              glVertex2f(it.x, it.y);
              for (int i=0; i<=10; i++) {
                  float r_outer = (i%2 == 0) ? r1 : r2;
                  float a = i * (2.0f * 3.1415926f / 10.0f) + (3.1415926f / 2.0f);
                  glVertex2f(it.x + cosf(a)*r_outer, it.y + sinf(a)*r_outer);
              }
            glEnd();
        } else {
            // Egg/Poop/Rotten (oval)
            float rX = (it.type == ITEM_POOP) ? 6.0f : 8.0f;
            float rY = (it.type == ITEM_POOP) ? 8.0f : 12.0f;
            glBegin(GL_TRIANGLE_FAN);
              glVertex2f(it.x, it.y);
              for (int i=0;i<=20;i++){
                float a = i * 2.0f * 3.1415926f / 20.0f;
                glVertex2f(it.x + cosf(a)*rX, it.y + sinf(a)*rY);
              }
            glEnd();
            // <-- NEW: Draw crack on rotten egg
            if (it.type == ITEM_EGG_ROTTEN) {
                glColor3f(0.1f, 0.2f, 0.1f); // Dark crack lines
                glBegin(GL_LINE_STRIP);
                  glVertex2f(it.x-4, it.y+6); glVertex2f(it.x, it.y+2);
                  glVertex2f(it.x+5, it.y+5);
                glEnd();
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Game logic helpers

// Helper to add a new chicken
void addChicken(float wireY) {
    Chicken ch;
    // Start new chickens at alternating sides
    if (chickens.size() % 2 == 0) { // First chicken, or even number of chickens
        ch.x = 220.0f;
        ch.dir = 1;
    } else { // Odd number of chickens
        ch.x = WIN_W - 220.0f;
        ch.dir = -1;
    }
    ch.speed = 120.0f + (rand() % 40) - 20; // slightly different speeds
    ch.wireY = wireY;
    chickens.push_back(ch);
}

void resetGame() {
    items.clear();
    chickens.clear(); // Clear chicken vector

    // Always start with one chicken
    addChicken(CHICKEN_WIRE_Y_1);

    basketX = WIN_W/2.0f;
    basketHalfW = basketBaseHalfW;
    scoreVal = 0;
    scoreMultiplier = 1; // <-- NEW: Reset multiplier
    gameTime = 60;
    spawnAccumulator = 0.0f;
    baseFallSpeed = 180.0f;
    fallSpeedMultiplier = 1.0f;
    perk_big_basket_timer = 0.0f;
    perk_small_basket_timer = 0.0f; // <-- NEW: Reset small basket timer
    perk_slow_fall_timer = 0.0f;
    perk_multiplier_timer = 0.0f; // <-- NEW: Reset multiplier timer
    windSpeed = 0.0f; // <-- NEW: Reset wind
    windTimer = 5.0f; // <-- NEW: Reset wind timer
    gameRunning = true;
    gamePaused = false;
}

// spawn an item at a random active chicken's X with random type/rarity
void spawnItem() {
    if (chickens.empty()) return; // safety check

    // Pick a random chicken to spawn from
    int chIndex = rand() % chickens.size();
    float spawnX = chickens[chIndex].x;
    float spawnY = chickens[chIndex].wireY;

    Item it;
    float r = (float)rand() / RAND_MAX; // Random number 0.0 to 1.0
    float chickenFactor = (float)chickens.size();

    // Cap scaling so it doesn't get too crazy
    float chickenScale = std::min(chickenFactor, 3.0f); // Max 3x chance for gems
    float perkScale = std::min(chickenFactor, 2.0f);    // Max 2x chance for perks

    float goldChance = 0.03f * chickenScale; // 3% base, scales up
    float blueChance = 0.08f * chickenScale; // 8% base, scales up
    float multiplierChance = 0.04f * perkScale; // <-- NEW: Multiplier chance
    float perkChance = 0.05f * perkScale;    // 5% base, scales up
    float rottenChance = 0.08f; // <-- NEW: Rotten egg chance
    float poopChance = 0.15f;

    // --- Create Cumulative Windows (Correct, non-buggy logic) ---
    float cumulativeChance = 0.0f;

    // Window 1: Gold
    if (r < (cumulativeChance += goldChance)) {
        it.type = ITEM_EGG_GOLD;
    }
    // Window 2: Blue
    else if (r < (cumulativeChance += blueChance)) {
        it.type = ITEM_EGG_BLUE;
    }
    // <-- NEW: Window 3: Multiplier Perk -->
    else if (r < (cumulativeChance += multiplierChance)) {
        it.type = ITEM_PERK_MULTIPLIER;
        it.perk = PERK_SCORE_MULTIPLIER;
    }
    // Window 4: Other Perks
    else if (r < (cumulativeChance += perkChance)) {
        it.type = ITEM_PERK;
        // pick a perk randomly (only big basket or slow fall)
        float p = (float)rand() / RAND_MAX;
        if (p < 0.5f) it.perk = PERK_BIG_BASKET;
        else it.perk = PERK_SLOW_FALL;
    }
    // <-- NEW: Window 5: Rotten Egg -->
    else if (r < (cumulativeChance += rottenChance)) {
        it.type = ITEM_EGG_ROTTEN;
    }
    // Window 6: Poop
    else if (r < (cumulativeChance += poopChance)) {
        it.type = ITEM_POOP;
    }
    // Window 7: Normal Egg (everything else)
    else {
        it.type = ITEM_EGG_NORMAL;
    }

    // --- Set item properties ---
    it.x = spawnX + ( (rand()%41) - 20 ); // small horizontal jitter
    it.y = spawnY - 18.0f;
    it.vx = 0.0f; // <-- NEW: Start with no horizontal speed
    it.vy = baseFallSpeed;
    it.active = true;
    items.push_back(it);
}

// apply perk
void applyPerk(Item &it) {
    if (it.perk == PERK_BIG_BASKET) {
        playSound("perk");
        basketHalfW = basketBaseHalfW * 1.7f;
        perk_big_basket_timer = 8.0f;
    } else if (it.perk == PERK_SLOW_FALL) {
        playSound("perk");
        fallSpeedMultiplier = 0.55f;
        perk_slow_fall_timer = 8.0f;
    }
    // <-- NEW: Multiplier Perk -->
    else if (it.perk == PERK_SCORE_MULTIPLIER) {
        playSound("perk");
        scoreMultiplier = 2;
        perk_multiplier_timer = 10.0f; // 10 seconds of 2x score
    }
    // <-- NEW: Small Basket (from rotten egg) -->
    else if (it.perk == PERK_SMALL_BASKET) {
        playSound("poop"); // Negative sound
        basketHalfW = basketBaseHalfW * 0.6f; // Shrink basket
        perk_small_basket_timer = 6.0f;
    }
}

// handle caught item or missed
void handleCatch(Item &it) {
    int points = 0;
    if (it.type == ITEM_EGG_NORMAL) { points = 1; playSound("collect_egg"); }
    else if (it.type == ITEM_EGG_BLUE) { points = 5; playSound("collect_gem"); }
    else if (it.type == ITEM_EGG_GOLD) { points = 10; playSound("collect_gem"); }
    else if (it.type == ITEM_POOP) { points = -10; playSound("poop"); }
    // <-- NEW: Handle Rotten Egg -->
    else if (it.type == ITEM_EGG_ROTTEN) {
        points = -5;
        it.perk = PERK_SMALL_BASKET; // Apply the small basket perk
        applyPerk(it);
    }
    // <-- MODIFIED: Handle both perk types -->
    else if (it.type == ITEM_PERK || it.type == ITEM_PERK_MULTIPLIER) {
        applyPerk(it);
    }

    // <-- MODIFIED: Apply multiplier to points -->
    scoreVal += (points * scoreMultiplier);
    // Ensure score doesn't go negative
    scoreVal = std::max(0, scoreVal);
}

// -----------------------------------------------------------------------------
// Update (dt in seconds)
void updateGame(float dt) {
    if (!gameRunning || gamePaused) return;

    // --- NEW: Wind Logic ---
    windTimer -= dt;
    if (windTimer <= 0.0f) {
        float r = (float)rand() / RAND_MAX;
        if (r < 0.3f) windSpeed = 0.0f; // 30% chance of no wind
        else if (r < 0.65f) windSpeed = 50.0f + (rand() % 50); // Wind to the right
        else windSpeed = -50.0f - (rand() % 50); // Wind to the left
        windTimer = 3.0f + (rand() % 4); // Next change in 3-6 seconds
    }
    // Animate wind lines
    for(auto& line : windLines) {
        line.x += windSpeed * dt * 0.5f; // Move lines at half wind speed
        if (line.x > WIN_W + 20) line.x = -20;
        if (line.x < -20) line.x = WIN_W + 20;
    }

    // --- Chicken Spawning/Despawning Logic ---
    int targetChickens = 1;
    if (scoreVal >= 100) targetChickens = 3;
    else if (scoreVal >= 50) targetChickens = 2;

    // Add chickens if needed
    if (chickens.size() < (size_t)targetChickens) {
        if (chickens.size() == 1) addChicken(CHICKEN_WIRE_Y_2);
        else if (chickens.size() == 2) addChicken(CHICKEN_WIRE_Y_3);
    }
    // Remove chickens if score drops
    while (chickens.size() > (size_t)targetChickens) {
        chickens.pop_back(); // Remove the last chicken
    }
    // --- End Chicken Spawning/Despawning ---


    // chicken movement (loop over all chickens)
    for (auto& ch : chickens) {
        ch.x += ch.dir * ch.speed * dt;
        if (ch.x > WIN_W - 160) { ch.x = WIN_W - 160; ch.dir = -1; }
        if (ch.x < 160)        { ch.x = 160;        ch.dir =  1; }
    }


    // spawn logic
    spawnAccumulator += dt;
    // Increase spawning frequency with more chickens
    float effectiveSpawnInterval = spawnInterval / (float)chickens.size(); // e.g., 2 chickens -> half interval
    if (spawnAccumulator >= effectiveSpawnInterval) {
        spawnItem();
        spawnAccumulator = 0.0f;
        // slightly randomize next spawn interval base
        spawnInterval = 0.8f + (rand()%100)/200.0f; // 0.8..1.3
    }

    // move items
    for (auto &it : items) {
        if (!it.active) continue;
        it.x += windSpeed * dt; // <-- NEW: Apply wind
        it.y -= it.vy * fallSpeedMultiplier * dt;
        // hit basket?
        if (it.y <= basketY + 30 && it.y >= basketY - 30) {
            // check X overlap
            if (it.x >= basketX - basketHalfW && it.x <= basketX + basketHalfW) {
                handleCatch(it);
                it.active = false;
            }
        }
        // missed bottom
        if (it.y < 20) {
            it.active = false;
            // missing an egg does not penalize here; you could subtract points
        }
    }

    // remove inactive items occasionally to keep vector small
    if (items.size() > 200) {
        std::vector<Item> tmp;
        tmp.reserve(items.size());
        for (auto &it : items) if (it.active) tmp.push_back(it);
        items.swap(tmp);
    }

    // update perks timers
    if (perk_big_basket_timer > 0.0f) {
        perk_big_basket_timer -= dt;
        if (perk_big_basket_timer <= 0.0f) { basketHalfW = basketBaseHalfW; perk_big_basket_timer = 0.0f; }
    }
    // <-- NEW: Small basket timer -->
    if (perk_small_basket_timer > 0.0f) {
        perk_small_basket_timer -= dt;
        if (perk_small_basket_timer <= 0.0f) { basketHalfW = basketBaseHalfW; perk_small_basket_timer = 0.0f; }
    }
    if (perk_slow_fall_timer > 0.0f) {
        perk_slow_fall_timer -= dt;
        if (perk_slow_fall_timer <= 0.0f) { fallSpeedMultiplier = 1.0f; perk_slow_fall_timer = 0.0f; }
    }
    // <-- NEW: Multiplier timer -->
    if (perk_multiplier_timer > 0.0f) {
        perk_multiplier_timer -= dt;
        if (perk_multiplier_timer <= 0.0f) { scoreMultiplier = 1; perk_multiplier_timer = 0.0f; }
    }
}

// -----------------------------------------------------------------------------
// Game loop via glutTimerFunc
const int TIMER_MS = 16; // ~60fps
void timer_cb(int) {
    static int last_ms = 0;
    int now_ms = glutGet(GLUT_ELAPSED_TIME);
    if (last_ms == 0) last_ms = now_ms;
    float dt = (now_ms - last_ms) / 1000.0f;
    if (dt > 0.06f) dt = 0.06f; // clamp
    if (dt <= 0.0f) dt = 0.016f; // prevent 0 dt
    last_ms = now_ms;

    if (currentScene == SCENE_GAME && gameRunning && !gamePaused) {
        // decrement game time every second (we accumulate)
        static float timeAcc = 0.0f;
        timeAcc += dt;
        if (timeAcc >= 1.0f) {
            gameTime -= 1;
            timeAcc -= 1.0f;
            if (gameTime <= 0) {
                gameTime = 0;
                gameRunning = false;
                gamePaused = false;
                // --- Save High Score ---
                saveHighScore(scoreVal);
            }
        }
        updateGame(dt);
    }

    glutPostRedisplay();
    glutTimerFunc(TIMER_MS, timer_cb, 0);
}

// -----------------------------------------------------------------------------
// Input: keyboard + mouse
void keyboard_cb(unsigned char key, int x, int y) {
    (void)x; (void)y;
    // <-- MODIFIED: Check for help scene -->
    if (currentScene == SCENE_MENU || currentScene == SCENE_HELP) {
        if (key == 27) {
            if (currentScene == SCENE_HELP) currentScene = SCENE_MENU;
            else exit(0);
        }
        if (currentScene == SCENE_MENU && (key == ' ' || key == 's' || key == 'S')) {
            // start game
            resetGame();
            currentScene = SCENE_GAME;
        }
    } else if (currentScene == SCENE_GAME) {
        if (key == 27) { // ESC -> back to menu
            currentScene = SCENE_MENU;
            gameRunning = false;
            gamePaused = false;
        } else if (key == 'p' || key == 'P') {
            if (gameRunning) gamePaused = !gamePaused; // Only pause if game is running
        } else if (key == 'r' || key == 'R') {
            if (gamePaused) gamePaused = false;
        } else if (key == 'a' || key == 'A' || key == 81 /*left*/) {
            basketX -= 24.0f;
            if (basketX - basketHalfW < 8) basketX = 8 + basketHalfW;
        } else if (key == 'd' || key == 'D' || key == 83 /*right*/) {
            basketX += 24.0f;
            if (basketX + basketHalfW > WIN_W - 8) basketX = WIN_W - 8 - basketHalfW;
        }
    }
}

void passiveMouse_cb(int mx, int my) {
    // mouse moves basket X only when in game
    if (currentScene == SCENE_GAME && !gamePaused && gameRunning) {
        float fx = (float)mx;
        // clamp so basket stays inside window
        if (fx - basketHalfW < 8) fx = 8 + basketHalfW;
        if (fx + basketHalfW > WIN_W - 8) fx = WIN_W - 8 - basketHalfW;
        basketX = fx;
    }
}

void mouseClick_cb(int button, int state, int mx, int my) {
    float fx = (float)mx;
    float fy = invy(my);

    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;

    if (currentScene == SCENE_MENU) {
        if (fx >= startZone.x1 && fx <= startZone.x2 && fy >= startZone.y1 && fy <= startZone.y2) {
            resetGame();
            currentScene = SCENE_GAME;
            return;
        }
        // <-- NEW: Check for help button -->
        if (fx >= helpZone.x1 && fx <= helpZone.x2 && fy >= helpZone.y1 && fy <= helpZone.y2) {
            currentScene = SCENE_HELP;
            return;
        }
        if (fx >= quitZone.x1 && fx <= quitZone.x2 && fy >= quitZone.y1 && fy <= quitZone.y2) {
            exit(0);
        }
    }
    // <-- NEW: Check for help scene clicks -->
    else if (currentScene == SCENE_HELP) {
        if (fx >= backZone.x1 && fx <= backZone.x2 && fy >= backZone.y1 && fy <= backZone.y2) {
            currentScene = SCENE_MENU;
            return;
        }
    }
    else if (currentScene == SCENE_GAME) {
        // Check UI buttons first
        if (fx >= pauseZone.x1 && fx <= pauseZone.x2 && fy >= pauseZone.y1 && fy <= pauseZone.y2) {
            if (gameRunning) gamePaused = !gamePaused;
            return;
        }
        if (fx >= menuZone.x1 && fx <= menuZone.x2 && fy >= menuZone.y1 && fy <= menuZone.y2) {
            currentScene = SCENE_MENU;
            gameRunning = false;
            gamePaused = false;
            return;
        }

        // In-game click: move basket center to click (original behavior)
        if (fx - basketHalfW < 8) fx = 8 + basketHalfW;
        if (fx + basketHalfW > WIN_W - 8) fx = WIN_W - 8 - basketHalfW;
        basketX = fx;
    }
}

// -----------------------------------------------------------------------------
// Render
void drawButton(ClickZone z, const char* text) {
    // Draw button background
    glColor4f(0.2f, 0.2f, 0.2f, 0.7f);
    glRectf(z.x1, z.y1, z.x2, z.y2);
    // Draw button border
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
      glVertex2f(z.x1, z.y1);
      glVertex2f(z.x2, z.y1);
      glVertex2f(z.x2, z.y2);
      glVertex2f(z.x1, z.y2);
    glEnd();
    glLineWidth(1.0f);
    // Draw text (centered vertically)
    float textY = z.y1 + (z.y2 - z.y1) / 2.0f - 8.0f;
    drawText(text, z.x1 + 15, textY, GLUT_BITMAP_HELVETICA_18, 1,1,1);
}

void renderHUD() {
    char buf[128];

    // --- Score and Time ---
    sprintf(buf, "Score: %d", scoreVal);
    drawText(buf, 18, WIN_H - 35, GLUT_BITMAP_HELVETICA_18, 0,0,0);
    drawText(buf, 20, WIN_H - 33, GLUT_BITMAP_HELVETICA_18, 1,1,1); // white shadow


    sprintf(buf, "Time: %02d:%02d", gameTime/60, gameTime%60);
    drawText(buf, WIN_W/2.0f - 60, WIN_H - 35, GLUT_BITMAP_HELVETICA_18, 0,0,0);
    drawText(buf, WIN_W/2.0f - 58, WIN_H - 33, GLUT_BITMAP_HELVETICA_18, 1,1,1); // white shadow

    // --- Buttons ---
    const char* pauseText = (gamePaused) ? "Resume" : "Pause";
    drawButton(pauseZone, pauseText);
    drawButton(menuZone, "Menu");

    // --- Active Perks ---
    int px = 20;
    // Draw shadow first for readability
    if (perk_big_basket_timer > 0.0f) {
        sprintf(buf, "Big Basket: %.0fs", perk_big_basket_timer);
        drawText(buf, px+1, WIN_H - 59, GLUT_BITMAP_HELVETICA_12, 0,0,0);
        drawText(buf, px, WIN_H - 60, GLUT_BITMAP_HELVETICA_12, 0.8f, 0.1f, 0.8f);
        px += 140;
    }
    // <-- NEW: Small basket timer -->
    if (perk_small_basket_timer > 0.0f) {
        sprintf(buf, "Small Basket: %.0fs", perk_small_basket_timer);
        drawText(buf, px+1, WIN_H - 59, GLUT_BITMAP_HELVETICA_12, 0,0,0);
        drawText(buf, px, WIN_H - 60, GLUT_BITMAP_HELVETICA_12, 1.0f, 0.2f, 0.2f);
        px += 140;
    }
    if (perk_slow_fall_timer > 0.0f) {
        sprintf(buf, "Slow Fall: %.0fs", perk_slow_fall_timer);
        drawText(buf, px+1, WIN_H - 59, GLUT_BITMAP_HELVETICA_12, 0,0,0);
        drawText(buf, px, WIN_H - 60, GLUT_BITMAP_HELVETICA_12, 0.8f, 0.1f, 0.8f);
        px += 140;
    }
    // <-- NEW: Multiplier timer -->
    if (perk_multiplier_timer > 0.0f) {
        sprintf(buf, "2x SCORE: %.0fs", perk_multiplier_timer);
        drawText(buf, px+1, WIN_H - 59, GLUT_BITMAP_HELVETICA_12, 0,0,0);
        drawText(buf, px, WIN_H - 60, GLUT_BITMAP_HELVETICA_12, 1.0f, 1.0f, 0.2f);
    }
}

// <-- NEW: Function to draw wind effects -->
void drawWind() {
    if (windSpeed == 0.0f) return;
    glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x00FF); // Dashed line pattern
    glLineWidth(2.0f);
    for (const auto& line : windLines) {
        glBegin(GL_LINES);
          glVertex2f(line.x, line.y);
          glVertex2f(line.x + windSpeed * 0.2f, line.y); // Short lines indicating direction
        glEnd();
    }
    glDisable(GL_LINE_STIPPLE);
    glLineWidth(1.0f);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND); // Ensure blend is enabled for overlays
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (currentScene == SCENE_MENU) {
        if (texMenu) drawFullScreenTexture(texMenu);
        else {
            glColor3f(0.9f,0.9f,1.0f);
            glRectf(0,0,WIN_W,WIN_H);
        }

        // --- High Score Display (Middle and Bold) ---
        std::stringstream ss;
        ss << "Highest Score: " << highScore;
        std::string highScoreStr = ss.str();

        void* font = GLUT_BITMAP_TIMES_ROMAN_24;

        // Calculate text width for centering
        int textWidth = 0;
        for (size_t i = 0; i < highScoreStr.length(); ++i) {
            textWidth += glutBitmapWidth(font, highScoreStr[i]);
        }
        float textX = (WIN_W - textWidth) / 2.0f;
        float textY = WIN_H / 2.0f + 100; // Adjust Y to be visually central

        // Draw shadow first for boldness effect
        drawText(highScoreStr.c_str(), textX + 2, textY + 2, font, 0.1f, 0.1f, 0.1f);
        // Draw main text
        drawText(highScoreStr.c_str(), textX, textY, font, 1.0f, 1.0f, 0.0f); // Yellow for visibility

        // --- Footer Text ---
        const char* footer = "Game developed by Delowar and Oboni";
        int footerWidth = 0;
        for (const char* p=footer; *p; ++p) footerWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *p);

        drawText(footer, (WIN_W - footerWidth) / 2.0f, 20, GLUT_BITMAP_HELVETICA_18, 0,0,0);

    }
    else if (currentScene == SCENE_GAME) {
        // background
        if (texGame) drawFullScreenTexture(texGame);
        else { glColor3f(0.8f,0.95f,1.0f); glRectf(0,0,WIN_W,WIN_H); }

        // <-- NEW: Draw Wind -->
        drawWind();

        // wires (draw for all chickens)
        glColor3f(0.2f,0.2f,0.2f);
        glLineWidth(3.0f);
        for (const auto& ch : chickens) {
            glBegin(GL_LINES);
              glVertex2f(20.0f, ch.wireY+10.0f);
              glVertex2f(WIN_W-20.0f, ch.wireY+10.0f);
            glEnd();
        }

        // chicken, items, basket, hud
        drawChickens(); // updated
        drawItems();
        drawBasket();
        renderHUD(); // updated

        // paused overlay
        if (!gameRunning) {
            glColor4f(0,0,0,0.4f);
            glRectf(0,0,WIN_W,WIN_H);

            std::stringstream ss;
            ss << "TIME UP! Final Score: " << scoreVal;
            std::string finalScoreStr = ss.str();

            int finalScoreWidth = 0;
            for (size_t i = 0; i < finalScoreStr.length(); ++i) {
                finalScoreWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, finalScoreStr[i]);
            }
            drawText(finalScoreStr.c_str(), (WIN_W - finalScoreWidth)/2.0f, WIN_H/2.0f + 20, GLUT_BITMAP_HELVETICA_18, 1,1,1);

            if (scoreVal >= highScore && scoreVal > 0) {
                 const char* newHigh = "NEW HIGH SCORE!";
                 int newHighWidth = 0;
                 for (const char* p=newHigh; *p; ++p) newHighWidth += glutBitmapWidth(GLUT_BITMAP_TIMES_ROMAN_24, *p);
                 drawText(newHigh, (WIN_W - newHighWidth)/2.0f, WIN_H/2.0f + 50, GLUT_BITMAP_TIMES_ROMAN_24, 1,1,0);
            }
            const char* escMsg = "Press ESC to return to menu.";
            int escMsgWidth = 0;
            for (const char* p=escMsg; *p; ++p) escMsgWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *p);
            drawText(escMsg, (WIN_W - escMsgWidth)/2.0f, WIN_H/2.0f - 40, GLUT_BITMAP_HELVETICA_18, 1,1,1);

        } else if (gamePaused) {
            glColor4f(0,0,0,0.4f);
            glRectf(0,0,WIN_W,WIN_H);

            const char* pauseMsg = "PAUSED";
            int pauseMsgWidth = 0;
            for (const char* p=pauseMsg; *p; ++p) pauseMsgWidth += glutBitmapWidth(GLUT_BITMAP_TIMES_ROMAN_24, *p);
            drawText(pauseMsg, (WIN_W - pauseMsgWidth)/2.0f, WIN_H/2.0f, GLUT_BITMAP_TIMES_ROMAN_24, 1,1,1);

            const char* resumeMsg = "Click Resume or press R";
            int resumeMsgWidth = 0;
            for (const char* p=resumeMsg; *p; ++p) resumeMsgWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *p);
            drawText(resumeMsg, (WIN_W - resumeMsgWidth)/2.0f, WIN_H/2.0f - 30, GLUT_BITMAP_HELVETICA_18, 1,1,1);
        }
    }
    // <-- NEW: Help Screen Rendering -->
    else if (currentScene == SCENE_HELP) {
        // Draw menu as background
        if (texMenu) drawFullScreenTexture(texMenu);
        else { glColor3f(0.9f,0.9f,1.0f); glRectf(0,0,WIN_W,WIN_H); }

        // Dark overlay
        glColor4f(0,0,0,0.7f);
        glRectf(100, 80, WIN_W-100, WIN_H-80);

        // Draw Help Text
        drawText("How to Play", WIN_W/2-100, WIN_H-120, GLUT_BITMAP_TIMES_ROMAN_24, 1,1,0);
        float y = WIN_H - 200;
        drawText("Objective: Catch as many valuable items as you can before time runs out!", 150, y, GLUT_BITMAP_HELVETICA_18, 1,1,1); y-=30;
        drawText("Controls: Move the mouse left and right to control the basket.", 150, y, GLUT_BITMAP_HELVETICA_18, 1,1,1); y-=40;

        drawText("Items & Scoring:", 150, y, GLUT_BITMAP_HELVETICA_18, 1,1,0); y-=30;
        drawText("- Normal Egg (White): +1 Point", 180, y, GLUT_BITMAP_HELVETICA_18, 1,1,1); y-=25;
        drawText("- Blue Gem (Blue): +5 Points", 180, y, GLUT_BITMAP_HELVETICA_18, 1,1,1); y-=25;
        drawText("- Gold Gem (Yellow): +10 Points", 180, y, GLUT_BITMAP_HELVETICA_18, 1,1,1); y-=25;
        drawText("- Poop (Brown): -10 Points", 180, y, GLUT_BITMAP_HELVETICA_18, 1,0.5,0.5); y-=25;
        drawText("- Rotten Egg (Green): -5 Points & Shrinks Basket", 180, y, GLUT_BITMAP_HELVETICA_18, 1,0.5,0.5); y-=40;

        drawText("Perks & Wind:", 150, y, GLUT_BITMAP_HELVETICA_18, 1,1,0); y-=30;
        drawText("- Pink Star: Big Basket or Slow Fall", 180, y, GLUT_BITMAP_HELVETICA_18, 1,1,1); y-=25;
        drawText("- Purple Star: 2x Score Multiplier!", 180, y, GLUT_BITMAP_HELVETICA_18, 1,1,1); y-=25;
        drawText("- Wind (White Lines): Gusts will blow items sideways!", 180, y, GLUT_BITMAP_HELVETICA_18, 1,1,1); y-=25;

        drawButton(backZone, "Back to Menu");
    }

    glutSwapBuffers();
}

// -----------------------------------------------------------------------------
// Initialization + main
void initGL() {
    glClearColor(0.85f,0.9f,1.0f,1.0f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    texMenu = tryLoadTexture(IMG_PATH, FALLBACK_MENU);
    if (!texMenu) fprintf(stderr, "Menu texture NOT loaded.\n");
    texGame = tryLoadTexture(GAME_BG_PATH, FALLBACK_GAME);
    if (!texGame) fprintf(stderr, "Game texture not loaded.\n");

    // <-- NEW: Initialize wind lines -->
    for(int i=0; i<30; ++i) {
        windLines.push_back({(float)(rand()%WIN_W), (float)(100 + rand()%(WIN_H-200))});
    }
}

// -----------------------------------------------------------------------------
// Main
int main(int argc, char** argv) {
    srand((unsigned)time(NULL));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Catch The Eggs - Playable"); // <-- Kept your original title
    initGL();

    loadHighScore(); // Load high score at start

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard_cb);
    glutPassiveMotionFunc(passiveMouse_cb);
    glutMouseFunc(mouseClick_cb);
    glutTimerFunc(TIMER_MS, timer_cb, 0);
    glutReshapeFunc([](int w,int h){ glutReshapeWindow(WIN_W, WIN_H); });

    // Start in menu
    currentScene = SCENE_MENU;

    glutMainLoop();
    return 0;
}
