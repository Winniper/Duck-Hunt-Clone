#define _USE_MATH_DEFINES
#include <GL/glut.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <iostream>
#include <string>
#include <cmath>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Game constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int MAX_DUCKS = 3;
const float DUCK_SPEED = 3.0f;
const int DUCK_SIZE = 30;
const int SHOT_RADIUS = 20;
const int GAME_DURATION = 60;
const int CROSSHAIR_SIZE = 15;
const int SHOTS_PER_ROUND = 10;
const int BASE_POINTS = 500;
const int BONUS_POINTS = 500;
const float MAX_BONUS_TIME = 2.0f;

// Game state
int score = 0;
int timeRemaining = GAME_DURATION;
bool gameOver = false;
int lastTime = 0;
int missedShots = 0;
int totalShots = 0;
int mouseX = WINDOW_WIDTH / 2;
int mouseY = WINDOW_HEIGHT / 2;
int shotsRemaining = SHOTS_PER_ROUND;
float duckSpawnTime = 0.0f;
bool roundOver = false;

// Duck struct
struct Duck {
    float x, y;
    float dx, dy;
    bool active;
    int color;
    float wingAngle;
    float wingDir;
    int bodyColor;
};

struct FloatingText {
    float x, y;
    float alpha;
    float speed;
    std::string text;
};

std::vector<Duck> ducks;
std::vector<FloatingText> floatingTexts;

const float duckColors[][3] = {
    {1.0f, 1.0f, 0.0f}, // Yellow duck
    {0.2f, 0.8f, 0.2f}  // Green duck (wings, head)
};

const float duckBodyColors[][3] = {
    {1.0f, 1.0f, 0.0f}, // Yellow duck body 
    {1.0f, 1.0f, 1.0f}  // White body for green duck
};

void display();
void reshape(int w, int h);
void timer(int value);
void mouseClick(int button, int state, int x, int y);
void passiveMouseMotion(int x, int y);
void initGame();
void spawnDuck();
void drawDuck(const Duck& duck);
void drawHUD();
void drawBackground();
void drawCrosshair();
void updateAndDrawFloatingTexts();
void addFloatingText(float x, float y, int points);
int getDigitCount(int number);

int main(int argc, char** argv) {
    srand(static_cast<unsigned int>(time(nullptr)));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Duck Hunt Clone");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(1000, timer, 0);
    glutMouseFunc(mouseClick);
    glutPassiveMotionFunc(passiveMouseMotion);
    glutSetCursor(GLUT_CURSOR_NONE);

    initGame();
    glutMainLoop();
    return 0;
}

int getDigitCount(int number) {
    if (number == 0) return 1;
    int count = 0;
    while (number > 0) {
        number /= 10;
        count++;
    }
    return count;
}

void passiveMouseMotion(int x, int y) {
    mouseX = x;
    mouseY = WINDOW_HEIGHT - y;
    glutPostRedisplay();
}

void drawCrosshair() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);

    glBegin(GL_LINES);
    glVertex2f(mouseX - CROSSHAIR_SIZE, mouseY);
    glVertex2f(mouseX + CROSSHAIR_SIZE, mouseY);
    glVertex2f(mouseX, mouseY - CROSSHAIR_SIZE);
    glVertex2f(mouseX, mouseY + CROSSHAIR_SIZE);
    glEnd();

    glBegin(GL_LINE_LOOP);
    float radius = CROSSHAIR_SIZE / 3.0f;
    for (int i = 0; i < 16; i++) {
        float angle = 2.0f * M_PI * i / 16;
        glVertex2f(mouseX + radius * cos(angle), mouseY + radius * sin(angle));
    }
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void initGame() {
    ducks.clear();
    floatingTexts.clear();
    score = 0;
    timeRemaining = GAME_DURATION;
    gameOver = false;
    missedShots = 0;
    totalShots = 0;
    shotsRemaining = SHOTS_PER_ROUND;
    roundOver = false;
    lastTime = glutGet(GLUT_ELAPSED_TIME);

    for (int i = 0; i < MAX_DUCKS; ++i) {
        spawnDuck();
    }
}

void spawnDuck() {
    Duck duck;
    duck.active = true;
    duck.color = rand() % 2;
    duck.bodyColor = duck.color; // Default to matching body color

    // For green ducks (color index 1), use the white body color
    if (duck.color == 1) {
        duck.bodyColor = 1; // Use white body for green ducks
    }

    if (rand() % 2 == 0) {
        duck.x = -DUCK_SIZE;
        duck.dx = DUCK_SPEED * (0.5f + static_cast<float>(rand()) / RAND_MAX);
    }
    else {
        duck.x = WINDOW_WIDTH + DUCK_SIZE;
        duck.dx = -DUCK_SPEED * (0.5f + static_cast<float>(rand()) / RAND_MAX);
    }

    duck.y = WINDOW_HEIGHT / 2 + (WINDOW_HEIGHT / 3) * static_cast<float>(rand()) / RAND_MAX;
    duck.dy = DUCK_SPEED * 0.5f * (static_cast<float>(rand()) / RAND_MAX - 0.5f);
    duck.wingAngle = DUCK_SIZE * 0.8f;
    duck.wingDir = -1.0f;

    if (ducks.empty()) {
        duckSpawnTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    }

    ducks.push_back(duck);
}

void drawDuck(const Duck& duck) {
    glPushMatrix();
    glTranslatef(duck.x, duck.y, 0.0f);

    if (duck.dx < 0) {
        glScalef(-1.0f, 1.0f, 1.0f);
    }

    // Draw the beak with the main color
    glColor3fv(duckColors[duck.color]);
    glBegin(GL_TRIANGLES);
    glVertex2f(-DUCK_SIZE * 0.9f, -DUCK_SIZE * 0.2f);
    glVertex2f(-DUCK_SIZE * 1.2f, 0.0f);
    glVertex2f(-DUCK_SIZE * 0.9f, DUCK_SIZE * 0.2f);
    glEnd();

    // Draw the body with the body color
    // For green ducks, this will be white
    glColor3fv(duckBodyColors[duck.bodyColor]);
    glBegin(GL_POLYGON);
    float bodyRadius = DUCK_SIZE * 0.8f;
    float bodyHeight = DUCK_SIZE * 0.6f;
    for (int i = 0; i < 20; i++) {
        float angle = 2.0f * M_PI * i / 20;
        glVertex2f(bodyRadius * 0.8f * cos(angle), bodyHeight * sin(angle));
    }
    glEnd();

    // Draw the neck with the body color too
    glBegin(GL_QUADS);
    glVertex2f(DUCK_SIZE * 0.5f, -DUCK_SIZE * 0.1f);
    glVertex2f(DUCK_SIZE * 0.7f, -DUCK_SIZE * 0.1f);
    glVertex2f(DUCK_SIZE * 0.7f, DUCK_SIZE * 0.3f);
    glVertex2f(DUCK_SIZE * 0.5f, DUCK_SIZE * 0.3f);
    glEnd();

    // Head uses the main color
    glColor3fv(duckColors[duck.color]);
    glBegin(GL_POLYGON);
    float headX = DUCK_SIZE * 0.85f;
    float headY = DUCK_SIZE * 0.4f;
    float headRadius = DUCK_SIZE * 0.3f;
    for (int i = 0; i < 16; i++) {
        float angle = 2.0f * M_PI * i / 16;
        glVertex2f(headX + headRadius * cos(angle), headY + headRadius * sin(angle));
    }
    glEnd();

    glColor3f(1.0f, 0.5f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(DUCK_SIZE * 1.1f, DUCK_SIZE * 0.3f);
    glVertex2f(DUCK_SIZE * 1.5f, DUCK_SIZE * 0.4f);
    glVertex2f(DUCK_SIZE * 1.1f, DUCK_SIZE * 0.5f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POLYGON);
    float eyeX = DUCK_SIZE * 1.0f;
    float eyeY = DUCK_SIZE * 0.5f;
    float eyeRadius = DUCK_SIZE * 0.1f;
    for (int i = 0; i < 12; i++) {
        float angle = 2.0f * M_PI * i / 12;
        glVertex2f(eyeX + eyeRadius * cos(angle), eyeY + eyeRadius * sin(angle));
    }
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_POLYGON);
    float pupilRadius = DUCK_SIZE * 0.05f;
    for (int i = 0; i < 8; i++) {
        float angle = 2.0f * M_PI * i / 8;
        glVertex2f(eyeX + pupilRadius * cos(angle), eyeY + pupilRadius * sin(angle));
    }
    glEnd();

    // Wings use the main color
    glColor3fv(duckColors[duck.color]);
    glBegin(GL_POLYGON);
    float wingAngle = duck.wingAngle * 0.7f;
    float wingTipX = -DUCK_SIZE * 0.2f;
    float wingTipY = DUCK_SIZE * 0.3f + wingAngle;
    glVertex2f(-DUCK_SIZE * 0.1f, DUCK_SIZE * 0.1f);
    glVertex2f(-DUCK_SIZE * 0.5f, DUCK_SIZE * 0.2f + wingAngle * 0.5f);
    glVertex2f(wingTipX, wingTipY);
    glVertex2f(-DUCK_SIZE * 0.3f, DUCK_SIZE * 0.1f + wingAngle * 0.3f);
    glVertex2f(DUCK_SIZE * 0.1f, DUCK_SIZE * 0.1f);
    glEnd();

    glBegin(GL_POLYGON);
    float wingAngle2 = -wingAngle * 0.8f;
    float wingTipX2 = -DUCK_SIZE * 0.2f;
    float wingTipY2 = -DUCK_SIZE * 0.1f + wingAngle2;
    glVertex2f(-DUCK_SIZE * 0.1f, -DUCK_SIZE * 0.1f);
    glVertex2f(-DUCK_SIZE * 0.4f, -DUCK_SIZE * 0.1f + wingAngle2 * 0.5f);
    glVertex2f(wingTipX2, wingTipY2);
    glVertex2f(-DUCK_SIZE * 0.3f, -DUCK_SIZE * 0.1f + wingAngle2 * 0.3f);
    glVertex2f(DUCK_SIZE * 0.1f, -DUCK_SIZE * 0.1f);
    glEnd();

    if (fabs(duck.dy) < fabs(duck.dx) * 0.5f) {
        glColor3f(1.0f, 0.5f, 0.0f);
        glBegin(GL_TRIANGLES);
        glVertex2f(-DUCK_SIZE * 0.1f, -DUCK_SIZE * 0.5f);
        glVertex2f(-DUCK_SIZE * 0.3f, -DUCK_SIZE * 0.7f);
        glVertex2f(DUCK_SIZE * 0.1f, -DUCK_SIZE * 0.7f);
        glVertex2f(DUCK_SIZE * 0.3f, -DUCK_SIZE * 0.5f);
        glVertex2f(DUCK_SIZE * 0.1f, -DUCK_SIZE * 0.7f);
        glVertex2f(DUCK_SIZE * 0.5f, -DUCK_SIZE * 0.7f);
        glEnd();
    }

    glPopMatrix();
}

void drawBackground() {
    // Sky
    glBegin(GL_QUADS);
    glColor3f(0.5f, 0.8f, 1.0f);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
    glVertex2f(0, WINDOW_HEIGHT);
    glEnd();

    // Sky grid lines
    glColor3f(0.6f, 0.85f, 1.0f);
    glLineWidth(0.5f);
    glBegin(GL_LINES);
    for (int i = 0; i < WINDOW_WIDTH; i += 40) {
        glVertex2f(i, WINDOW_HEIGHT / 5);
        glVertex2f(i, WINDOW_HEIGHT);
    }
    for (int i = WINDOW_HEIGHT / 5; i < WINDOW_HEIGHT; i += 40) {
        glVertex2f(0, i);
        glVertex2f(WINDOW_WIDTH, i);
    }
    glEnd();

    // Sand/ground
    glBegin(GL_QUADS);
    glColor3f(0.9f, 0.9f, 0.0f);
    glVertex2f(0, WINDOW_HEIGHT / 5);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT / 5);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT / 3.5);
    glVertex2f(0, WINDOW_HEIGHT / 3.5);
    glEnd();

    // Grass tufts on sand
    glColor3f(0.7f, 0.8f, 0.0f);
    for (int i = 10; i < WINDOW_WIDTH; i += 30) {
        glBegin(GL_TRIANGLES);
        glVertex2f(i, WINDOW_HEIGHT / 3.5);
        glVertex2f(i + 5, WINDOW_HEIGHT / 3.2);
        glVertex2f(i + 10, WINDOW_HEIGHT / 3.5);
        glEnd();
    }

    // Dirt/ground
    glBegin(GL_QUADS);
    glColor3f(0.6f, 0.3f, 0.0f);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT / 5);
    glVertex2f(0, WINDOW_HEIGHT / 5);
    glEnd();

    // Tree trunk - Draw as a single rectangle that extends up to the leaves
    glColor3f(0.8f, 0.2f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(120, WINDOW_HEIGHT / 3.5);  // Start at ground level
    glVertex2f(140, WINDOW_HEIGHT / 3.5);
    glVertex2f(140, WINDOW_HEIGHT / 1.6);  // Extend higher to connect with leaves
    glVertex2f(120, WINDOW_HEIGHT / 1.6);
    glEnd();

    // Tree branch
    glBegin(GL_QUADS);
    glVertex2f(140, WINDOW_HEIGHT / 1.7);
    glVertex2f(180, WINDOW_HEIGHT / 1.7);
    glVertex2f(180, WINDOW_HEIGHT / 1.6);
    glVertex2f(140, WINDOW_HEIGHT / 1.6);
    glEnd();

    // Tree leaves (four green circles)
    glColor3f(0.7f, 0.9f, 0.0f);
    for (int cx = 0; cx < 2; cx++) {
        for (int cy = 0; cy < 2; cy++) {
            glBegin(GL_POLYGON);
            float centerX = 130 + cx * 50;
            float centerY = WINDOW_HEIGHT / 1.6 + cy * 60;  // Adjust to connect with trunk
            float radius = 30;
            for (int i = 0; i < 12; i++) {
                float angle = 2.0f * M_PI * i / 12;
                glVertex2f(centerX + radius * cos(angle), centerY + radius * sin(angle));
            }
            glEnd();
        }
    }

    // Bush
    glColor3f(0.7f, 0.9f, 0.0f);
    glBegin(GL_POLYGON);
    float bushX = WINDOW_WIDTH - 100;
    float bushY = WINDOW_HEIGHT / 3.5 + 30;
    float bushRadius = 40;
    for (int i = 0; i < 12; i++) {
        float angle = 2.0f * M_PI * i / 12;
        glVertex2f(bushX + bushRadius * cos(angle), bushY + bushRadius * sin(angle));
    }
    glEnd();
}

void updateAndDrawFloatingTexts() {
    for (auto it = floatingTexts.begin(); it != floatingTexts.end();) {
        it->y += it->speed;
        it->alpha -= 0.02f;

        if (it->alpha <= 0.0f) {
            it = floatingTexts.erase(it);
        }
        else {
            ++it;
        }
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    for (const auto& ft : floatingTexts) {
        glColor4f(1.0f, 1.0f, 0.0f, ft.alpha);
        glRasterPos2f(ft.x, ft.y);
        for (char c : ft.text) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
        }
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void addFloatingText(float x, float y, int points) {
    FloatingText ft;
    ft.x = x;
    ft.y = y;
    ft.alpha = 1.0f;
    ft.speed = 1.0f;
    ft.text = "+" + std::to_string(points);
    floatingTexts.push_back(ft);
}

void drawHUD() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(0.6f, 0.3f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT / 10);
    glVertex2f(0, WINDOW_HEIGHT / 10);
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(10, WINDOW_HEIGHT / 60);
    glVertex2f(120, WINDOW_HEIGHT / 60);
    glVertex2f(120, WINDOW_HEIGHT * 0.09);
    glVertex2f(10, WINDOW_HEIGHT * 0.09);
    glEnd();

    glColor3f(1.0f, 0.6f, 0.0f);
    glRasterPos2f(15, WINDOW_HEIGHT / 20);
    std::string shotText = "SHOT";
    for (char c : shotText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    for (int i = 0; i < 3 - missedShots % 3; i++) {
        glColor3f(0.7f, 0.5f, 0.1f);
        glBegin(GL_QUADS);
        glVertex2f(55 + i * 20, WINDOW_HEIGHT / 40);
        glVertex2f(70 + i * 20, WINDOW_HEIGHT / 40);
        glVertex2f(70 + i * 20, WINDOW_HEIGHT / 18);
        glVertex2f(55 + i * 20, WINDOW_HEIGHT / 18);
        glEnd();

        glColor3f(0.0f, 0.5f, 1.0f);
        glBegin(GL_QUADS);
        glVertex2f(55 + i * 20, WINDOW_HEIGHT / 18);
        glVertex2f(70 + i * 20, WINDOW_HEIGHT / 18);
        glVertex2f(70 + i * 20, WINDOW_HEIGHT / 13);
        glVertex2f(55 + i * 20, WINDOW_HEIGHT / 13);
        glEnd();
    }

    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(130, WINDOW_HEIGHT / 60);
    glVertex2f(WINDOW_WIDTH - 160, WINDOW_HEIGHT / 60);
    glVertex2f(WINDOW_WIDTH - 160, WINDOW_HEIGHT * 0.09);
    glVertex2f(130, WINDOW_HEIGHT * 0.09);
    glEnd();

    glColor3f(1.0f, 0.9f, 0.0f);
    glRasterPos2f(140, WINDOW_HEIGHT / 20);
    std::string hitText = "HIT";
    for (char c : hitText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    int hitMarkWidth = (WINDOW_WIDTH - 350) / 10;
    for (int i = 0; i < score % 10; i++) {
        glBegin(GL_LINE_STRIP);
        glVertex2f(180 + i * hitMarkWidth, WINDOW_HEIGHT / 45);
        glVertex2f(180 + i * hitMarkWidth + hitMarkWidth / 2, WINDOW_HEIGHT / 15);
        glVertex2f(180 + i * hitMarkWidth + hitMarkWidth, WINDOW_HEIGHT / 45);
        glEnd();
    }

    // New score display similar to image
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(WINDOW_WIDTH - 150, WINDOW_HEIGHT / 60);
    glVertex2f(WINDOW_WIDTH - 10, WINDOW_HEIGHT / 60);
    glVertex2f(WINDOW_WIDTH - 10, WINDOW_HEIGHT * 0.09);
    glVertex2f(WINDOW_WIDTH - 150, WINDOW_HEIGHT * 0.09);
    glEnd();

    // Dark green background for score display
    glColor3f(0.0f, 0.2f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(WINDOW_WIDTH - 145, WINDOW_HEIGHT / 40);
    glVertex2f(WINDOW_WIDTH - 15, WINDOW_HEIGHT / 40);
    glVertex2f(WINDOW_WIDTH - 15, WINDOW_HEIGHT * 0.08);
    glVertex2f(WINDOW_WIDTH - 145, WINDOW_HEIGHT * 0.08);
    glEnd();

    // Score text
    std::string scoreStr = std::to_string(score);
    while (scoreStr.length() < 6) {
        scoreStr = "0" + scoreStr;  // Pad with leading zeros
    }

    glColor3f(0.8f, 0.8f, 0.0f);  // Yellow/gold color for score
    glRasterPos2f(WINDOW_WIDTH - 140, WINDOW_HEIGHT / 20);
    std::string scoreText = "SCORE";
    for (char c : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    // Display score digits
    glRasterPos2f(WINDOW_WIDTH - 95, WINDOW_HEIGHT / 20);
    for (char c : scoreStr) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
    }

    // Add shots remaining display
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(15, WINDOW_HEIGHT / 20 + 20);
    std::string shotsText = "SHOTS: " + std::to_string(shotsRemaining) + "/" + std::to_string(SHOTS_PER_ROUND);
    for (char c : shotsText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    if (gameOver || roundOver) {
        std::string endMessage = gameOver ? "GAME OVER!" : "ROUND OVER!";
        std::string scoreStr = "Final Score: " + std::to_string(score);
        std::string restartStr = "Click to Restart";

        glColor3f(1.0f, 0.0f, 0.0f);
        glRasterPos2f(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2);
        for (char c : endMessage) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }

        glRasterPos2f(WINDOW_WIDTH / 2 - 60, WINDOW_HEIGHT / 2 - 30);
        for (char c : scoreStr) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }

        glRasterPos2f(WINDOW_WIDTH / 2 - 60, WINDOW_HEIGHT / 2 - 60);
        for (char c : restartStr) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawBackground();

    for (const Duck& duck : ducks) {
        if (duck.active) {
            drawDuck(duck);
        }
    }

    updateAndDrawFloatingTexts();
    drawHUD();
    drawCrosshair();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
}

void timer(int value) {
    if (!gameOver && !roundOver) {
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        int deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        timeRemaining -= deltaTime / 1000;
        if (timeRemaining <= 0) {
            timeRemaining = 0;
            gameOver = true;
        }

        for (auto& duck : ducks) {
            if (duck.active) {
                duck.x += duck.dx;
                duck.y += duck.dy;

                duck.wingAngle += duck.wingDir * 0.2f;
                if (duck.wingAngle < -DUCK_SIZE * 0.8f || duck.wingAngle > DUCK_SIZE * 0.8f) {
                    duck.wingDir *= -1.0f;
                }

                float minHeight = WINDOW_HEIGHT / 3.5 + DUCK_SIZE * 2;
                if (duck.y < minHeight) {
                    duck.y = minHeight;
                    duck.dy = fabs(duck.dy);
                }

                if (duck.y > WINDOW_HEIGHT - DUCK_SIZE) {
                    duck.y = WINDOW_HEIGHT - DUCK_SIZE;
                    duck.dy = -fabs(duck.dy);
                }

                if (duck.x < -DUCK_SIZE * 2 || duck.x > WINDOW_WIDTH + DUCK_SIZE * 2) {
                    duck.active = false;
                }
            }
        }

        int activeDucks = 0;
        for (const auto& duck : ducks) {
            if (duck.active) {
                activeDucks++;
            }
        }

        if (activeDucks < MAX_DUCKS) {
            spawnDuck();
        }

        for (auto it = ducks.begin(); it != ducks.end();) {
            if (!it->active) {
                it = ducks.erase(it);
            }
            else {
                ++it;
            }
        }

        glutPostRedisplay();
    }

    glutTimerFunc(16, timer, 0);
}

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (gameOver || roundOver) {
            initGame();
        }
        else {
            if (shotsRemaining <= 0) {
                roundOver = true;
                return;
            }

            int viewportY = WINDOW_HEIGHT - y;
            shotsRemaining--;
            totalShots++;

            bool hitDuck = false;
            for (auto& duck : ducks) {
                if (duck.active) {
                    float dx = duck.x - x;
                    float dy = duck.y - viewportY;
                    float distance = sqrt(dx * dx + dy * dy);

                    if (distance < SHOT_RADIUS + DUCK_SIZE) {
                        float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
                        float timeSinceSpawn = currentTime - duckSpawnTime;

                        int bonusPoints = 0;
                        if (timeSinceSpawn <= MAX_BONUS_TIME) {
                            bonusPoints = BONUS_POINTS;
                        }
                        else if (timeSinceSpawn <= MAX_BONUS_TIME * 2) {
                            float bonusFactor = 1.0f - (timeSinceSpawn - MAX_BONUS_TIME) / MAX_BONUS_TIME;
                            bonusPoints = static_cast<int>(BONUS_POINTS * bonusFactor);
                        }

                        int pointsEarned = BASE_POINTS + bonusPoints;
                        score += pointsEarned;

                        addFloatingText(duck.x, duck.y, pointsEarned);

                        duck.active = false;
                        hitDuck = true;

                        duckSpawnTime = currentTime;
                        break;
                    }
                }
            }

            if (!hitDuck) {
                missedShots++;
            }

            if (shotsRemaining <= 0) {
                roundOver = true;
            }
        }
    }
}