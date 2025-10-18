// test_bg.cpp
// Minimal test: load PNG via stb_image and draw fullscreen textured quad.
//
// - Put stb_image.h in the same folder.
// - Put menu_background.png either next to the exe or set IMG_PATH to an absolute path.
// Compile (example):
//  g++ test_bg.cpp -o test_bg -lGL -lGLU -lglut    (Linux/MinGW)


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

// Window size (logical)
const int WIN_W = 1024;
const int WIN_H = 768;

// If you prefer absolute path, set it here. Otherwise set to empty string ""
// const char* IMG_PATH = "C:/Users/MR_DEL/Documents/catch-the-eggs-v3/menu_background.png";
const char* IMG_PATH = ""; // leave empty to use local "menu_background.png"

const char* FALLBACK_NAME = "C:/Users/MR_DEL/Documents/catch-the-eggs-v3/menu_background.png";

GLuint texID = 0;

// Try to load texture from 'path'. Returns 0 on failure.
GLuint loadTextureFromFile(const char* path) {
    int w = 0, h = 0, comp = 0;
    // flip vertically so image matches OpenGL coordinate system
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path, &w, &h, &comp, STBI_rgb_alpha);
    if (!data) {
        fprintf(stderr, "stbi_load failed for '%s' (reason: %s)\n", path, stbi_failure_reason());
        return 0;
    }

    printf("Loaded image '%s' (%d x %d), components(original)=%d\n", path, w, h, comp);

    GLuint tid = 0;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);

    // Simple filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clamp to edge to avoid wrap seams
#ifdef GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif

    // Upload RGBA data to GL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    printf("Texture uploaded to GPU, id=%u\n", (unsigned)tid);
    return tid;
}

// Try a couple of locations to find the image: IMG_PATH (if set) then fallback name in CWD.
GLuint tryLoadTexture(const char* configuredPath) {
    // 1) If configuredPath provided, try it first.
    if (configuredPath && configuredPath[0] != '\0') {
        printf("Attempting to load texture from configured path: '%s'\n", configuredPath);
        GLuint t = loadTextureFromFile(configuredPath);
        if (t != 0) return t;
    }

    // 2) Try the fallback name in current working directory
    char cwd[1024] = {0};
    if (GetCurrentDir(cwd, sizeof(cwd))) {
        printf("Current working directory: %s\n", cwd);
    } else {
        printf("Could not get current working directory.\n");
    }

    printf("Attempting to load texture from working directory: '%s'\n", FALLBACK_NAME);
    GLuint t = loadTextureFromFile(FALLBACK_NAME);
    if (t != 0) return t;

    // 3) As a final attempt, try "../" (project root) + fallback (useful if exe is in bin/Debug)
    char parentPath[1100];
    if (GetCurrentDir(cwd, sizeof(cwd))) {
        snprintf(parentPath, sizeof(parentPath), "%s/%s", cwd, FALLBACK_NAME);
        printf("Attempting to load texture from: %s\n", parentPath);
        t = loadTextureFromFile(parentPath);
        if (t != 0) return t;

        // try one level up
        snprintf(parentPath, sizeof(parentPath), "%s/..%s", cwd, FALLBACK_NAME);
        // but better to build proper path - try "../menu_background.png"
        snprintf(parentPath, sizeof(parentPath), "%s/../%s", cwd, FALLBACK_NAME);
        printf("Attempting to load texture from: %s\n", parentPath);
        t = loadTextureFromFile(parentPath);
        if (t != 0) return t;
    }

    // failed
    return 0;
}

void initGL() {
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup orthographic projection matching window pixels
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Try load texture from configured path or fallback
    texID = tryLoadTexture(IMG_PATH);
    if (texID == 0) {
        fprintf(stderr, "ERROR: failed to find or load '%s' or '%s' in working dir. Please copy image next to exe.\n",
                (IMG_PATH && IMG_PATH[0] ? IMG_PATH : "(none)"), FALLBACK_NAME);
    } else {
        printf("Background texture ready (id=%u)\n", (unsigned)texID);
    }
}

void drawFullScreenTexture(GLuint t) {
    if (t == 0) return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, t);
    glColor3f(1,1,1);

    // With stbi_set_flip_vertically_on_load(true) above, use normal texcoords
    glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
      glTexCoord2f(1.0f, 0.0f); glVertex2f((float)WIN_W, 0.0f);
      glTexCoord2f(1.0f, 1.0f); glVertex2f((float)WIN_W, (float)WIN_H);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, (float)WIN_H);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (texID != 0) {
        drawFullScreenTexture(texID);
    } else {
        // fallback colored background so issue is obvious
        glColor3f(1, 0.6f, 0.6f);
        glBegin(GL_QUADS);
          glVertex2f(0,0); glVertex2f(WIN_W,0); glVertex2f(WIN_W, WIN_H); glVertex2f(0, WIN_H);
        glEnd();
    }

    // status text
    glColor3f(0,0,0);
    glRasterPos2f(10, 10);
    const char* msg = (texID != 0) ? "Texture loaded - OK" : "Texture not loaded - check path/working dir";
    for (const char* p = msg; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    // keep fixed logical resolution for simplicity
    glutReshapeWindow(WIN_W, WIN_H);
}

void keyboard(unsigned char k, int x, int y) {
    (void)x; (void)y;
    if (k == 27) exit(0);
}

int main(int argc, char** argv) {
    printf("Starting test_bg. Configured IMG_PATH: '%s'\n", (IMG_PATH && IMG_PATH[0]) ? IMG_PATH : "(none)");

    // Print CWD for debugging
    char cwd[1024];
    if (GetCurrentDir(cwd, sizeof(cwd))) {
        printf("Current working directory: %s\n", cwd);
    } else {
        printf("Could not get current working directory.\n");
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("test_bg - texture load test");
    initGL();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
