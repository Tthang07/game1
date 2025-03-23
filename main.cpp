#include <SDL.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
using namespace std;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PLAYER_WIDTH = 50;
const int PLAYER_HEIGHT = 50;
const int ENEMY_WIDTH = 40;
const int ENEMY_HEIGHT = 40;
const int BULLET_WIDTH = 10;
const int BULLET_HEIGHT = 20;
struct GameObject {
    int x, y, w, h;
    bool active = true;
};
struct Player {
    int x, y;
    int speed = 10;
    int lives = 3;
    int score = 0;
    void moveLeft() { if (x > 0) x -= speed; }
    void moveRight() { if (x < SCREEN_WIDTH - PLAYER_WIDTH) x += speed; }
};
vector<GameObject> bullets;
vector<GameObject> enemies;
void spawnEnemy() {
    int xPos = rand() % (SCREEN_WIDTH - ENEMY_WIDTH);
    enemies.push_back({ xPos, 0, ENEMY_WIDTH, ENEMY_HEIGHT, true });
}
void renderScore(SDL_Renderer* renderer, int score, int lives) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < lives; i++) {
        SDL_Rect lifeRect = { 10 + i * 20, 10, 15, 15 };
        SDL_RenderFillRect(renderer, &lifeRect);
    }
    int offsetX = 100;
    int tempScore = score;
    do {
        int digit = tempScore % 10;
        SDL_Rect digitRect = { offsetX, 10, 15, 20 };
        SDL_RenderFillRect(renderer, &digitRect);
        offsetX -= 20;
        tempScore /= 10;
    } while (tempScore > 0);
}
int main() {
    srand(time(0));
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Space Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    Player player = { SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - 60 };
    bool running = true;
    SDL_Event event;
    int enemySpawnCounter = 0;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LEFT]) player.moveLeft();
        if (keystate[SDL_SCANCODE_RIGHT]) player.moveRight();
        if (keystate[SDL_SCANCODE_SPACE]) {
            bullets.push_back({ player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2, player.y, BULLET_WIDTH, BULLET_HEIGHT, true });
        }
