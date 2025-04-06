/*
BSD 3-Clause License

Copyright (c) 2025, BISMAYA JYOTI DALEI

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "include/raylib.h"
#include "include/raymath.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

// Game constants
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800
#define PADDLE_WIDTH 25
#define PADDLE_HEIGHT 200
#define BALL_RADIUS 20
#define BALL_INITIAL_SPEED 8.0f
#define PADDLE_SPEED 12.0f
#define MAX_BALL_SPEED 15.0f
#define SPEED_INCREMENT 0.2f
#define WALL_BOUNCE_BUFFER 2.0f  // Prevent ball from sticking to walls

#define COLOR_BACKGROUND        (Color){ 16, 24, 32, 255 }
#define COLOR_ACCENT            (Color){ 65, 105, 225, 255 }
#define COLOR_PLAYER_ONE        (Color){ 0, 180, 255, 255 }   // Bright blue
#define COLOR_PLAYER_TWO        (Color){ 0, 220, 120, 255 }   // Teal green
#define COLOR_AI                (Color){ 255, 100, 100, 255 } // Coral red
#define COLOR_BALL              (Color){ 255, 255, 255, 255 }
#define COLOR_GLOW              (Color){ 120, 120, 255, 40 }  // For glow effects

// Game states
typedef enum {
    STATE_SPLASH,
    STATE_MODE_SELECT,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} GameState;

// Game modes
typedef enum {
    MODE_AI,        // Player vs AI
    MODE_MULTIPLAYER // Player vs Player
} GameMode;

// Ball structure
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float radius;
    Color color;
} Ball;

// Paddle structure
typedef struct {
    Rectangle rect;
    float speed;
    Color color;
} Paddle;

// Define a particle structure
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifeTime;
    float maxLifeTime;
    Color color;
    float size;
} Particle;

#define MAX_PARTICLES 100

// Game structure
typedef struct {
    GameState state;
    GameMode mode;
    Ball ball;
    Paddle playerPaddle; // First paddle (Player 1)
    Paddle aiPaddle;  // Second paddle (AI or Player 2)
    int playerScore;
    int aiScore;
    int winScore;
    Font gameFont;         // Custom font for the game
    Sound paddleHitSound;  // Sound for paddle hits
    Sound scoreSound;      // Sound for scoring
    bool fullscreen;       // Track fullscreen state
    float ballSpeedMultiplier; // Speed multiplier for ball (0.5 to 2.0)
    // Animation and effects
    float scoreAnimScale;    // For score change animation
    float lastScoreTime;     // Track when score last changed
    float screenShake;       // Screen shake effect amount
    Vector2 shakeOffset;     // Current screen shake offset
    // Particle system
    Particle particles[MAX_PARTICLES];
    int activeParticles;
} Game;

// Function prototypes
void InitGame(Game *game, GameMode mode);
void UpdateGame(Game *game);
void DrawGame(Game *game);
void ResetBall(Game *game, bool serverIsPlayer);
bool CheckPaddleCollision(Ball *ball, Paddle *paddle, Game *game);
void UpdateAI(Paddle *aiPaddle, Ball *ball, Game *game);
void DrawSplashScreen(Font font);
void UpdateSplashScreen(Game *game);
void DrawModeSelect(Font font, Game *game);
void UpdateModeSelect(Game *game);
void CleanupGame(Game *game);
void ToggleGameFullscreen(Game *game);
void DrawRoundedRectangleWithGlow(Rectangle rec, float roundness, int segments, Color color);
void DrawBallWithGlow(Vector2 center, float radius, Color color);
void CreateParticleEffect(Game *game, Vector2 position, Color color, int count);
void UpdateAndDrawParticles(Game *game);

int main(void) {
    // Initialize window and audio
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pong Game - By Bismaya");
    InitAudioDevice();
    SetTargetFPS(60);

    // Load splash screen music
    Music splashMusic = LoadMusicStream("assets/audio/Onyx - Ataraxia.mp3");
    SetMusicVolume(splashMusic, 0.7f);
    PlayMusicStream(splashMusic);

    // Load custom font
    Font gameFont = LoadFont("assets/fonts/Exo2-SemiBold.ttf");
    if (gameFont.texture.id == 0) {
        // Fallback to default if custom font fails to load
        gameFont = GetFontDefault();
    }

    // Initialize game
    Game game = {0};  // Initialize all fields to zero/NULL
    game.state = STATE_SPLASH;
    game.gameFont = gameFont;
    game.fullscreen = false;
    game.ballSpeedMultiplier = 1.0f; // Default speed multiplier
    
    // Load sound effects
    game.paddleHitSound = LoadSound("assets/audio/paddle_hit.mp3");
    game.scoreSound = LoadSound("assets/audio/score.mp3");

    // Main game loop
    while (!WindowShouldClose()) {
        UpdateMusicStream(splashMusic);

        // Check for fullscreen toggle
        if (IsKeyPressed(KEY_F)) {
            ToggleGameFullscreen(&game);
        }

        switch (game.state) {
            case STATE_SPLASH:
                UpdateSplashScreen(&game);
                DrawSplashScreen(game.gameFont);
                break;
            case STATE_MODE_SELECT:
                UpdateModeSelect(&game);
                DrawModeSelect(game.gameFont, &game);
                break;
            default:
                UpdateGame(&game);
                DrawGame(&game);
                break;
        }
    }

    // Cleanup to prevent memory leaks
    StopMusicStream(splashMusic);
    UnloadMusicStream(splashMusic);
    CleanupGame(&game);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

void ToggleGameFullscreen(Game *game) {
    if (!game->fullscreen) {
        // Save current window size before going to fullscreen
        SetWindowState(FLAG_FULLSCREEN_MODE);
        game->fullscreen = true;
    } else {
        // Return to windowed mode
        ClearWindowState(FLAG_FULLSCREEN_MODE);
        game->fullscreen = false;
    }
}

void CleanupGame(Game *game) {
    // Unload resources to prevent memory leaks
    UnloadFont(game->gameFont);
    UnloadSound(game->paddleHitSound);
    UnloadSound(game->scoreSound);
}

void DrawSplashScreen(Font font) {
    BeginDrawing();
    
    // Modern gradient background
    Color topColor = (Color){ 12, 20, 28, 255 };
    Color bottomColor = COLOR_BACKGROUND;
    
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float factor = (float)y / SCREEN_HEIGHT;
        Color lineColor = {
            (unsigned char)Lerp(topColor.r, bottomColor.r, factor),
            (unsigned char)Lerp(topColor.g, bottomColor.g, factor),
            (unsigned char)Lerp(topColor.b, bottomColor.b, factor),
            255
        };
        DrawLine(0, y, SCREEN_WIDTH, y, lineColor);
    }

    // Draw animated particles
    static float particleTime = 0;
    particleTime += GetFrameTime();
    
    // Draw particles
    for (int i = 0; i < 100; i++) {
        float speed = (float)i / 30.0f;
        float size = sinf(particleTime * speed) * 4.0f + 2.0f;
        float x = sinf(particleTime * 0.5f + i * 0.987f) * SCREEN_WIDTH * 0.5f + SCREEN_WIDTH * 0.5f;
        float y = cosf(particleTime * 0.37f + i * 1.153f) * SCREEN_HEIGHT * 0.5f + SCREEN_HEIGHT * 0.5f;
        float alpha = (sinf(particleTime + i) * 0.5f + 0.5f) * 0.5f;
        
        DrawCircle(x, y, size, ColorAlpha(COLOR_ACCENT, alpha));
    }
    
    // Draw animated pong elements in background
    static float ballPosX = 300;
    static float ballPosY = 400;
    static float ballVelX = 3;
    static float ballVelY = 2;
    
    ballPosX += ballVelX;
    ballPosY += ballVelY;
    
    if (ballPosX < 50 || ballPosX > SCREEN_WIDTH - 50) ballVelX *= -1;
    if (ballPosY < 50 || ballPosY > SCREEN_HEIGHT - 50) ballVelY *= -1;
    
    // Enhanced glow effects for background elements
    float glowSize = sinf(GetTime() * 2) * 5 + 15;
    
    // Draw background paddles and ball with dynamic glow
    DrawCircle(ballPosX, ballPosY, 15 + glowSize, ColorAlpha(WHITE, 0.1f));
    DrawCircle(ballPosX, ballPosY, 15, ColorAlpha(WHITE, 0.4f));
    
    DrawRectangleRounded(
        (Rectangle){ 30, ballPosY - 50, 15, 100 },
        0.3f,
        6,
        ColorAlpha(COLOR_PLAYER_ONE, 0.4f)
    );
    
    DrawRectangleRounded(
        (Rectangle){ SCREEN_WIDTH - 45, ballPosY - 50, 15, 100 },
        0.3f,
        6,
        ColorAlpha(COLOR_AI, 0.4f)
    );
    
    // Title with animated effects
    const char* title = "PHANTOM PONG";
    float titleFontSize = 100;
    
    // Animate title position with smooth sine wave
    static float titleOffset = 0;
    titleOffset = sinf(GetTime() * 1.5f) * 8.0f;
    
    Vector2 titlePos = {
        SCREEN_WIDTH / 2 - MeasureTextEx(font, title, titleFontSize, 1).x / 2,
        SCREEN_HEIGHT / 2 - 120 + titleOffset
    };
    
    // Enhanced glow effect with multiple layers
    DrawTextEx(font, title, (Vector2){titlePos.x + 6, titlePos.y + 6}, titleFontSize, 1, ColorAlpha(COLOR_ACCENT, 0.2f));
    DrawTextEx(font, title, (Vector2){titlePos.x + 4, titlePos.y + 4}, titleFontSize, 1, ColorAlpha(COLOR_ACCENT, 0.4f));
    DrawTextEx(font, title, (Vector2){titlePos.x + 2, titlePos.y + 2}, titleFontSize, 1, ColorAlpha(COLOR_ACCENT, 0.6f));
    DrawTextEx(font, title, titlePos, titleFontSize, 1, WHITE);
    
    // Subtitle with fade effect
    const char* subtitle = "A Game By Bismaya";
    static float subtitleAlpha = 0;
    subtitleAlpha = sinf(GetTime() * 2) * 0.2f + 0.8f;
    
    Vector2 subtitlePos = {
        SCREEN_WIDTH / 2 - MeasureTextEx(font, subtitle, 30, 1).x / 2,
        SCREEN_HEIGHT / 2 - 20
    };
    DrawTextEx(font, subtitle, subtitlePos, 30, 1, ColorAlpha(COLOR_ACCENT, subtitleAlpha));
    
    // Start button with enhanced animation
    static float pulseSize = 0;
    static bool pulsing = true;
    
    if (pulsing) {
        pulseSize += 0.01f;
        if (pulseSize > 0.2f) pulsing = false;
    } else {
        pulseSize -= 0.01f;
        if (pulseSize < 0.0f) pulsing = true;
    }
    
    const char* startText = "Click to Start";
    float startFontSize = 40 + pulseSize * 15;
    Vector2 startPos = {
        SCREEN_WIDTH / 2 - MeasureTextEx(font, startText, startFontSize, 1).x / 2, 
        SCREEN_HEIGHT / 2 + 80
    };
    
    // Glowing effect for start text
    Color startColor = ColorAlpha(WHITE, 0.8f + pulseSize);
    DrawTextEx(font, startText, (Vector2){startPos.x + 3, startPos.y + 3}, startFontSize, 1, ColorAlpha(COLOR_ACCENT, 0.3f + pulseSize * 0.3f));
    DrawTextEx(font, startText, startPos, startFontSize, 1, startColor);
    
    // Modern fullscreen button
    const char* fullscreenText = "Press F for Fullscreen";
    Vector2 fullscreenPos = {
        SCREEN_WIDTH / 2 - MeasureTextEx(font, fullscreenText, 20, 1).x / 2,
        SCREEN_HEIGHT - 30
    };
    
    Rectangle fsRect = {
        fullscreenPos.x - 10,
        fullscreenPos.y - 5,
        MeasureTextEx(font, fullscreenText, 20, 1).x + 20,
        30
    };
    
    DrawRectangleRounded(fsRect, 0.5f, 8, ColorAlpha(BLACK, 0.3f));
    DrawRectangleRoundedLines(fsRect, 0.5f, 8, ColorAlpha(COLOR_ACCENT, 0.5f + sinf(GetTime() * 3) * 0.2f));
    DrawTextEx(font, fullscreenText, fullscreenPos, 20, 1, ColorAlpha(WHITE, 0.5f + sinf(GetTime() * 3) * 0.2f));

    EndDrawing();
}

void UpdateSplashScreen(Game *game) {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) {
        game->state = STATE_MODE_SELECT;  // Go to mode selection instead of directly to game
    }
}

void DrawModeSelect(Font font, Game *game) {
    BeginDrawing();
    
    // Enhanced animated gradient background
    Color topColor = (Color){ 12, 20, 28, 255 };
    Color bottomColor = COLOR_BACKGROUND;
    Color accentGlow = ColorAlpha(COLOR_ACCENT, 0.1f + sinf(GetTime() * 2) * 0.05f);
    
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float factor = (float)y / SCREEN_HEIGHT;
        Color lineColor = {
            (unsigned char)Lerp(topColor.r, bottomColor.r, factor),
            (unsigned char)Lerp(topColor.g, bottomColor.g, factor),
            (unsigned char)Lerp(topColor.b, bottomColor.b, factor),
            255
        };
        DrawLine(0, y, SCREEN_WIDTH, y, lineColor);
    }
    
    // Draw animated elements in background
    float time = GetTime();
    for (int i = 0; i < 40; i++) {
        float size = 3.0f + sinf(time * 0.5f + i * 0.2f) * 2.0f;
        float alpha = 0.1f + sinf(time * 0.3f + i * 0.7f) * 0.05f;
        DrawCircle(
            sinf(time * 0.1f + i * 1.1f) * SCREEN_WIDTH * 0.5f + SCREEN_WIDTH * 0.5f,
            cosf(time * 0.2f + i * 0.8f) * SCREEN_HEIGHT * 0.5f + SCREEN_HEIGHT * 0.5f,
            size,
            ColorAlpha(COLOR_ACCENT, alpha)
        );
    }

    // Animated title with floating effect
    float titleOffset = sinf(time * 1.5f) * 5.0f;
    Vector2 titlePos = {
        SCREEN_WIDTH / 2 - MeasureTextEx(font, "SELECT GAME MODE", 60, 1).x / 2,
        SCREEN_HEIGHT / 4 + titleOffset
    };
    
    // Draw title glow
    DrawTextEx(font, "SELECT GAME MODE", (Vector2){titlePos.x + 4, titlePos.y + 4}, 60, 1, ColorAlpha(COLOR_ACCENT, 0.4f));
    DrawTextEx(font, "SELECT GAME MODE", (Vector2){titlePos.x + 2, titlePos.y + 2}, 60, 1, ColorAlpha(COLOR_ACCENT, 0.6f));
    DrawTextEx(font, "SELECT GAME MODE", titlePos, 60, 1, WHITE);
    
    // Hover animations for mode options
    Vector2 mousePos = GetMousePosition();
    
    // Mode 1 animation
    const char* mode1Text = "1. Player vs AI";
    Vector2 mode1Pos = {
        SCREEN_WIDTH / 2 - MeasureTextEx(font, mode1Text, 40, 1).x / 2,
        SCREEN_HEIGHT / 2 - 20
    };
    
    Rectangle mode1Bounds = {
        mode1Pos.x - 20, mode1Pos.y - 10, 
        MeasureTextEx(font, mode1Text, 40, 1).x + 40, 60
    };
    
    bool mode1Hover = CheckCollisionPointRec(mousePos, mode1Bounds);
    float mode1Scale = mode1Hover ? 1.1f : 1.0f;
    Color mode1Color = mode1Hover ? WHITE : YELLOW;
    
    // Draw mode1 option with glow when hovered
    if (mode1Hover) {
        DrawRectangleRounded(mode1Bounds, 0.3f, 8, ColorAlpha(COLOR_ACCENT, 0.2f));
        DrawTextEx(font, mode1Text, 
                  (Vector2){mode1Pos.x - (mode1Scale-1.0f)*MeasureTextEx(font, mode1Text, 40, 1).x/2, mode1Pos.y}, 
                  40 * mode1Scale, 1, mode1Color);
    } else {
        DrawTextEx(font, mode1Text, mode1Pos, 40, 1, mode1Color);
    }
    
    // Mode 2 animation
    const char* mode2Text = "2. Player vs Player";
    Vector2 mode2Pos = {
        SCREEN_WIDTH / 2 - MeasureTextEx(font, mode2Text, 40, 1).x / 2,
        SCREEN_HEIGHT / 2 + 40
    };
    
    Rectangle mode2Bounds = {
        mode2Pos.x - 20, mode2Pos.y - 10, 
        MeasureTextEx(font, mode2Text, 40, 1).x + 40, 60
    };
    
    bool mode2Hover = CheckCollisionPointRec(mousePos, mode2Bounds);
    float mode2Scale = mode2Hover ? 1.1f : 1.0f;
    Color mode2Color = mode2Hover ? WHITE : YELLOW;
    
    // Draw mode2 option with glow when hovered
    if (mode2Hover) {
        DrawRectangleRounded(mode2Bounds, 0.3f, 8, ColorAlpha(COLOR_ACCENT, 0.2f));
        DrawTextEx(font, mode2Text, 
                  (Vector2){mode2Pos.x - (mode2Scale-1.0f)*MeasureTextEx(font, mode2Text, 40, 1).x/2, mode2Pos.y}, 
                  40 * mode2Scale, 1, mode2Color);
    } else {
        DrawTextEx(font, mode2Text, mode2Pos, 40, 1, mode2Color);
    }
    
    // Enhanced slider with animations
    Vector2 sliderLabelPos = {
        SCREEN_WIDTH / 2 - MeasureTextEx(font, "Ball Speed:", 30, 1).x - 50,
        SCREEN_HEIGHT * 3/4 - 50
    };
    DrawTextEx(font, "Ball Speed:", sliderLabelPos, 30, 1, WHITE);
    
    // Draw slider with glow effect
    Rectangle sliderBg = { 
        SCREEN_WIDTH / 2 - 150, 
        SCREEN_HEIGHT * 3/4, 
        300, 
        20 
    };
    
    // Animated slider background
    DrawRectangleRounded(sliderBg, 0.5, 8, DARKGRAY);
    
    // Draw filled portion of slider
    Rectangle filledSlider = {
        sliderBg.x,
        sliderBg.y,
        (sliderBg.width * (game->ballSpeedMultiplier - 0.5f) / 1.5f),
        sliderBg.height
    };
    DrawRectangleRounded(filledSlider, 0.5, 8, ColorAlpha(COLOR_ACCENT, 0.7f));
    
    // Enhanced slider knob with glow
    float knobPos = sliderBg.x + (sliderBg.width * (game->ballSpeedMultiplier - 0.5f) / 1.5f);
    float knobPulse = sinf(GetTime() * 4) * 2;
    
    DrawCircle(knobPos, sliderBg.y + sliderBg.height/2, 15 + knobPulse, ColorAlpha(COLOR_ACCENT, 0.3f));
    DrawCircle(knobPos, sliderBg.y + sliderBg.height/2, 10, WHITE);
    
    // Animated speed indicator
    char speedText[20];
    sprintf(speedText, "%.1fx", game->ballSpeedMultiplier);
    Vector2 speedTextPos = {
        // Inside the slider, centered
        // sliderBg.x + (sliderBg.width - MeasureTextEx(font, speedText, 25, 1).x) / 2,
        // sliderBg.y + (sliderBg.height - MeasureTextEx(font, speedText, 25, 1).y) / 2
        // In front of the Ball Speed label
        sliderLabelPos.x + MeasureTextEx(font, "Ball Speed:", 30, 1).x + 10,
        sliderLabelPos.y + (sliderBg.height - MeasureTextEx(font, speedText, 25, 1).y) / 2 + 5
    };
    
    // Animate the speed text when it changes
    static float prevSpeed = 1.0f;
    static float speedAnimScale = 1.0f;
    
    if (fabsf(prevSpeed - game->ballSpeedMultiplier) > 0.05f) {
        speedAnimScale = 1.3f;
        prevSpeed = game->ballSpeedMultiplier;
    }
    
    speedAnimScale = Lerp(speedAnimScale, 1.0f, 0.1f);
    
    DrawTextEx(font, speedText, speedTextPos, 25 * speedAnimScale, 1, 
              ColorAlpha(WHITE, 0.7f + (speedAnimScale - 1.0f) * 1.5f));
    
    // Draw slider indicators with subtle animation
    float indicatorAlpha = 0.6f + sinf(GetTime() * 2) * 0.2f;
    DrawTextEx(font, "Slow", (Vector2){ sliderBg.x - 40, sliderBg.y }, 20, 1, 
              ColorAlpha(LIGHTGRAY, indicatorAlpha));
    DrawTextEx(font, "Fast", (Vector2){ sliderBg.x + sliderBg.width + 10, sliderBg.y }, 20, 1, 
              ColorAlpha(LIGHTGRAY, indicatorAlpha));
    
    // Controls information with modern styling
    Rectangle controlsBg = {
        SCREEN_WIDTH / 2 - 200,
        SCREEN_HEIGHT * 3/4 + 50,
        400,
        40
    };
    
    DrawRectangleRounded(controlsBg, 0.3f, 8, ColorAlpha(BLACK, 0.3f));
    DrawRectangleRoundedLines(controlsBg, 0.3f, 8, accentGlow);
    
    const char* controlsText = "Player 1: W/S    Player 2: UP/DOWN";
    Vector2 controlsPos = {
        SCREEN_WIDTH / 2 - MeasureTextEx(font, controlsText, 20, 1).x / 2,
        SCREEN_HEIGHT * 3/4 + 60
    };
    DrawTextEx(font, controlsText, controlsPos, 20, 1, WHITE);
    
    // Animated fullscreen instruction
    float fsAlpha = 0.5f + sinf(GetTime() * 3) * 0.2f;
    const char* fullscreenText = "Press F for Fullscreen";
    Vector2 fullscreenPos = {
        SCREEN_WIDTH / 2 - MeasureTextEx(font, fullscreenText, 20, 1).x / 2,
        SCREEN_HEIGHT - 30
    };
    
    Rectangle fsRect = {
        fullscreenPos.x - 10,
        fullscreenPos.y - 5,
        MeasureTextEx(font, fullscreenText, 20, 1).x + 20,
        30
    };
    
    DrawRectangleRounded(fsRect, 0.5f, 8, ColorAlpha(BLACK, 0.3f));
    DrawRectangleRoundedLines(fsRect, 0.5f, 8, accentGlow);
    DrawTextEx(font, fullscreenText, fullscreenPos, 20, 1, ColorAlpha(WHITE, fsAlpha));

    EndDrawing();
}

void UpdateModeSelect(Game *game) {
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
        InitGame(game, MODE_AI);
    } else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
        InitGame(game, MODE_MULTIPLAYER);
    }
    
    // Handle slider interaction
    Rectangle sliderBg = { 
        SCREEN_WIDTH / 2 - 150, 
        SCREEN_HEIGHT * 3/4, 
        300, 
        20 
    };
    
    // Extend hit area for easier interaction
    Rectangle sliderHitArea = {
        sliderBg.x, sliderBg.y - 10,
        sliderBg.width, sliderBg.height + 20
    };
    
    Vector2 mousePos = GetMousePosition();
    
    // Check if mouse is over slider or if dragging
    static bool dragging = false;
    
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        if (dragging || CheckCollisionPointRec(mousePos, sliderHitArea)) {
            dragging = true;
            
            // Calculate speed multiplier based on mouse position
            float relativeX = Clamp(mousePos.x - sliderBg.x, 0, sliderBg.width);
            game->ballSpeedMultiplier = 0.5f + (relativeX / sliderBg.width) * 1.5f;
            
            // Snap to 0.1 increments for better control
            game->ballSpeedMultiplier = roundf(game->ballSpeedMultiplier * 10.0f) / 10.0f;
        }
    } else {
        dragging = false;
    }
    
    // Handle mouse clicks on mode options
    Rectangle mode1Bounds = {
        SCREEN_WIDTH / 2 - 200, 
        SCREEN_HEIGHT / 2 - 30, 
        400, 
        60
    };
    
    Rectangle mode2Bounds = {
        SCREEN_WIDTH / 2 - 200, 
        SCREEN_HEIGHT / 2 + 30, 
        400, 
        60
    };
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mousePos, mode1Bounds)) {
            InitGame(game, MODE_AI);
        } else if (CheckCollisionPointRec(mousePos, mode2Bounds)) {
            InitGame(game, MODE_MULTIPLAYER);
        }
    }
}

void InitGame(Game *game, GameMode mode) {
    // Set game mode
    game->mode = mode;
    
    // Reset game state
    game->state = STATE_PLAYING;
    game->playerScore = 0;
    game->aiScore = 0;
    game->winScore = 10;
    
    // Initialize ball with modern styling
    game->ball.radius = BALL_RADIUS;
    game->ball.color = COLOR_BALL;
    
    // Initialize player paddle with modern styling
    game->playerPaddle.rect = (Rectangle){
        10, 
        (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2, 
        PADDLE_WIDTH, 
        PADDLE_HEIGHT
    };
    game->playerPaddle.speed = PADDLE_SPEED;
    game->playerPaddle.color = COLOR_PLAYER_ONE;
    
    // Initialize AI or second player paddle with modern styling
    game->aiPaddle.rect = (Rectangle){
        SCREEN_WIDTH - 10 - PADDLE_WIDTH,
        (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2,
        PADDLE_WIDTH,
        PADDLE_HEIGHT
    };
    game->aiPaddle.speed = PADDLE_SPEED;
    game->aiPaddle.color = (mode == MODE_AI) ? COLOR_AI : COLOR_PLAYER_TWO;
    
    // Reset ball with player serving
    ResetBall(game, true);

    // Initialize particle system
    game->activeParticles = 0;
    
    // Initialize animation values
    game->scoreAnimScale = 1.0f;
    game->lastScoreTime = 0;
    game->screenShake = 0;
    game->shakeOffset = (Vector2){0, 0};
}

void UpdateGame(Game *game) {
    switch (game->state) {
        case STATE_PLAYING:
            // Toggle pause
            if (IsKeyPressed(KEY_P)) {
                game->state = STATE_PAUSED;
                return;
            }
            
            // Handle player 1 paddle movement
            float playerMovement = 0.0f;
            if (IsKeyDown(KEY_W)) playerMovement -= game->playerPaddle.speed;
            if (IsKeyDown(KEY_S)) playerMovement += game->playerPaddle.speed;
            
            game->playerPaddle.rect.y += playerMovement;
            
            // Clamp player paddle position to screen bounds
            game->playerPaddle.rect.y = Clamp(
                game->playerPaddle.rect.y, 
                0, 
                SCREEN_HEIGHT - game->playerPaddle.rect.height
            );
            
            // Handle second paddle (AI or Player 2)
            if (game->mode == MODE_AI) {
                // AI controls the paddle - pass the game object for ball speed info
                UpdateAI(&game->aiPaddle, &game->ball, game);
            } else {
                // Player 2 controls the paddle
                float player2Movement = 0.0f;
                if (IsKeyDown(KEY_UP)) player2Movement -= game->aiPaddle.speed;
                if (IsKeyDown(KEY_DOWN)) player2Movement += game->aiPaddle.speed;
                
                game->aiPaddle.rect.y += player2Movement;
                
                // Clamp player 2 paddle position to screen bounds
                game->aiPaddle.rect.y = Clamp(
                    game->aiPaddle.rect.y, 
                    0, 
                    SCREEN_HEIGHT - game->aiPaddle.rect.height
                );
            }
            
            // Update ball position
            game->ball.position.x += game->ball.velocity.x;
            game->ball.position.y += game->ball.velocity.y;
            
            // Ball collision with top and bottom walls
            if (game->ball.position.y - game->ball.radius <= 0 || 
                game->ball.position.y + game->ball.radius >= SCREEN_HEIGHT) {
                
                game->ball.velocity.y *= -1.0f;
                
                // Ensure ball doesn't get stuck in walls
                if (game->ball.position.y < game->ball.radius) {
                    game->ball.position.y = game->ball.radius + WALL_BOUNCE_BUFFER;
                }
                if (game->ball.position.y > SCREEN_HEIGHT - game->ball.radius) {
                    game->ball.position.y = SCREEN_HEIGHT - game->ball.radius - WALL_BOUNCE_BUFFER;
                }
            }
            
            // Check for paddle collisions
            if (CheckPaddleCollision(&game->ball, &game->playerPaddle, game)) {
                // Calculate normalized hit position (-0.5 to 0.5)
                float hitPosition = (game->ball.position.y - (game->playerPaddle.rect.y + game->playerPaddle.rect.height/2)) / 
                                    (game->playerPaddle.rect.height/2);
                
                // Make the ball faster with each hit, using adjusted max speed
                float adjustedMaxSpeed = MAX_BALL_SPEED * game->ballSpeedMultiplier;
                float speed = fminf(fabs(game->ball.velocity.x) + SPEED_INCREMENT, adjustedMaxSpeed);
                
                // Set new velocity based on hit position (affects angle)
                game->ball.velocity.x = speed;
                game->ball.velocity.y = hitPosition * (speed * 0.75f);
                
                // Play hit sound
                PlaySound(game->paddleHitSound);
                game->screenShake = 5.0f;
                
                // Create particle effect
                CreateParticleEffect(game, game->ball.position, ColorAlpha(WHITE, 0.8f), 15);
            }

            if (CheckPaddleCollision(&game->ball, &game->aiPaddle, game)) {
                // Calculate normalized hit position (-0.5 to 0.5)
                float hitPosition = (game->ball.position.y - (game->aiPaddle.rect.y + game->aiPaddle.rect.height/2)) / 
                                    (game->aiPaddle.rect.height/2);
                
                // Make the ball faster with each hit, using adjusted max speed
                float adjustedMaxSpeed = MAX_BALL_SPEED * game->ballSpeedMultiplier;
                float speed = fminf(fabs(game->ball.velocity.x) + SPEED_INCREMENT, adjustedMaxSpeed);
                
                // Set new velocity based on hit position (affects angle)
                game->ball.velocity.x = -speed;
                game->ball.velocity.y = hitPosition * (speed * 0.75f);
                
                // Play hit sound
                PlaySound(game->paddleHitSound);
                game->screenShake = 5.0f;
                
                // Create particle effect
                CreateParticleEffect(game, game->ball.position, ColorAlpha(WHITE, 0.8f), 15);
            }
            
            // Ball out of bounds - scoring
            if (game->ball.position.x < -BALL_RADIUS) {
                game->aiScore++;
                PlaySound(game->scoreSound);
                ResetBall(game, false);
            } else if (game->ball.position.x > SCREEN_WIDTH + BALL_RADIUS) {
                game->playerScore++;
                PlaySound(game->scoreSound);
                ResetBall(game, true);
            }
            
            // Check for game over
            if (game->playerScore >= game->winScore || game->aiScore >= game->winScore) {
                game->state = STATE_GAME_OVER;
            }
            break;
            
        case STATE_PAUSED:
            // Resume game if P is pressed again
            if (IsKeyPressed(KEY_P)) {
                game->state = STATE_PLAYING;
            }
            break;
            
        case STATE_GAME_OVER:
            // Restart game if R is pressed
            if (IsKeyPressed(KEY_R)) {
                InitGame(game, game->mode);  // Keep same mode
            } else if (IsKeyPressed(KEY_M)) {
                game->state = STATE_MODE_SELECT;  // Go back to mode selection
            }
            break;
            
        default:
            // Handle other states if needed
            break;
    }
}

void DrawGame(Game *game) {
    BeginDrawing();
    
    // Apply screen shake if active
    game->screenShake *= 0.9f; // Dampen the shake effect over time
    
    if (game->screenShake > 0.1f) {
        game->shakeOffset.x = GetRandomValue(-10, 10) * (game->screenShake / 10.0f);
        game->shakeOffset.y = GetRandomValue(-10, 10) * (game->screenShake / 10.0f);
    } else {
        game->shakeOffset = (Vector2){0, 0};
        game->screenShake = 0;
    }
    
    // Apply offset to all drawing operations
    BeginMode2D((Camera2D){ 
        .offset = game->shakeOffset,
        .target = (Vector2){0, 0},
        .rotation = 0,
        .zoom = 1.0f
    });
    
    // Modern gradient background
    Color topColor = (Color){ 12, 20, 28, 255 };
    Color bottomColor = COLOR_BACKGROUND;
    
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float factor = (float)y / SCREEN_HEIGHT;
        Color lineColor = {
            (unsigned char)Lerp(topColor.r, bottomColor.r, factor),
            (unsigned char)Lerp(topColor.g, bottomColor.g, factor),
            (unsigned char)Lerp(topColor.b, bottomColor.b, factor),
            255
        };
        DrawLine(0, y, SCREEN_WIDTH, y, lineColor);
    }
    
    // Draw court lines and center circle
    DrawLineEx(
        (Vector2){ SCREEN_WIDTH/2, 0 },
        (Vector2){ SCREEN_WIDTH/2, SCREEN_HEIGHT },
        2,
        ColorAlpha(WHITE, 0.3f)
    );
    
    DrawCircleLines(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 100, ColorAlpha(WHITE, 0.3f));
    
    // Draw middle dashed line with smoother appearance
    for (int i = 0; i < SCREEN_HEIGHT; i += 20) {
        DrawRectangleRounded(
            (Rectangle){ SCREEN_WIDTH/2 - 1, i, 2, 10 },
            0.5f,
            4,
            ColorAlpha(WHITE, 0.5f)
        );
    }
    
    // Draw paddles with rounded corners and glow
    DrawRoundedRectangleWithGlow(
        game->playerPaddle.rect,
        0.3f,
        8,
        game->playerPaddle.color
    );
    
    DrawRoundedRectangleWithGlow(
        game->aiPaddle.rect,
        0.3f,
        8,
        game->aiPaddle.color
    );
    
    // Draw ball with glow effect
    DrawBallWithGlow(game->ball.position, game->ball.radius, COLOR_BALL);
    
    // Update and draw particles
    UpdateAndDrawParticles(game);
    
    // Draw scores with shadow effect
    char scoreText[8];
    const char* player1Label = "P1";
    const char* player2Label = (game->mode == MODE_AI) ? "AI" : "P2";
    
    // Player 1 score shadow + text
    sprintf(scoreText, "%d", game->playerScore);
    Vector2 playerScorePos = {
        SCREEN_WIDTH/4 - MeasureTextEx(game->gameFont, scoreText, 80, 1).x/2,
        20
    };
    Vector2 scoreShadowPos = { playerScorePos.x + 3, playerScorePos.y + 3 };
    DrawTextEx(game->gameFont, scoreText, scoreShadowPos, 80, 1, ColorAlpha(BLACK, 0.5f));
    DrawTextEx(game->gameFont, scoreText, playerScorePos, 80, 1, WHITE);
    
    // Player 1 label
    Vector2 player1LabelPos = {
        SCREEN_WIDTH/4 - MeasureTextEx(game->gameFont, player1Label, 24, 1).x/2,
        110
    };
    DrawTextEx(game->gameFont, player1Label, player1LabelPos, 24, 1, game->playerPaddle.color);
    
    // Player 2 / AI score shadow + text
    sprintf(scoreText, "%d", game->aiScore);
    Vector2 aiScorePos = {
        3*SCREEN_WIDTH/4 - MeasureTextEx(game->gameFont, scoreText, 80, 1).x/2,
        20
    };
    scoreShadowPos = (Vector2){ aiScorePos.x + 3, aiScorePos.y + 3 };
    DrawTextEx(game->gameFont, scoreText, scoreShadowPos, 80, 1, ColorAlpha(BLACK, 0.5f));
    DrawTextEx(game->gameFont, scoreText, aiScorePos, 80, 1, WHITE);
    
    // Player 2 / AI label
    Vector2 player2LabelPos = {
        3*SCREEN_WIDTH/4 - MeasureTextEx(game->gameFont, player2Label, 24, 1).x/2,
        110
    };
    DrawTextEx(game->gameFont, player2Label, player2LabelPos, 24, 1, game->aiPaddle.color);
    
    EndMode2D(); // End the camera mode with shake
    
    // UI elements that shouldn't shake (scores, messages)
    // Draw game state messages with improved styling
    if (game->state == STATE_PAUSED) {
        // Draw semi-transparent overlay
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.7f));
        
        char pauseText[] = "GAME PAUSED";
        char resumeText[] = "Press P to resume";
        char copyrightText[] = "(C) Duke Bismaya 2025. All rights Reserved";
        
        // Draw pause message with glow effect
        Vector2 pauseTextPos = {
            SCREEN_WIDTH/2 - MeasureTextEx(game->gameFont, pauseText, 60, 1).x/2,
            SCREEN_HEIGHT/2 - 60
        };
        
        Vector2 glowPos = { pauseTextPos.x + 2, pauseTextPos.y + 2 };
        DrawTextEx(game->gameFont, pauseText, glowPos, 60, 1, ColorAlpha(COLOR_ACCENT, 0.5f));
        DrawTextEx(game->gameFont, pauseText, pauseTextPos, 60, 1, WHITE);
        
        Vector2 resumeTextPos = {
            SCREEN_WIDTH/2 - MeasureTextEx(game->gameFont, resumeText, 24, 1).x/2,
            SCREEN_HEIGHT/2 + 30
        };
        DrawTextEx(game->gameFont, resumeText, resumeTextPos, 24, 1, COLOR_ACCENT);
        
        Vector2 copyrightPos = {
            SCREEN_WIDTH/2 - MeasureTextEx(game->gameFont, copyrightText, 20, 1).x/2,
            SCREEN_HEIGHT - 40
        };
        DrawTextEx(game->gameFont, copyrightText, copyrightPos, 20, 1, ColorAlpha(WHITE, 0.7f));
        
    } else if (game->state == STATE_GAME_OVER) {
        // Draw semi-transparent overlay
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.7f));
        
        const char* winnerLabel = (game->playerScore >= game->winScore) ? 
                                (game->mode == MODE_AI ? "YOU WIN!" : "PLAYER 1 WINS!") : 
                                (game->mode == MODE_AI ? "AI WINS!" : "PLAYER 2 WINS!");
        
        char restartText[] = "Press R to restart";
        char menuText[] = "Press M for menu";
        
        // Draw winner message with glow effect
        Vector2 winnerTextPos = {
            SCREEN_WIDTH/2 - MeasureTextEx(game->gameFont, winnerLabel, 70, 1).x/2,
            SCREEN_HEIGHT/2 - 80
        };
        
        Vector2 glowPos = { winnerTextPos.x + 3, winnerTextPos.y + 3 };
        DrawTextEx(game->gameFont, winnerLabel, glowPos, 70, 1, ColorAlpha(COLOR_ACCENT, 0.6f));
        DrawTextEx(game->gameFont, winnerLabel, winnerTextPos, 70, 1, WHITE);
        
        Vector2 restartTextPos = {
            SCREEN_WIDTH/2 - MeasureTextEx(game->gameFont, restartText, 24, 1).x/2,
            SCREEN_HEIGHT/2 + 20
        };
        DrawTextEx(game->gameFont, restartText, restartTextPos, 24, 1, COLOR_ACCENT);
        
        Vector2 menuTextPos = {
            SCREEN_WIDTH/2 - MeasureTextEx(game->gameFont, menuText, 24, 1).x/2,
            SCREEN_HEIGHT/2 + 60
        };
        DrawTextEx(game->gameFont, menuText, menuTextPos, 24, 1, COLOR_ACCENT);
    }
    
    // Always show fullscreen toggle hint with subtle styling
    char fsText[] = "F: Toggle Fullscreen";
    Vector2 fsTextPos = {20, SCREEN_HEIGHT - 35};
    DrawRectangleRounded(
        (Rectangle){ fsTextPos.x - 10, fsTextPos.y - 5, 
                    MeasureTextEx(game->gameFont, fsText, 20, 1).x + 20, 30 },
        0.3f,
        6,
        ColorAlpha(BLACK, 0.5f)
    );
    DrawTextEx(game->gameFont, fsText, fsTextPos, 20, 1, ColorAlpha(WHITE, 0.8f));
    
    EndDrawing();
}

// Function to create particles
void CreateParticleEffect(Game *game, Vector2 position, Color color, int count) {
    for (int i = 0; i < count && game->activeParticles < MAX_PARTICLES; i++) {
        int particleIndex = game->activeParticles;
        game->particles[particleIndex].position = position;
        game->particles[particleIndex].velocity = (Vector2){
            (float)GetRandomValue(-200, 200) / 100.0f,
            (float)GetRandomValue(-200, 200) / 100.0f
        };
        game->particles[particleIndex].lifeTime = 0;
        game->particles[particleIndex].maxLifeTime = (float)GetRandomValue(30, 90) / 100.0f;
        game->particles[particleIndex].color = color;
        game->particles[particleIndex].size = (float)GetRandomValue(2, 6);
        game->activeParticles++;
    }
}

// Function to update and draw particles
void UpdateAndDrawParticles(Game *game) {
    float deltaTime = GetFrameTime();
    int particlesToKeep = 0;
    
    for (int i = 0; i < game->activeParticles; i++) {
        Particle *p = &game->particles[i];
        
        // Update particle
        p->lifeTime += deltaTime;
        
        if (p->lifeTime >= p->maxLifeTime) {
            continue; // Skip dead particles
        }
        
        // Update position
        p->position.x += p->velocity.x;
        p->position.y += p->velocity.y;
        
        // Apply fade based on lifetime
        float alpha = 1.0f - (p->lifeTime / p->maxLifeTime);
        Color fadeColor = ColorAlpha(p->color, alpha);
        
        // Draw particle
        DrawCircleV(p->position, p->size * alpha, fadeColor);
        
        // Keep this particle for next frame
        if (i != particlesToKeep) {
            game->particles[particlesToKeep] = *p;
        }
        particlesToKeep++;
    }
    
    game->activeParticles = particlesToKeep;
}

// CheckPaddleCollision to create particles on collision
bool CheckPaddleCollision(Ball *ball, Paddle *paddle, Game *game) {
    // A slightly larger rectangle for better collision detection
    Rectangle paddleRect = paddle->rect;
    paddleRect.x -= ball->radius;
    paddleRect.width += ball->radius * 2;
    
    // Only check collision if ball is moving toward paddle
    bool movingTowardPaddle = (paddleRect.x < SCREEN_WIDTH/2 && ball->velocity.x < 0) || 
                              (paddleRect.x > SCREEN_WIDTH/2 && ball->velocity.x > 0);
                              
    if (!movingTowardPaddle) return false;
    
    // Improved collision detection using ray-rectangle intersection
    Vector2 ballPrev = {
        ball->position.x - ball->velocity.x,
        ball->position.y - ball->velocity.y
    };
    
    Vector2 ballDir = {
        ball->position.x - ballPrev.x,
        ball->position.y - ballPrev.y
    };
    
    float rayLength = sqrtf(ballDir.x * ballDir.x + ballDir.y * ballDir.y);
    
    if (rayLength > 0) {
        ballDir.x /= rayLength;
        ballDir.y /= rayLength;
        
        // Extended hit box for smoother collisions
        Rectangle hitBox = {
            paddleRect.x - ball->radius,
            paddleRect.y - ball->radius,
            paddleRect.width + ball->radius * 2,
            paddleRect.height + ball->radius * 2
        };
        
        // Check if ray intersects with paddle hit box
        if (CheckCollisionPointRec(ball->position, hitBox) ||
            CheckCollisionCircleRec(ball->position, ball->radius, paddleRect)) {
            return true;
        }
    }
    
    // Fallback to simpler collision check
    return CheckCollisionCircleRec(ball->position, ball->radius, paddleRect);
}

void UpdateAI(Paddle *aiPaddle, Ball *ball, Game *game) {
    // Extract difficulty from game parameters - base difficulty is still 0.7
    // Higher ball speed means AI needs better accuracy
    float difficulty = 0.7f * (1.0f + (game->ballSpeedMultiplier - 1.0f) * 0.5f);
    difficulty = Clamp(difficulty, 0.5f, 0.95f); // Keep AI challenge balanced
    
    
    // Calculate the predicted y-position where the ball will intersect with AI paddle
    float predictedY = ball->position.y;
    
    // Only do advanced prediction when ball is moving toward the AI paddle
    if (ball->velocity.x > 0) {
        // Calculate time until ball reaches paddle (x distance / x speed)
        float distanceToIntercept = aiPaddle->rect.x - ball->position.x;
        float timeToIntercept = distanceToIntercept / ball->velocity.x;
        
        if (timeToIntercept > 0) {
            // Predict where the ball will be at that time
            predictedY = ball->position.y + (ball->velocity.y * timeToIntercept);
            
            // Account for possible bounces off top/bottom walls
            float bounceDistance = predictedY < 0 ? -predictedY : 
                                 (predictedY > SCREEN_HEIGHT ? 2*SCREEN_HEIGHT - predictedY : 0);
            
            while (bounceDistance > 0) {
                if (predictedY < 0) {
                    predictedY = -predictedY; // Bounce off top
                } else if (predictedY > SCREEN_HEIGHT) {
                    predictedY = 2*SCREEN_HEIGHT - predictedY; // Bounce off bottom
                } else {
                    break;
                }
                
                // Check if another bounce would occur
                bounceDistance = predictedY < 0 ? -predictedY : 
                               (predictedY > SCREEN_HEIGHT ? 2*SCREEN_HEIGHT - predictedY : 0);
            }
        }
    }
    
    // Target position (center of paddle aligned with predicted ball position)
    float targetY = predictedY - aiPaddle->rect.height/2;
    
    // Some imperfection based on difficulty
    if (GetRandomValue(0, 100) < (int)(30 * (1.0f - difficulty))) {
        targetY += GetRandomValue(-30, 30) * (1.0f - difficulty);
    }
    
    // Clamp target position to screen bounds
    targetY = Clamp(targetY, 0, SCREEN_HEIGHT - aiPaddle->rect.height);
    
    // Calculate distance to target
    float distanceToTarget = targetY - aiPaddle->rect.y;
    
    // Apply smooth movement with easing
    if (fabsf(distanceToTarget) > 1.0f) {
        // Use a proportional approach with difficulty factor
        float moveStep = distanceToTarget * 0.1f * difficulty;
        
        // Cap movement speed
        float maxStep = aiPaddle->speed * difficulty;
        if (fabsf(moveStep) > maxStep) {
            moveStep = maxStep * (distanceToTarget > 0 ? 1.0f : -1.0f);
        }
        
        aiPaddle->rect.y += moveStep;
    }
    
    // Ensure paddle stays in bounds
    aiPaddle->rect.y = Clamp(aiPaddle->rect.y, 0, SCREEN_HEIGHT - aiPaddle->rect.height);
}

// Draw a rounded rectangle with glow effect
void DrawRoundedRectangleWithGlow(Rectangle rec, float roundness, int segments, Color color) {
    // Draw glow effect first (larger rectangle with semi-transparent color)
    Rectangle glowRec = {
        rec.x - 8, rec.y - 8,
        rec.width + 16, rec.height + 16
    };
    DrawRectangleRounded(glowRec, roundness, segments, ColorAlpha(color, 0.3f));
    
    // Draw the main rectangle
    DrawRectangleRounded(rec, roundness, segments, color);
}

// Draw ball with glow effect
void DrawBallWithGlow(Vector2 center, float radius, Color color) {
    // Draw glow
    DrawCircleV(center, radius * 1.5f, ColorAlpha(color, 0.3f));
    DrawCircleV(center, radius * 1.3f, ColorAlpha(color, 0.2f));
    
    // Draw main ball
    DrawCircleV(center, radius, color);
    
    // Draw highlight
    DrawCircleSector(
        center,
        radius * 0.7f,
        225,
        315,
        10,
        ColorAlpha(WHITE, 0.3f)
    );
}

void ResetBall(Game *game, bool serverIsPlayer) {
    // Reset ball position to center
    game->ball.position = (Vector2){ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
    
    // Set initial velocity based on server
    float initialSpeed = BALL_INITIAL_SPEED * game->ballSpeedMultiplier;
    game->ball.velocity = (Vector2){
        serverIsPlayer ? initialSpeed : -initialSpeed,
        (float)GetRandomValue(-100, 100) / 100.0f * initialSpeed
    };
    
    // Update last score time for animation
    game->lastScoreTime = GetTime();
    game->scoreAnimScale = 1.5f; // Start animation scale
    
    // Add particles effect on scoring
    Color particleColor = serverIsPlayer ? game->aiPaddle.color : game->playerPaddle.color;
    CreateParticleEffect(
        game, 
        game->ball.position,
        particleColor,
        30
    );
}