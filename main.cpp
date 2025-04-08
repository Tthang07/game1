#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PLAYER_WIDTH = 60;
const int PLAYER_HEIGHT = 120;
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
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("Space Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Surface* surface = IMG_Load("tàu 1 (2).png");
    if (!surface) {
        cout << "Không thể load ảnh máy bay: " << IMG_GetError() << endl;
        return -1;
    }
    SDL_Texture* playerTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    Player player = { SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT - 10 };

    bool running = true;
    SDL_Event event;
    int enemySpawnCounter = 0;
    int bulletCooldown = 0;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LEFT]) player.moveLeft();
        if (keystate[SDL_SCANCODE_RIGHT]) player.moveRight();


        if (bulletCooldown > 0) bulletCooldown--;


        if (keystate[SDL_SCANCODE_SPACE] && bulletCooldown == 0) {
            bullets.push_back({ player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2, player.y, BULLET_WIDTH, BULLET_HEIGHT, true });
            bulletCooldown = 10;
        }

        for (auto& bullet : bullets) {
            if (bullet.active) {
                bullet.y -= 10;
                if (bullet.y < 0) bullet.active = false;
            }
        }

        if (++enemySpawnCounter > 30) {
            spawnEnemy();
            enemySpawnCounter = 0;
        }

        for (auto& enemy : enemies) {
            if (enemy.active) {
                enemy.y += 3;
                if (enemy.y > SCREEN_HEIGHT) {
                    enemy.active = false;
                    player.lives--;
                    if (player.lives <= 0) running = false;
                }
            }
        }

        for (auto& bullet : bullets) {
            if (bullet.active) {
                for (auto& enemy : enemies) {
                    if (enemy.active && bullet.x < enemy.x + enemy.w && bullet.x + bullet.w > enemy.x &&
                        bullet.y < enemy.y + enemy.h && bullet.y + bullet.h > enemy.y) {
                        enemy.active = false;
                        bullet.active = false;
                        player.score += 10;
                    }
                }
            }
        }

        bullets.erase(remove_if(bullets.begin(), bullets.end(), [](const GameObject& b) { return !b.active; }), bullets.end());
        enemies.erase(remove_if(enemies.begin(), enemies.end(), [](const GameObject& e) { return !e.active; }), enemies.end());

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Rect playerRect = { player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT };
        SDL_RenderCopy(renderer, playerTexture, NULL, &playerRect);

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        for (const auto& bullet : bullets) {
            if (bullet.active) {
                SDL_Rect bulletRect = { bullet.x, bullet.y, BULLET_WIDTH, BULLET_HEIGHT };
                SDL_RenderFillRect(renderer, &bulletRect);
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (const auto& enemy : enemies) {
            if (enemy.active) {
                SDL_Rect enemyRect = { enemy.x, enemy.y, ENEMY_WIDTH, ENEMY_HEIGHT };
                SDL_RenderFillRect(renderer, &enemyRect);
            }
        }

        renderScore(renderer, player.score, player.lives);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(playerTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
