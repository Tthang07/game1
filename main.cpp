#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <algorithm>
#include <fstream>
using namespace std;

const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;
const int PLAYER_WIDTH = 80;
const int PLAYER_HEIGHT = 140;
const int ENEMY_WIDTH = 85;
const int ENEMY_HEIGHT = 85;
const int BULLET_WIDTH = 40;
const int BULLET_HEIGHT = 70;

struct GameObject {
    int x, y, w, h;
    bool active = true;
};

struct Explosion {
    int x, y;
    int frame = 0;
};

vector<GameObject> bullets;
vector<GameObject> enemies;
vector<GameObject> enemyBullets;
vector<Explosion> explosions;

struct Player {
    int x, y;
    int speed = 10;
    int lives = 3;
    int score = 0;
    bool invincible = false;
    int invincibleTimer = 0;

    void moveLeft() { if (x > 0) x -= speed; }
    void moveRight() { if (x < SCREEN_WIDTH - PLAYER_WIDTH) x += speed; }
    void moveUp() { if (y > 0) y -= speed; }
    void moveDown() { if (y < SCREEN_HEIGHT - PLAYER_HEIGHT) y += speed; }
};

int enemyWaveCount = 0;

void spawnEnemyBullet(GameObject& enemy) {
    GameObject bullet = { enemy.x + enemy.w / 2 - 10, enemy.y + enemy.h, 20, 50, true };
    enemyBullets.push_back(bullet);
}

void spawnEnemyWave() {
    enemyWaveCount++;

    if (enemyWaveCount % 10 == 0) {
        int spacing = 90;
        int startX = 100;
        for (int i = 0; i < 5; ++i) {
            enemies.push_back({ startX + i * spacing, 0, ENEMY_WIDTH, ENEMY_HEIGHT, true });
        }
    }
    else if (enemyWaveCount % 15 == 0) {
        int centerX = SCREEN_WIDTH / 2;
        enemies.push_back({ centerX - ENEMY_WIDTH / 2, 0, ENEMY_WIDTH, ENEMY_HEIGHT, true });
        enemies.push_back({ centerX - ENEMY_WIDTH - 20, -ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true });
        enemies.push_back({ centerX + 20, -ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true });
        enemies.push_back({ centerX - 2 * ENEMY_WIDTH - 40, -2 * ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true });
        enemies.push_back({ centerX + 2 * ENEMY_WIDTH + 40 - ENEMY_WIDTH, -2 * ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true });
    }
    else {
        int xPos = rand() % (SCREEN_WIDTH - ENEMY_WIDTH);
        enemies.push_back({ xPos, 0, ENEMY_WIDTH, ENEMY_HEIGHT, true });
    }
}

void renderScore(SDL_Renderer* renderer, SDL_Texture* lifeTexture, int lives, int score) {
    for (int i = 0; i < lives; i++) {
        SDL_Rect rect = { 10 + i * 35, 10, 30, 30 };
        SDL_RenderCopy(renderer, lifeTexture, NULL, &rect);
    }
}

int main() {
    srand(time(0));
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("Space Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Surface* surface = IMG_Load("tàu 1.png");
    if (!surface) {
        cout << "Không thể load ảnh máy bay: " << IMG_GetError() << endl;
        return -1;
    }
    SDL_Texture* playerTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    SDL_Texture* bulletTexture = IMG_LoadTexture(renderer, "đạn 4.png");
    SDL_Texture* enemyTexture = IMG_LoadTexture(renderer, "địch.png");
    SDL_Texture* backgroundTexture = IMG_LoadTexture(renderer, "nền.png");
    SDL_Texture* enemyBulletTexture = IMG_LoadTexture(renderer, "đạn địch.png");
    SDL_Texture* lifeTexture = IMG_LoadTexture(renderer, "tàu 1.png");
    SDL_Texture* explosionTexture = IMG_LoadTexture(renderer, "địch nổ.png");

    Player player = { SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT - 10 };

    bool running = true;
    SDL_Event event;
    int enemySpawnCounter = 0;
    int enemyShootCounter = 0;
    int bulletCooldown = 0;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LEFT]) player.moveLeft();
        if (keystate[SDL_SCANCODE_RIGHT]) player.moveRight();
        if (keystate[SDL_SCANCODE_UP]) player.moveUp();
        if (keystate[SDL_SCANCODE_DOWN]) player.moveDown();

        if (bulletCooldown > 0) bulletCooldown--;

        if (keystate[SDL_SCANCODE_SPACE] && bulletCooldown == 0) {
            bullets.push_back({ player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2, player.y, BULLET_WIDTH, BULLET_HEIGHT, true });
            bulletCooldown = 10;
        }

        if (player.invincible) {
            player.invincibleTimer--;
            if (player.invincibleTimer <= 0) {
                player.invincible = false;
            }
        }

        for (auto& bullet : bullets) {
            if (bullet.active) {
                bullet.y -= 10;
                if (bullet.y < 0) bullet.active = false;
            }
        }

        if (++enemySpawnCounter > 60) {
            spawnEnemyWave();
            enemySpawnCounter = 0;
        }

        if (++enemyShootCounter > 30) {
            for (auto& enemy : enemies) {
                if (enemy.active && rand() % 2 == 0) {
                    spawnEnemyBullet(enemy);
                }
            }
            enemyShootCounter = 0;
        }

        for (auto& enemy : enemies) {
            if (enemy.active) {
                enemy.y += 3;
                if (enemy.y > SCREEN_HEIGHT) {
                    enemy.active = false;
                }
            }
        }

        for (auto& bullet : bullets) {
            if (bullet.active) {
                for (auto& enemy : enemies) {
                    if (enemy.active &&
                        bullet.x < enemy.x + enemy.w && bullet.x + bullet.w > enemy.x &&
                        bullet.y < enemy.y + enemy.h && bullet.y + bullet.h > enemy.y) {
                        explosions.push_back({ enemy.x, enemy.y });
                        enemy.active = false;
                        bullet.active = false;
                        player.score += 10;
                    }
                }
            }
        }

        for (auto& eBullet : enemyBullets) {
            if (eBullet.active) {
                eBullet.y += 6;
                if (eBullet.y > SCREEN_HEIGHT) eBullet.active = false;

                SDL_Rect bRect = { eBullet.x, eBullet.y, eBullet.w, eBullet.h };
                SDL_Rect pRect = { player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT };
                if (!player.invincible && SDL_HasIntersection(&bRect, &pRect)) {
                    eBullet.active = false;
                    player.lives--;
                    player.invincible = true;
                    player.invincibleTimer = 90; // 1.5 giây
                    if (player.lives <= 0) running = false;
                }
            }
        }

        bullets.erase(remove_if(bullets.begin(), bullets.end(), [](const GameObject& b) { return !b.active; }), bullets.end());
        enemies.erase(remove_if(enemies.begin(), enemies.end(), [](const GameObject& e) { return !e.active; }), enemies.end());
        enemyBullets.erase(remove_if(enemyBullets.begin(), enemyBullets.end(), [](const GameObject& b) { return !b.active; }), enemyBullets.end());

        SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);

        if (player.invincible && player.invincibleTimer > 80) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100);
            SDL_Rect flashRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
            SDL_RenderFillRect(renderer, &flashRect);
        }

        SDL_Rect playerRect = { player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT };
        SDL_RenderCopy(renderer, playerTexture, NULL, &playerRect);

        for (const auto& bullet : bullets) {
            if (bullet.active) {
                SDL_Rect rect = { bullet.x, bullet.y, BULLET_WIDTH, BULLET_HEIGHT };
                SDL_RenderCopy(renderer, bulletTexture, NULL, &rect);
            }
        }

        for (const auto& enemy : enemies) {
            if (enemy.active) {
                SDL_Rect rect = { enemy.x, enemy.y, ENEMY_WIDTH, ENEMY_HEIGHT };
                SDL_RenderCopy(renderer, enemyTexture, NULL, &rect);
            }
        }

        for (const auto& eBullet : enemyBullets) {
            if (eBullet.active) {
                SDL_Rect rect = { eBullet.x, eBullet.y, eBullet.w, eBullet.h };
                SDL_RenderCopy(renderer, enemyBulletTexture, NULL, &rect);
            }
        }

        for (int i = 0; i < explosions.size(); ++i) {
            SDL_Rect rect = { explosions[i].x, explosions[i].y, ENEMY_WIDTH, ENEMY_HEIGHT };
            SDL_RenderCopy(renderer, explosionTexture, NULL, &rect);
            explosions[i].frame++;
        }
        explosions.erase(remove_if(explosions.begin(), explosions.end(), [](const Explosion& e) {
            return e.frame > 15;
        }), explosions.end());

        renderScore(renderer, lifeTexture, player.lives, player.score);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    ofstream out("score.txt");
    out << "Score: " << player.score << endl;
    out.close();

    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(enemyBulletTexture);
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(lifeTexture);
    SDL_DestroyTexture(explosionTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
