<<<<<<< HEAD
// test_bg.cpp
// Menu (menu_background.png) -> Game (game_background.png) switching demo.
// Requirements: stb_image.h in same folder; menu_background.png and game_background.png accessible.
// Compile example (MinGW/Windows):
//   g++ test_bg.cpp -o test_bg.exe -lopengl32 -lglu32 -lfreeglut
//"C:/Users/MR_DEL/Documents/catch-the-eggs-v3/game_background.png"
// catch_the_eggs.cpp
// Menu -> Game playable "Catch the Eggs" prototype
// Requirements: stb_image.h in same folder; menu_background.png and game_background.png accessible.
// Compile (MinGW/Windows):
//   g++ catch_the_eggs.cpp -o catch_the_eggs.exe -lopengl32 -lglu32 -lfreeglut

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <ctime>
=======

#include <cstdio>
#include <cstdlib>
#include <cstring>
>>>>>>> bb171153cb90817d9ecb3f837a07a661cd2ffc50

#include <GL/glut.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

<<<<<<< HEAD
#ifdef _WIN32
  #include <direct.h>
  #define GetCurrentDir _getcwd
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

// -----------------------------------------------------------------------------
=======
// Cross-platform getcwd wrapper
#ifdef _WIN32
    #include <direct.h>   // for _getcwd
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>   // for getcwd
    #define GetCurrentDir getcwd
#endif

// Logical window size
const int WIN_W = 1024;
const int WIN_H = 768;

// If you prefer absolute paths, set them here (otherwise leave empty and place PNGs next to exe)
const char* IMG_PATH = ""; // absolute path for menu image (optional)
const char* GAME_BG_PATH = ""; // absolute path for game image (optional)

// Fallback file names (looked up in working directory)
const char* FALLBACK_MENU  = "C:/Users/MR_DEL/Documents/catch-the-eggs-v3/menu_background.png";
const char* FALLBACK_GAME  = "C:/Users/MR_DEL/Documents/catch-the-eggs-v3/game_background.png";

>>>>>>> bb171153cb90817d9ecb3f837a07a661cd2ffc50
// Textures
GLuint texMenu = 0;
GLuint texGame = 0;

<<<<<<< HEAD
// Scenes
enum Scene { SCENE_MENU, SCENE_GAME };
Scene currentScene = SCENE_MENU;

// Menu click zones (tuned to your background)
struct ClickZone { float x1,y1,x2,y2; const char* name; };
ClickZone startZone = {770.0f, 440.0f, 970.0f, 520.0f, "Start"};
ClickZone quitZone  = {770.0f, 340.0f, 970.0f, 420.0f, "Quit"};

// -----------------------------------------------------------------------------
// Game data: eggs, perks, basket, chicken
enum ItemType { ITEM_EGG_NORMAL, ITEM_EGG_BLUE, ITEM_EGG_GOLD, ITEM_POOP, ITEM_PERK };
enum PerkType { PERK_NONE, PERK_BIG_BASKET, PERK_SLOW_FALL, PERK_EXTRA_TIME };

struct Item {
    ItemType type;
    float x, y;
    float vy;
    bool active;
    PerkType perk; // only used if type == ITEM_PERK
};

std::vector<Item> items;

// basket
float basketX = WIN_W/2.0f;
float basketY = 90.0f;
float basketBaseHalfW = 60.0f; // half width
float basketHalfW = basketBaseHalfW; // may change by big-basket perk

// chicken on wire
float chickenX = 220.0f;
float chickenY = WIN_H - 160.0f;
float chickenSpeed = 120.0f; // px/sec
int chickenDir = 1; // 1 = right, -1 = left

// game rules
int scoreVal = 0;
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
float perk_slow_fall_timer = 0.0f;

// -----------------------------------------------------------------------------
// Utility: load texture file (stb_image)
GLuint loadTextureFromFile(const char* path) {
    int w=0,h=0,comp=0;
    stbi_set_flip_vertically_on_load(true);
=======
// Scene state
enum Scene { SCENE_MENU, SCENE_GAME };
Scene currentScene = SCENE_MENU;

// Click zone structure (OpenGL coords, origin bottom-left)
struct ClickZone {
    float x1, y1, x2, y2;
    const char* name;
};
// Zones tuned for the menu image used earlier
ClickZone startZone = {770.0f, 440.0f, 970.0f, 520.0f, "Start"};
ClickZone quitZone  = {770.0f, 340.0f, 970.0f, 420.0f, "Quit"};

// Optional debug overlay for zones
bool showDebugZones = false;

// ---------------- Texture loading helpers ----------------
GLuint loadTextureFromFile(const char* path) {
    int w=0,h=0,comp=0;
    stbi_set_flip_vertically_on_load(true); // flip to match OpenGL coords
>>>>>>> bb171153cb90817d9ecb3f837a07a661cd2ffc50
    unsigned char* data = stbi_load(path, &w, &h, &comp, STBI_rgb_alpha);
    if (!data) {
        fprintf(stderr, "stbi_load failed for '%s' -> %s\n", path, stbi_failure_reason());
        return 0;
    }
<<<<<<< HEAD
    GLuint tid = 0;
    glGenTextures(1,&tid);
    glBindTexture(GL_TEXTURE_2D, tid);
=======
    printf("Loaded image '%s' (%d x %d), orig_comp=%d\n", path, w, h, comp);

    GLuint tid = 0;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);

>>>>>>> bb171153cb90817d9ecb3f837a07a661cd2ffc50
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif
<<<<<<< HEAD
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
=======

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    printf("Texture uploaded to GPU, id=%u\n", (unsigned)tid);
    return tid;
}

// Try configured path (if set) then fallback filename(s) in working dir, then parent
GLuint tryLoadTexture(const char* configuredPath, const char* fallbackName) {
    // 1) configured absolute path
    if (configuredPath && configuredPath[0] != '\0') {
        printf("Trying configured path: %s\n", configuredPath);
        GLuint t = loadTextureFromFile(configuredPath);
        if (t) return t;
    }

    // 2) fallback in CWD
    char cwd[1024] = {0};
    if (GetCurrentDir(cwd, sizeof(cwd))) printf("CWD: %s\n", cwd);
    else printf("Could not get CWD.\n");

    printf("Attempting fallback file in CWD: %s\n", fallbackName);
    {
        GLuint t = loadTextureFromFile(fallbackName);
        if (t) return t;
    }

    // 3) try parent folder (useful if exe runs in bin/Debug)
    if (GetCurrentDir(cwd, sizeof(cwd))) {
        char p[1100];
        snprintf(p, sizeof(p), "%s/../%s", cwd, fallbackName);
        printf("Attempting parent path: %s\n", p);
        GLuint t = loadTextureFromFile(p);
        if (t) return t;
    }

    return 0;
}
// ---------------------------------------------------------

void initGL() {
    glClearColor(0.8f,0.8f,0.8f,1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Load menu background
    texMenu = tryLoadTexture(IMG_PATH, FALLBACK_MENU);
    if (texMenu == 0) {
        fprintf(stderr, "ERROR: failed to load menu background. Place '%s' next to exe or set IMG_PATH.\n", FALLBACK_MENU);
    } else {
        printf("Menu background loaded (id=%u)\n", (unsigned)texMenu);
    }

    // Load game background (separate texture)
    texGame = tryLoadTexture(GAME_BG_PATH, FALLBACK_GAME);
    if (texGame == 0) {
        printf("Warning: failed to load game background '%s'. Game will fallback to menu bg.\n", FALLBACK_GAME);
    } else {
        printf("Game background loaded (id=%u)\n", (unsigned)texGame);
    }
}

// Draw full-screen textured quad (assumes ortho matches window)
void drawFullScreenTexture(GLuint t) {
    if (t == 0) return;
>>>>>>> bb171153cb90817d9ecb3f837a07a661cd2ffc50
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

<<<<<<< HEAD
void drawText(const char* s, float x, float y) {
    glRasterPos2f(x,y);
    for (const char* p=s; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
}

// draw a simple chicken circle (replace with sprite later)
void drawChicken() {
    const float r = 28.0f;
    glColor3f(1.0f, 0.45f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
      glVertex2f(chickenX, chickenY);
      for(int i=0;i<=32;i++){
        float a = i * 2.0f * 3.1415926f / 32.0f;
        glVertex2f(chickenX + cosf(a)*r, chickenY + sinf(a)*r);
      }
    glEnd();
}

// draw basket rectangle
void drawBasket() {
    glColor3f(0.85f,0.7f,0.45f);
    float hx = basketHalfW;
    glBegin(GL_QUADS);
      glVertex2f(basketX - hx, basketY - 12);
      glVertex2f(basketX + hx, basketY - 12);
      glVertex2f(basketX + hx, basketY + 24);
      glVertex2f(basketX - hx, basketY + 24);
    glEnd();
    // rim
    glColor3f(1,1,1);
    glBegin(GL_QUADS);
      glVertex2f(basketX - hx + 6, basketY + 24);
      glVertex2f(basketX + hx - 6, basketY + 24);
      glVertex2f(basketX + hx - 6, basketY + 28);
      glVertex2f(basketX - hx + 6, basketY + 28);
    glEnd();
}

// draw items (eggs/poop/perk)
void drawItems() {
    for (auto &it : items) {
        if (!it.active) continue;
        switch (it.type) {
          case ITEM_EGG_NORMAL:
            glColor3f(1.0f, 1.0f, 0.9f); // pale yellow
            break;
          case ITEM_EGG_BLUE:
            glColor3f(0.4f, 0.7f, 1.0f);
            break;
          case ITEM_EGG_GOLD:
            glColor3f(1.0f, 0.86f, 0.2f);
            break;
          case ITEM_POOP:
            glColor3f(0.12f, 0.08f, 0.02f);
            break;
          case ITEM_PERK:
            glColor3f(0.8f, 0.4f, 0.9f);
            break;
        }
        // draw oval for egg/poop, square for perk
        if (it.type == ITEM_PERK) {
            float s = 16;
            glBegin(GL_QUADS);
              glVertex2f(it.x-s, it.y-s);
              glVertex2f(it.x+s, it.y-s);
              glVertex2f(it.x+s, it.y+s);
              glVertex2f(it.x-s, it.y+s);
            glEnd();
        } else {
            // oval
            glBegin(GL_TRIANGLE_FAN);
              glVertex2f(it.x, it.y);
              for (int i=0;i<=20;i++){
                float a = i * 2.0f * 3.1415926f / 20.0f;
                glVertex2f(it.x + cosf(a)*8.0f, it.y + sinf(a)*12.0f);
              }
            glEnd();
        }
    }
}

// -----------------------------------------------------------------------------
// Game logic helpers
void resetGame() {
    items.clear();
    basketX = WIN_W/2.0f;
    basketHalfW = basketBaseHalfW;
    chickenX = 220.0f;
    chickenDir = 1;
    chickenSpeed = 120.0f;
    scoreVal = 0;
    gameTime = 60;
    spawnAccumulator = 0.0f;
    baseFallSpeed = 180.0f;
    fallSpeedMultiplier = 1.0f;
    perk_big_basket_timer = 0.0f;
    perk_slow_fall_timer = 0.0f;
    gameRunning = true;
    gamePaused = false;
}

// spawn an item at chickenX with random type/rarity
void spawnItem() {
    Item it;
    float r = (float)rand() / RAND_MAX;
    if (r < 0.02f) { // rare: gold
        it.type = ITEM_EGG_GOLD;
    } else if (r < 0.12f) { // blue eggs
        it.type = ITEM_EGG_BLUE;
    } else if (r < 0.17f) { // poop
        it.type = ITEM_POOP;
    } else if (r < 0.22f) { // perk (small chance)
        it.type = ITEM_PERK;
        // pick a perk randomly
        float p = (float)rand() / RAND_MAX;
        if (p < 0.34f) it.perk = PERK_BIG_BASKET;
        else if (p < 0.67f) it.perk = PERK_SLOW_FALL;
        else it.perk = PERK_EXTRA_TIME;
    } else {
        it.type = ITEM_EGG_NORMAL;
    }
    it.x = chickenX + ( (rand()%41) - 20 ); // small horizontal jitter
    it.y = chickenY - 18.0f;
    it.vy = baseFallSpeed;
    it.active = true;
    items.push_back(it);
}

// apply perk
void applyPerk(PerkType p) {
    if (p == PERK_BIG_BASKET) {
        basketHalfW = basketBaseHalfW * 1.7f;
        perk_big_basket_timer = 8.0f; // seconds
    } else if (p == PERK_SLOW_FALL) {
        fallSpeedMultiplier = 0.55f;
        perk_slow_fall_timer = 8.0f;
    } else if (p == PERK_EXTRA_TIME) {
        gameTime += 10;
    }
}

// handle caught item or missed
void handleCatch(Item &it) {
    if (it.type == ITEM_EGG_NORMAL) scoreVal += 1;
    else if (it.type == ITEM_EGG_BLUE) scoreVal += 5;
    else if (it.type == ITEM_EGG_GOLD) scoreVal += 10;
    else if (it.type == ITEM_POOP) scoreVal -= 10;
    else if (it.type == ITEM_PERK) {
        applyPerk(it.perk);
    }
}

// -----------------------------------------------------------------------------
// Update (dt in seconds)
void updateGame(float dt) {
    if (!gameRunning || gamePaused) return;

    // chicken movement
    chickenX += chickenDir * chickenSpeed * dt;
    if (chickenX > WIN_W - 160) { chickenX = WIN_W - 160; chickenDir = -1; }
    if (chickenX < 160)        { chickenX = 160;        chickenDir =  1; }

    // spawn logic
    spawnAccumulator += dt;
    if (spawnAccumulator >= spawnInterval) {
        spawnItem();
        spawnAccumulator = 0.0f;
        // slightly randomize next spawn interval
        spawnInterval = 0.8f + (rand()%100)/200.0f; // 0.8..1.3
    }

    // move items
    for (auto &it : items) {
        if (!it.active) continue;
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
    if (perk_slow_fall_timer > 0.0f) {
        perk_slow_fall_timer -= dt;
        if (perk_slow_fall_timer <= 0.0f) { fallSpeedMultiplier = 1.0f; perk_slow_fall_timer = 0.0f; }
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
    last_ms = now_ms;

    if (currentScene == SCENE_GAME && gameRunning && !gamePaused) {
        // decrement game time every second (we accumulate)
        static float timeAcc = 0.0f;
        timeAcc += dt;
        if (timeAcc >= 1.0f) {
            gameTime -= 1;
            timeAcc -= 1.0f;
            if (gameTime <= 0) {
                gameRunning = false;
                gamePaused = false;
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
    if (currentScene == SCENE_MENU) {
        if (key == 27) exit(0);
        if (key == ' ' || key == 's' || key == 'S') {
            // start game
            resetGame();
            currentScene = SCENE_GAME;
        }
    } else if (currentScene == SCENE_GAME) {
        if (key == 27) { // ESC -> back to menu
            currentScene = SCENE_MENU;
            gameRunning = false;
        } else if (key == 'p' || key == 'P') {
            gamePaused = !gamePaused;
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
    float fy = invy(my);
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (currentScene == SCENE_MENU) {
            if (mx >= startZone.x1 && mx <= startZone.x2 && fy >= startZone.y1 && fy <= startZone.y2) {
                resetGame();
                currentScene = SCENE_GAME;
                return;
            }
            if (mx >= quitZone.x1 && mx <= quitZone.x2 && fy >= quitZone.y1 && fy <= quitZone.y2) {
                exit(0);
            }
        } else if (currentScene == SCENE_GAME) {
            // In-game click: could be used to drop basket or special action; here we move basket center to click
            float fx = (float)mx;
            if (fx - basketHalfW < 8) fx = 8 + basketHalfW;
            if (fx + basketHalfW > WIN_W - 8) fx = WIN_W - 8 - basketHalfW;
            basketX = fx;
        }
    }
}

// -----------------------------------------------------------------------------
// Render
void renderHUD() {
    char buf[128];
    sprintf(buf, "Score: %d", scoreVal);
    glColor3f(0,0,0);
    glRasterPos2f(18, WIN_H - 28);
    for (const char* p=buf; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);

    sprintf(buf, "Time: %02d:%02d", gameTime/60, gameTime%60);
    glRasterPos2f(WIN_W - 170, WIN_H - 28);
    for (const char* p=buf; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);

    // show active perks
    int px = 18;
    if (perk_big_basket_timer > 0.0f) {
        sprintf(buf, "Big Basket: %.0fs", perk_big_basket_timer);
        glRasterPos2f(px, WIN_H - 54);
        for (const char* p=buf; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);
        px += 140;
    }
    if (perk_slow_fall_timer > 0.0f) {
        sprintf(buf, "Slow Fall: %.0fs", perk_slow_fall_timer);
        glRasterPos2f(px, WIN_H - 54);
        for (const char* p=buf; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);
        px += 140;
    }
}
=======
// debug overlay rectangle
void drawRectOverlay(float x1, float y1, float x2, float y2, float r, float g, float b, float a) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r,g,b,a);
    glBegin(GL_QUADS);
      glVertex2f(x1,y1); glVertex2f(x2,y1); glVertex2f(x2,y2); glVertex2f(x1,y2);
    glEnd();
    glDisable(GL_BLEND);
}

inline float invy(int y) { return (float)(WIN_H - y); }
>>>>>>> bb171153cb90817d9ecb3f837a07a661cd2ffc50

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (currentScene == SCENE_MENU) {
<<<<<<< HEAD
        if (texMenu) drawFullScreenTexture(texMenu);
        else {
            glColor3f(0.9f,0.9f,1.0f);
            glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WIN_W,0); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H); glEnd();
        }
        // small hint
        glColor3f(0,0,0);
        glRasterPos2f(20, 18);
        const char* hint = "Click START or press SPACE to play. ESC to quit.";
        for (const char* p=hint; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);
    }
    else if (currentScene == SCENE_GAME) {
        // background
        if (texGame) drawFullScreenTexture(texGame);
        else if (texMenu) drawFullScreenTexture(texMenu);
        else { glColor3f(0.8f,0.95f,1.0f); glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WIN_W,0); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H); glEnd(); }

        // wire
        glColor3f(0.2f,0.2f,0.2f);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
          glVertex2f(20.0f, chickenY+10.0f);
          glVertex2f(WIN_W-20.0f, chickenY+10.0f);
        glEnd();

        // chicken, items, basket, hud
        drawChicken();
        drawItems();
        drawBasket();
        renderHUD();

        // paused overlay
        if (!gameRunning) {
            glColor4f(0,0,0,0.4f);
            glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WIN_W,0); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H); glEnd();
            glColor3f(1,1,1);
            const char* over = "TIME UP! Press ESC to return to menu.";
            glRasterPos2f(WIN_W/2 - 140, WIN_H/2);
            for (const char* p=over; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
        } else if (gamePaused) {
            glColor4f(0,0,0,0.4f);
            glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WIN_W,0); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H); glEnd();
            glColor3f(1,1,1);
            const char* pauseMsg = "PAUSED - Press R to resume or ESC to menu";
            glRasterPos2f(WIN_W/2 - 180, WIN_H/2);
            for (const char* p=pauseMsg; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
        }
=======
        if (texMenu != 0) drawFullScreenTexture(texMenu);
        else {
            // fallback colored background
            glColor3f(0.9f,0.6f,0.6f);
            glBegin(GL_QUADS);
              glVertex2f(0,0); glVertex2f(WIN_W,0); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
            glEnd();
        }

        if (showDebugZones) {
            drawRectOverlay(startZone.x1, startZone.y1, startZone.x2, startZone.y2, 1,1,0,0.25f);
            drawRectOverlay(quitZone.x1, quitZone.y1, quitZone.x2, quitZone.y2, 1,0,0,0.25f);
        }

        // small status text
        glColor3f(0,0,0);
        glRasterPos2f(8,12);
        const char* msg = (texMenu != 0) ? "Menu: Click Start or Quit. Press Space to Start." : "Menu texture not loaded.";
        for (const char* p=msg; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);
    }
    else if (currentScene == SCENE_GAME) {
        // draw game background (fallback to menu if missing)
        if (texGame != 0) drawFullScreenTexture(texGame);
        else if (texMenu != 0) drawFullScreenTexture(texMenu);
        else {
            glColor3f(0.95f,0.98f,0.95f);
            glBegin(GL_QUADS);
              glVertex2f(0,0); glVertex2f(WIN_W,0); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
            glEnd();
        }

        // draw placeholder wire for chicken (near top quarter)
        glColor3f(0.2f,0.2f,0.2f);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
            glVertex2f(20.0f, WIN_H - 160.0f);
            glVertex2f((float)WIN_W - 20.0f, WIN_H - 160.0f);
        glEnd();

        // placeholder basket at bottom center
        glColor3f(0.76f, 0.58f, 0.33f);
        glBegin(GL_QUADS);
            glVertex2f(WIN_W/2 - 60, 60);
            glVertex2f(WIN_W/2 + 60, 60);
            glVertex2f(WIN_W/2 + 60, 120);
            glVertex2f(WIN_W/2 - 60, 120);
        glEnd();

        // HUD placeholder: score and time
        glColor3f(0,0,0);
        glRasterPos2f(20, WIN_H - 28);
        const char* scoreMsg = "Score: 0";
        for (const char* p=scoreMsg; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);

        glRasterPos2f(WIN_W - 160, WIN_H - 28);
        const char* timeMsg = "Time: 02:00";
        for (const char* p=timeMsg; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
>>>>>>> bb171153cb90817d9ecb3f837a07a661cd2ffc50
    }

    glutSwapBuffers();
}

<<<<<<< HEAD
// -----------------------------------------------------------------------------
// Initialization + main
void initGL() {
    glClearColor(0.85f,0.9f,1.0f,1.0f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    texMenu = tryLoadTexture(IMG_PATH, FALLBACK_MENU);
    if (!texMenu) fprintf(stderr, "Menu texture NOT loaded. Put '%s' next to exe or set IMG_PATH.\n", FALLBACK_MENU);
    texGame = tryLoadTexture(GAME_BG_PATH, FALLBACK_GAME);
    if (!texGame) fprintf(stderr, "Game texture not loaded. Put '%s' next to exe or set GAME_BG_PATH.\n", FALLBACK_GAME);
}

// -----------------------------------------------------------------------------
// Main
int main(int argc, char** argv) {
    srand((unsigned)time(NULL));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Catch The Eggs - Playable");
    initGL();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard_cb);
    glutPassiveMotionFunc(passiveMouse_cb);
    glutMouseFunc(mouseClick_cb);
    glutTimerFunc(TIMER_MS, timer_cb, 0);
    glutReshapeFunc([](int w,int h){ glutReshapeWindow(WIN_W, WIN_H); });

    // Start in menu
    currentScene = SCENE_MENU;
=======
// Mouse click handler
void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        float fy = invy(y);
        if (currentScene == SCENE_MENU) {
            // Start zone
            if (x >= startZone.x1 && x <= startZone.x2 && fy >= startZone.y1 && fy <= startZone.y2) {
                printf("Start clicked - switching to GAME scene\n");
                currentScene = SCENE_GAME;
                glutPostRedisplay();
                return;
            }
            // Quit zone
            if (x >= quitZone.x1 && x <= quitZone.x2 && fy >= quitZone.y1 && fy <= quitZone.y2) {
                printf("Quit clicked - exiting\n");
                exit(0);
            }
        } else if (currentScene == SCENE_GAME) {
            // in-game clicks can be handled here later (catching eggs etc.)
        }
    }
}

void passiveMouse(int x, int y) {
    (void)x; (void)y; // not used now
}

void keyboard(unsigned char k, int x, int y) {
    (void)x; (void)y;
    if (k == 27) { // ESC
        if (currentScene == SCENE_GAME) {
            currentScene = SCENE_MENU;
            glutPostRedisplay();
            return;
        }
        exit(0);
    } else if (k == ' ') { // SPACE -> start
        if (currentScene == SCENE_MENU) {
            currentScene = SCENE_GAME;
            glutPostRedisplay();
        }
    }
}

void reshape(int w, int h) {
    // keep fixed logical resolution to simplify coordinates
    glutReshapeWindow(WIN_W, WIN_H);
}

int main(int argc, char** argv) {
    printf("Starting menu->game demo. Menu image: '%s'  Game image: '%s'\n",
           (IMG_PATH && IMG_PATH[0]) ? IMG_PATH : FALLBACK_MENU,
           (GAME_BG_PATH && GAME_BG_PATH[0]) ? GAME_BG_PATH : FALLBACK_GAME);

    char cwd[1024];
    if (GetCurrentDir(cwd, sizeof(cwd))) printf("CWD: %s\n", cwd);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Catch the Eggs - Menu -> Game demo");
    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseClick);
    glutPassiveMotionFunc(passiveMouse);
    glutKeyboardFunc(keyboard);
>>>>>>> bb171153cb90817d9ecb3f837a07a661cd2ffc50

    glutMainLoop();
    return 0;
}
<<<<<<< HEAD


=======
>>>>>>> bb171153cb90817d9ecb3f837a07a661cd2ffc50
