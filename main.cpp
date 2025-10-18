
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <GL/glut.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

// Textures
GLuint texMenu = 0;
GLuint texGame = 0;

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
    unsigned char* data = stbi_load(path, &w, &h, &comp, STBI_rgb_alpha);
    if (!data) {
        fprintf(stderr, "stbi_load failed for '%s' -> %s\n", path, stbi_failure_reason());
        return 0;
    }
    printf("Loaded image '%s' (%d x %d), orig_comp=%d\n", path, w, h, comp);

    GLuint tid = 0;
    glGenTextures(1, &tid);
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

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (currentScene == SCENE_MENU) {
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
    }

    glutSwapBuffers();
}

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

    glutMainLoop();
    return 0;
}
