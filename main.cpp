#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <algorithm>
#include <fstream>
#include <cmath>
using namespace std;

Mix_Chunk* soundHit;
Mix_Chunk* soundExplode;
Mix_Chunk* soundShoot;

enum GameMode {
    MENU,
    SURVIVAL,
    BOSS,
    EXIT
};

enum BossState {
    BOSS_NORMAL,
    BOSS_SHIELDED,
    BOSS_DEAD
};

enum BossSkill {
    SKILL_LASER,
    SKILL_MISSILE,
    SKILL_SHIELD,
    SKILL_SPIRAL,
    SKILL_MINIONS,
    SKILL_COUNT
};

const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;
const int PLAYER_WIDTH = 80;
const int PLAYER_HEIGHT = 140;
const int ENEMY_WIDTH = 85;
const int ENEMY_HEIGHT = 85;
const int BULLET_WIDTH = 40;
const int BULLET_HEIGHT = 70;
const int BOSS_WIDTH = 200;
const int BOSS_HEIGHT = 200;
const int BOSS_INITIAL_HEALTH = 1000;
const int SHIELD_DURATION = 240;
const int LASER_DURATION = 90;
const int MISSILE_DURATION = 180;

struct GameObject {
    int x, y, w, h;
    bool active = true;
};

struct Explosion {
    int x, y;
    int frame = 0;
};

struct Laser {
    int x, y;
    int width, height;
    bool active;
    int timer;
};

struct Boss {
    int x, y;
    int health;
    BossState state;
    int shieldTimer;
    int skillCooldowns[SKILL_COUNT];
    int phase;
    int attackPattern;
    int laserTimer;
    vector<Laser> lasers;
    vector<GameObject> missiles;
    vector<GameObject> spiralBullets;
    vector<GameObject> minions;
    int speedX = 3;
    int speedY = 1;
    int moveDirection = 1;
    int moveRange = 300;
    int initialX;
};

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

vector<GameObject> bullets;
vector<GameObject> enemies;
vector<GameObject> enemyBullets;
vector<Explosion> explosions;

int enemyWaveCount = 0;
int highScore = 0;

void updateExplosions(vector<Explosion>& explosions) {
    for (auto& explosion : explosions) {
        explosion.frame++;
    }
    explosions.erase(remove_if(explosions.begin(), explosions.end(),
        [](const Explosion& e) { return e.frame > 15; }), explosions.end());
}

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
    } else if (enemyWaveCount % 15 == 0) {
        int centerX = SCREEN_WIDTH / 2;
        enemies.push_back({ centerX - ENEMY_WIDTH / 2, 0, ENEMY_WIDTH, ENEMY_HEIGHT, true });
        enemies.push_back({ centerX - ENEMY_WIDTH - 20, -ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true });
        enemies.push_back({ centerX + 20, -ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true });
        enemies.push_back({ centerX - 2 * ENEMY_WIDTH - 40, -2 * ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true });
        enemies.push_back({ centerX + 2 * ENEMY_WIDTH + 40 - ENEMY_WIDTH, -2 * ENEMY_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, true });
    } else {
        int xPos = rand() % (SCREEN_WIDTH - ENEMY_WIDTH);
        enemies.push_back({ xPos, 0, ENEMY_WIDTH, ENEMY_HEIGHT, true });
    }
}

void initBoss(Boss& boss) {
    boss.x = SCREEN_WIDTH / 2 - BOSS_WIDTH / 2;
    boss.y = 100;
    boss.initialX = boss.x;
    boss.health = BOSS_INITIAL_HEALTH;
    boss.state = BOSS_NORMAL;
    boss.shieldTimer = 0;
    boss.phase = 0;
    boss.attackPattern = 0;
    boss.laserTimer = 0;

    for (int i = 0; i < SKILL_COUNT; i++) {
        boss.skillCooldowns[i] = 0;
    }
}

void updateBoss(Boss& boss, Player& player, vector<Explosion>& explosions, int& enemyShootCounter) {
    boss.x += boss.speedX * boss.moveDirection;

    if (boss.x > boss.initialX + boss.moveRange) {
        boss.moveDirection = -1;
    }
    else if (boss.x < boss.initialX - boss.moveRange) {
        boss.moveDirection = 1;
    }

    boss.y += boss.speedY * sin(SDL_GetTicks() * 0.005);

    boss.x = max(0, min(boss.x, SCREEN_WIDTH - BOSS_WIDTH));
    boss.y = max(50, min(boss.y, SCREEN_HEIGHT / 3));

    if (boss.state == BOSS_SHIELDED) {
        boss.shieldTimer--;
        if (boss.shieldTimer <= 0) {
            boss.state = BOSS_NORMAL;
        }
    }

    for (int i = 0; i < SKILL_COUNT; i++) {
        if (boss.skillCooldowns[i] > 0) {
            boss.skillCooldowns[i]--;
        }
    }

    boss.phase = (boss.health <= BOSS_INITIAL_HEALTH * 0.4) ? 1 : 0;

    if (boss.health > 0) {
        int skillChance = rand() % 100;
        int cooldownMultiplier = (boss.phase == 1) ? 2 : 1;

        if (boss.skillCooldowns[SKILL_LASER] == 0 && skillChance < 20) {
            for (int i = 0; i < 3; i++) {
                Laser laser;
                laser.x = (SCREEN_WIDTH / 4) * (i + 1) - 80;
                laser.y = BOSS_HEIGHT + 100;
                laser.width = 160;
                laser.height = SCREEN_HEIGHT - (BOSS_HEIGHT + 100);
                laser.active = true;
                laser.timer = LASER_DURATION;
                boss.lasers.push_back(laser);
            }
            boss.skillCooldowns[SKILL_LASER] = 900 / cooldownMultiplier;
        }

        if (boss.skillCooldowns[SKILL_MISSILE] == 0 && skillChance >= 20 && skillChance < 40) {
            GameObject missile;
            missile.x = boss.x + BOSS_WIDTH / 2 - 15;
            missile.y = boss.y + BOSS_HEIGHT;
            missile.w = 30;
            missile.h = 50;
            missile.active = true;
            boss.missiles.push_back(missile);
            boss.skillCooldowns[SKILL_MISSILE] = 500 / cooldownMultiplier;
        }

        if (boss.skillCooldowns[SKILL_SHIELD] == 0 && skillChance >= 40 && skillChance < 60) {
            boss.state = BOSS_SHIELDED;
            boss.shieldTimer = SHIELD_DURATION;
            boss.skillCooldowns[SKILL_SHIELD] = 700 / cooldownMultiplier;
        }

        if (boss.skillCooldowns[SKILL_SPIRAL] == 0 && skillChance >= 60 && skillChance < 80) {
            int bullets = 12 + rand() % 5;
            for (int i = 0; i < bullets; i++) {
                GameObject bullet;
                bullet.x = boss.x + BOSS_WIDTH / 2;
                bullet.y = boss.y + BOSS_HEIGHT;
                bullet.w = 20;
                bullet.h = 20;
                bullet.active = true;
                boss.spiralBullets.push_back(bullet);
            }
            boss.skillCooldowns[SKILL_SPIRAL] = 800 / cooldownMultiplier;
        }

        if (boss.skillCooldowns[SKILL_MINIONS] == 0 && skillChance >= 80) {
            int minionCount = 2 + rand() % 4;
            for (int i = 0; i < minionCount; i++) {
                GameObject minion;
                minion.x = rand() % (SCREEN_WIDTH - ENEMY_WIDTH);
                minion.y = -ENEMY_HEIGHT;
                minion.w = ENEMY_WIDTH;
                minion.h = ENEMY_HEIGHT;
                minion.active = true;
                boss.minions.push_back(minion);
            }
            boss.skillCooldowns[SKILL_MINIONS] = 300 / cooldownMultiplier;
        }
    }

    for (auto& minion : boss.minions) {
        if (minion.active) {
            minion.y += 3;

            if (rand() % 100 < 2) {
                spawnEnemyBullet(minion);
            }

            SDL_Rect mRect = { minion.x, minion.y, minion.w, minion.h };
            SDL_Rect pRect = { player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT };
            if (SDL_HasIntersection(&mRect, &pRect)) {
                minion.active = false;
                if (!player.invincible) {
                    player.lives--;
                    player.invincible = true;
                    player.invincibleTimer = 90;
                    Mix_PlayChannel(-1, soundHit, 0);
                }
            }

            if (minion.y > SCREEN_HEIGHT) {
                minion.active = false;
            }
        }
    }

    for (auto& laser : boss.lasers) {
        if (laser.active) {
            laser.timer--;
            if (laser.timer <= 0) {
                laser.active = false;
            }

            SDL_Rect lRect = { laser.x, laser.y, laser.width, laser.height };
            SDL_Rect pRect = { player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT };
            if (SDL_HasIntersection(&lRect, &pRect)) {
                if (!player.invincible) {
                    player.lives--;
                    player.invincible = true;
                    player.invincibleTimer = 90;
                    Mix_PlayChannel(-1, soundHit, 0);
                }
            }
        }
    }

    for (auto& missile : boss.missiles) {
        if (missile.active) {
            if (missile.x < player.x + PLAYER_WIDTH / 2) missile.x += 3;
            else if (missile.x > player.x + PLAYER_WIDTH / 2) missile.x -= 3;
            missile.y += 5;

            SDL_Rect mRect = { missile.x, missile.y, missile.w, missile.h };
            SDL_Rect pRect = { player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT };
            if (SDL_HasIntersection(&mRect, &pRect)) {
                missile.active = false;
                if (!player.invincible) {
                    player.lives--;
                    player.invincible = true;
                    player.invincibleTimer = 90;
                    Mix_PlayChannel(-1, soundHit, 0);
                }
            }

            if (missile.y > SCREEN_HEIGHT) {
                missile.active = false;
            }
        }
    }

    for (auto& bullet : boss.spiralBullets) {
        if (bullet.active) {
            float angle = atan2(bullet.y - (boss.y + BOSS_HEIGHT), bullet.x - (boss.x + BOSS_WIDTH / 2));
            angle += 0.1;
            float distance = sqrt(pow(bullet.x - (boss.x + BOSS_WIDTH / 2), 2) +
                                 pow(bullet.y - (boss.y + BOSS_HEIGHT), 2));
            distance += 2;

            bullet.x = boss.x + BOSS_WIDTH / 2 + distance * cos(angle);
            bullet.y = boss.y + BOSS_HEIGHT + distance * sin(angle);

            SDL_Rect bRect = { bullet.x, bullet.y, bullet.w, bullet.h };
            SDL_Rect pRect = { player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT };
            if (SDL_HasIntersection(&bRect, &pRect)) {
                bullet.active = false;
                if (!player.invincible) {
                    player.lives--;
                    player.invincible = true;
                    player.invincibleTimer = 90;
                    Mix_PlayChannel(-1, soundHit, 0);
                }
            }

            if (bullet.x < 0 || bullet.x > SCREEN_WIDTH || bullet.y < 0 || bullet.y > SCREEN_HEIGHT) {
                bullet.active = false;
            }
        }
    }

    boss.lasers.erase(remove_if(boss.lasers.begin(), boss.lasers.end(),
        [](const Laser& l) { return !l.active; }), boss.lasers.end());
    boss.missiles.erase(remove_if(boss.missiles.begin(), boss.missiles.end(),
        [](const GameObject& m) { return !m.active; }), boss.missiles.end());
    boss.spiralBullets.erase(remove_if(boss.spiralBullets.begin(), boss.spiralBullets.end(),
        [](const GameObject& b) { return !b.active; }), boss.spiralBullets.end());
    boss.minions.erase(remove_if(boss.minions.begin(), boss.minions.end(),
        [](const GameObject& m) { return !m.active; }), boss.minions.end());
}

void renderBoss(SDL_Renderer* renderer, Boss& boss, SDL_Texture* bossTexture,
                SDL_Texture* bossShieldTexture, SDL_Texture* laserTexture,
                SDL_Texture* bossMissileTexture, SDL_Texture* enemyTexture) {
    if (boss.health <= 0) return;

    SDL_Rect bossRect = { boss.x, boss.y, BOSS_WIDTH, BOSS_HEIGHT };
    if (boss.state == BOSS_SHIELDED) {
        SDL_RenderCopy(renderer, bossShieldTexture, NULL, &bossRect);
    } else {
        SDL_RenderCopy(renderer, bossTexture, NULL, &bossRect);
    }

    SDL_Rect healthBarBg = { boss.x, boss.y - 20, BOSS_WIDTH, 10 };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &healthBarBg);

    SDL_Rect healthBar = { boss.x, boss.y - 20, BOSS_WIDTH * boss.health / BOSS_INITIAL_HEALTH, 10 };
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &healthBar);

    for (const auto& laser : boss.lasers) {
        if (laser.active) {
            SDL_Rect laserRect = { laser.x, laser.y, laser.width, laser.height };
            SDL_RenderCopy(renderer, laserTexture, NULL, &laserRect);
        }
    }

    for (const auto& missile : boss.missiles) {
        if (missile.active) {
            SDL_Rect missileRect = { missile.x, missile.y, missile.w, missile.h };
            SDL_RenderCopy(renderer, bossMissileTexture, NULL, &missileRect);
        }
    }

    for (const auto& bullet : boss.spiralBullets) {
        if (bullet.active) {
            SDL_Rect bulletRect = { bullet.x, bullet.y, bullet.w, bullet.h };
            SDL_RenderCopy(renderer, bossMissileTexture, NULL, &bulletRect);
        }
    }

    for (const auto& minion : boss.minions) {
        if (minion.active) {
            SDL_Rect minionRect = { minion.x, minion.y, minion.w, minion.h };
            SDL_RenderCopy(renderer, enemyTexture, NULL, &minionRect);
        }
    }
}

void renderScore(SDL_Renderer* renderer, SDL_Texture* lifeTexture, int lives, int score) {
    for (int i = 0; i < lives; i++) {
        SDL_Rect rect = { 10 + i * 35, 10, 30, 30 };
        SDL_RenderCopy(renderer, lifeTexture, NULL, &rect);
    }
}

void renderText(SDL_Renderer* renderer, TTF_Font* font, const string& text, int x, int y) {
    SDL_Color color = { 255, 255, 255 };
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dstRect = { x, y, surface->w, surface->h };
    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    SDL_DestroyTexture(texture);
}

void renderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedOption, int highScore, SDL_Texture* menuBackgroundTexture) {
    SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, NULL);

    string options[3] = { "1. Survival", "2. Boss Fight", "3. Exit" };
    renderText(renderer, font, "High score: " + to_string(highScore), SCREEN_WIDTH/2 - 80, 150);

    for (int i = 0; i < 3; ++i) {
        SDL_Color color = (i == selectedOption) ? SDL_Color{255, 255, 0} : SDL_Color{255, 255, 255};
        SDL_Surface* surface = TTF_RenderText_Solid(font, options[i].c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = { SCREEN_WIDTH/2 - surface->w/2, 250 + i * 100, surface->w, surface->h };
        SDL_FreeSurface(surface);
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_DestroyTexture(texture);
    }

    SDL_RenderPresent(renderer);
}

void resetGame(Player& player, vector<GameObject>& bullets,
              vector<GameObject>& enemies, vector<GameObject>& enemyBullets,
              vector<Explosion>& explosions, int& enemyWaveCount) {
    player = { SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT - 10 };
    player.lives = 3;
    player.score = 0;
    bullets.clear();
    enemies.clear();
    enemyBullets.clear();
    explosions.clear();
    enemyWaveCount = 0;
}

int main() {
    srand(time(0));
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    soundHit = Mix_LoadWAV("trúng đạn.mp3");
    soundExplode = Mix_LoadWAV("địch nổ.mp3");
    Mix_Chunk* soundGameOver = Mix_LoadWAV("over.mp3");
    soundShoot = Mix_LoadWAV("voice đạn.mp3");
    Mix_Chunk* soundStart = Mix_LoadWAV("fight.mp3");

    SDL_Window* window = SDL_CreateWindow("Space Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font* font = TTF_OpenFont("PixelFont.ttf", 25);
    if (!font) {
        cout << "Failed to load font: " << TTF_GetError() << endl;
        return -1;
    }

    SDL_Texture* playerTexture = IMG_LoadTexture(renderer, "tàu.png");
    SDL_Texture* bulletTexture = IMG_LoadTexture(renderer, "đạn.png");
    SDL_Texture* enemyTexture = IMG_LoadTexture(renderer, "địch.png");
    SDL_Texture* backgroundTexture = IMG_LoadTexture(renderer, "nền.png");
    SDL_Texture* enemyBulletTexture = IMG_LoadTexture(renderer, "đạn địch.png");
    SDL_Texture* lifeTexture = IMG_LoadTexture(renderer, "tàu.png");
    SDL_Texture* explosionTexture = IMG_LoadTexture(renderer, "địch nổ.png");
    SDL_Texture* startTexture = IMG_LoadTexture(renderer, "fight.png");
    SDL_Texture* gameOverTexture = IMG_LoadTexture(renderer, "over.png");
    SDL_Texture* bossTexture = IMG_LoadTexture(renderer, "boss1.png");
    SDL_Texture* bossShieldTexture = IMG_LoadTexture(renderer, "khiên.png");
    SDL_Texture* missileTexture = IMG_LoadTexture(renderer, "đạn địch.png");
    SDL_Texture* bossMissileTexture = IMG_LoadTexture(renderer, "tên lửa boss.png");
    SDL_Texture* laserTexture = IMG_LoadTexture(renderer, "laze.png");
    SDL_Texture* menuBackgroundTexture = IMG_LoadTexture(renderer, "menu.png");

    ifstream in("highscore.txt");
    if (in) {
        in >> highScore;
        in.close();
    }

    Player player = { SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT - 10 };
    Boss boss;
    initBoss(boss);
    GameMode gameMode = MENU;
    int selectedOption = 0;
    bool running = true;
    SDL_Event event;
    int enemySpawnCounter = 0;
    int enemyShootCounter = 0;
    int bulletCooldown = 0;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (gameMode == MENU) {
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_DOWN) {
                        selectedOption = (selectedOption + 1) % 3;
                    }
                    if (event.key.keysym.sym == SDLK_UP) {
                        selectedOption = (selectedOption + 2) % 3;
                    }
                    if (event.key.keysym.sym == SDLK_RETURN) {
                        if (selectedOption == 0) {
                            gameMode = SURVIVAL;
                            SDL_RenderClear(renderer);
                            SDL_RenderCopy(renderer, startTexture, NULL, NULL);
                            SDL_RenderPresent(renderer);
                            Mix_PlayChannel(-1, soundStart, 0);
                            SDL_Delay(2000);
                            resetGame(player, bullets, enemies, enemyBullets, explosions, enemyWaveCount);
                        } else if (selectedOption == 1) {
                            gameMode = BOSS;
                            SDL_RenderClear(renderer);
                            SDL_RenderCopy(renderer, startTexture, NULL, NULL);
                            SDL_RenderPresent(renderer);
                            Mix_PlayChannel(-1, soundStart, 0);
                            SDL_Delay(2000);
                            resetGame(player, bullets, enemies, enemyBullets, explosions, enemyWaveCount);
                            initBoss(boss);
                        } else if (selectedOption == 2) {
                            running = false;
                        }
                    }
                }
            }
        }

        if (gameMode == MENU) {
            renderMenu(renderer, font, selectedOption, highScore, menuBackgroundTexture);
            SDL_Delay(16);
            continue;
        }

        updateExplosions(explosions);

        if (gameMode == SURVIVAL) {
            const Uint8* keystate = SDL_GetKeyboardState(NULL);
            if (keystate[SDL_SCANCODE_LEFT]) player.moveLeft();
            if (keystate[SDL_SCANCODE_RIGHT]) player.moveRight();
            if (keystate[SDL_SCANCODE_UP]) player.moveUp();
            if (keystate[SDL_SCANCODE_DOWN]) player.moveDown();

            if (bulletCooldown > 0) bulletCooldown--;
            if (keystate[SDL_SCANCODE_SPACE] && bulletCooldown == 0) {
                bullets.push_back({ player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2, player.y, BULLET_WIDTH, BULLET_HEIGHT, true });
                bulletCooldown = 10;
                Mix_PlayChannel(-1, soundShoot, 0);
            }

            if (player.invincible) {
                player.invincibleTimer--;
                if (player.invincibleTimer <= 0) player.invincible = false;
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
                    if (enemy.y > SCREEN_HEIGHT) enemy.active = false;
                }
            }

            for (auto& bullet : bullets) {
                if (bullet.active) {
                    for (auto& enemy : enemies) {
                        if (enemy.active &&
                            bullet.x < enemy.x + enemy.w && bullet.x + bullet.w > enemy.x &&
                            bullet.y < enemy.y + enemy.h && bullet.y + bullet.h > enemy.y) {
                            explosions.push_back({ enemy.x, enemy.y, 0 });
                            enemy.active = false;
                            bullet.active = false;
                            player.score += 10;
                            Mix_PlayChannel(-1, soundExplode, 0);
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
                        player.invincibleTimer = 90;
                        Mix_PlayChannel(-1, soundHit, 0);

                        if (player.lives <= 0) {
                            if (player.score > highScore) {
                                highScore = player.score;
                                ofstream out("highscore.txt");
                                out << highScore;
                                out.close();
                            }

                            SDL_RenderClear(renderer);
                            SDL_RenderCopy(renderer, gameOverTexture, NULL, NULL);
                            renderText(renderer, font, "Score: " + to_string(player.score), SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50);
                            renderText(renderer, font, "Press enter to continue", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 100);
                            SDL_RenderPresent(renderer);
                            Mix_PlayChannel(-1, soundGameOver, 0);

                            bool waiting = true;
                            while (waiting) {
                                while (SDL_PollEvent(&event)) {
                                    if (event.type == SDL_QUIT) {
                                        running = false;
                                        waiting = false;
                                    }
                                    if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) {
                                        waiting = false;
                                    }
                                }
                                SDL_Delay(16);
                            }

                            gameMode = MENU;
                            resetGame(player, bullets, enemies, enemyBullets, explosions, enemyWaveCount);
                        }
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

            for (const auto& explosion : explosions) {
                SDL_Rect rect = { explosion.x, explosion.y, ENEMY_WIDTH, ENEMY_HEIGHT };
                SDL_RenderCopy(renderer, explosionTexture, NULL, &rect);
            }

            renderScore(renderer, lifeTexture, player.lives, player.score);
            renderText(renderer, font, "Score: " + to_string(player.score), 950, 10);
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
        }

        if (gameMode == BOSS) {
            const Uint8* keystate = SDL_GetKeyboardState(NULL);
            if (keystate[SDL_SCANCODE_LEFT]) player.moveLeft();
            if (keystate[SDL_SCANCODE_RIGHT]) player.moveRight();
            if (keystate[SDL_SCANCODE_UP]) player.moveUp();
            if (keystate[SDL_SCANCODE_DOWN]) player.moveDown();

            if (bulletCooldown > 0) bulletCooldown--;
            if (keystate[SDL_SCANCODE_SPACE] && bulletCooldown == 0) {
                bullets.push_back({ player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2, player.y, BULLET_WIDTH, BULLET_HEIGHT, true });
                bulletCooldown = 10;
                Mix_PlayChannel(-1, soundShoot, 0);
            }

            if (player.invincible) {
                player.invincibleTimer--;
                if (player.invincibleTimer <= 0) player.invincible = false;
            }

            updateBoss(boss, player, explosions, enemyShootCounter);

            for (auto& bullet : bullets) {
                if (bullet.active) {
                    bullet.y -= 10;
                    if (bullet.y < 0) bullet.active = false;

                    if (boss.health > 0) {
                        SDL_Rect bRect = { bullet.x, bullet.y, bullet.w, bullet.h };
                        SDL_Rect bossRect = { boss.x, boss.y, BOSS_WIDTH, BOSS_HEIGHT };
                        if (SDL_HasIntersection(&bRect, &bossRect)) {
                            bullet.active = false;
                            if (boss.state != BOSS_SHIELDED) {
                                boss.health -= 10;
                                if (boss.health <= 0) {
                                    explosions.push_back({ boss.x, boss.y, 0 });
                                    player.score += 500;
                                    Mix_PlayChannel(-1, soundExplode, 0);
                                } else {
                                    Mix_PlayChannel(-1, soundHit, 0);
                                }
                            }
                        }
                    }

                    for (auto& minion : boss.minions) {
                        if (minion.active) {
                            SDL_Rect mRect = { minion.x, minion.y, minion.w, minion.h };
                            SDL_Rect bRect = { bullet.x, bullet.y, bullet.w, bullet.h };
                            if (SDL_HasIntersection(&bRect, &mRect)) {
                                bullet.active = false;
                                minion.active = false;
                                explosions.push_back({ minion.x, minion.y, 0 });
                                player.score += 10;
                                Mix_PlayChannel(-1, soundExplode, 0);
                            }
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
                        player.invincibleTimer = 90;
                        Mix_PlayChannel(-1, soundHit, 0);
                    }
                }
            }

            if (player.lives <= 0) {
                if (player.score > highScore) {
                    highScore = player.score;
                    ofstream out("highscore.txt");
                    out << highScore;
                    out.close();
                }

                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, gameOverTexture, NULL, NULL);
                renderText(renderer, font, "Score: " + to_string(player.score), SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50);
                renderText(renderer, font, "Press enter to continue", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 100);
                SDL_RenderPresent(renderer);
                Mix_PlayChannel(-1, soundGameOver, 0);

                bool waiting = true;
                while (waiting) {
                    while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_QUIT) {
                            running = false;
                            waiting = false;
                        }
                        if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) {
                            waiting = false;
                        }
                    }
                    SDL_Delay(16);
                }

                gameMode = MENU;
                resetGame(player, bullets, enemies, enemyBullets, explosions, enemyWaveCount);
                initBoss(boss);
            }

            SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);

            SDL_Rect playerRect = { player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT };
            SDL_RenderCopy(renderer, playerTexture, NULL, &playerRect);

            for (const auto& bullet : bullets) {
                if (bullet.active) {
                    SDL_Rect rect = { bullet.x, bullet.y, BULLET_WIDTH, BULLET_HEIGHT };
                    SDL_RenderCopy(renderer, bulletTexture, NULL, &rect);
                }
            }

            renderBoss(renderer, boss, bossTexture, bossShieldTexture, laserTexture, bossMissileTexture, enemyTexture);

            for (const auto& eBullet : enemyBullets) {
                if (eBullet.active) {
                    SDL_Rect rect = { eBullet.x, eBullet.y, eBullet.w, eBullet.h };
                    SDL_RenderCopy(renderer, enemyBulletTexture, NULL, &rect);
                }
            }

            for (const auto& explosion : explosions) {
                SDL_Rect rect = { explosion.x, explosion.y, ENEMY_WIDTH, ENEMY_HEIGHT };
                SDL_RenderCopy(renderer, explosionTexture, NULL, &rect);
            }

            renderScore(renderer, lifeTexture, player.lives, player.score);
            renderText(renderer, font, "Diem: " + to_string(player.score), 950, 10);

            if (boss.health <= 0) {
                renderText(renderer, font, "VICTORY! Press enter to continue", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2);
                if (keystate[SDL_SCANCODE_ESCAPE]) {
                    gameMode = MENU;
                    initBoss(boss);
                }
            }

            SDL_RenderPresent(renderer);
            SDL_Delay(16);
        }
    }

    Mix_FreeChunk(soundHit);
    Mix_FreeChunk(soundExplode);
    Mix_FreeChunk(soundShoot);
    Mix_FreeChunk(soundGameOver);
    Mix_FreeChunk(soundStart);
    Mix_CloseAudio();

    TTF_CloseFont(font);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(enemyBulletTexture);
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(lifeTexture);
    SDL_DestroyTexture(explosionTexture);
    SDL_DestroyTexture(startTexture);
    SDL_DestroyTexture(gameOverTexture);
    SDL_DestroyTexture(bossTexture);
    SDL_DestroyTexture(bossShieldTexture);
    SDL_DestroyTexture(missileTexture);
    SDL_DestroyTexture(bossMissileTexture);
    SDL_DestroyTexture(laserTexture);
    SDL_DestroyTexture(menuBackgroundTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
