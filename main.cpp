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
using namespace std;

enum GameMode {
    MENU,
    SURVIVAL,
    BOSS,
    EXIT
};

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
int highScore = 0;

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

void renderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedOption, int highScore) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    string options[3] = { "1. Survival", "2. Campaign", "3. Exit" };

    renderText(renderer, font, "Hight score: " + to_string(highScore), SCREEN_WIDTH/2 - 80, 150);

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

    // Load sound
    Mix_Chunk* soundStart = Mix_LoadWAV("fight.mp3");
    Mix_Chunk* soundExplode = Mix_LoadWAV("địch nổ.mp3");
    Mix_Chunk* soundGameOver = Mix_LoadWAV("over.mp3");
    Mix_Chunk* soundHit = Mix_LoadWAV("trúng đạn.mp3");
    Mix_Chunk* soundShoot = Mix_LoadWAV("voice đạn.mp3");

    SDL_Window* window = SDL_CreateWindow("Space Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font* font = TTF_OpenFont("PixelFont.ttf", 25);
    if (!font) {
        cout << "Khong the tai font: " << TTF_GetError() << endl;
        return -1;
    }

    // Load textures
    SDL_Texture* playerTexture = IMG_LoadTexture(renderer, "tàu.png");
    SDL_Texture* bulletTexture = IMG_LoadTexture(renderer, "đạn.png");
    SDL_Texture* enemyTexture = IMG_LoadTexture(renderer, "địch.png");
    SDL_Texture* backgroundTexture = IMG_LoadTexture(renderer, "nền.png");
    SDL_Texture* enemyBulletTexture = IMG_LoadTexture(renderer, "đạn địch.png");
    SDL_Texture* lifeTexture = IMG_LoadTexture(renderer, "tàu.png");
    SDL_Texture* explosionTexture = IMG_LoadTexture(renderer, "địch nổ.png");
    SDL_Texture* startTexture = IMG_LoadTexture(renderer, "fight.png");
    SDL_Texture* gameOverTexture = IMG_LoadTexture(renderer, "over.jpg");

    // Load high score
    ifstream in("highscore.txt");
    if (in) {
        in >> highScore;
        in.close();
    }

    Player player = { SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT - 10 };
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
                        } else if (selectedOption == 2) {
                            running = false;
                        }
                    }
                }
            }
        }

        if (gameMode == MENU) {
            renderMenu(renderer, font, selectedOption, highScore);
            SDL_Delay(16);
            continue;
        }

        if (gameMode == BOSS) {
            SDL_RenderClear(renderer);
            renderText(renderer, font, "CHE DO BOSS - DANG PHAT TRIEN!", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2);
            renderText(renderer, font, "NHAN ESC DE VE MENU", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 50);
            SDL_RenderPresent(renderer);

            const Uint8* keys = SDL_GetKeyboardState(NULL);
            if (keys[SDL_SCANCODE_ESCAPE]) {
                gameMode = MENU;
            }
            continue;
        }

        // Game logic (SURVIVAL mode)
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
                        explosions.push_back({ enemy.x, enemy.y });
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
                        // Update high score
                        if (player.score > highScore) {
                            highScore = player.score;
                            ofstream out("highscore.txt");
                            out << highScore;
                            out.close();
                        }

                        // Show game over screen
                        SDL_RenderClear(renderer);
                        SDL_RenderCopy(renderer, gameOverTexture, NULL, NULL);
                        renderText(renderer, font, "DIEM SO: " + to_string(player.score), SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50);
                        renderText(renderer, font, "NHAN PHIM BAT KY DE VE MENU", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 100);
                        SDL_RenderPresent(renderer);
                        Mix_PlayChannel(-1, soundGameOver, 0);

                        // Wait for any key press
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

                        // Return to menu
                        gameMode = MENU;
                        resetGame(player, bullets, enemies, enemyBullets, explosions, enemyWaveCount);
                    }
                }
            }
        }

        // Clean up inactive objects
        bullets.erase(remove_if(bullets.begin(), bullets.end(), [](const GameObject& b) { return !b.active; }), bullets.end());
        enemies.erase(remove_if(enemies.begin(), enemies.end(), [](const GameObject& e) { return !e.active; }), enemies.end());
        enemyBullets.erase(remove_if(enemyBullets.begin(), enemyBullets.end(), [](const GameObject& b) { return !b.active; }), enemyBullets.end());
        explosions.erase(remove_if(explosions.begin(), explosions.end(), [](const Explosion& e) { return e.frame > 15; }), explosions.end());

        // Rendering
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

        renderScore(renderer, lifeTexture, player.lives, player.score);
        renderText(renderer, font, "Diem: " + to_string(player.score), 950, 10);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Clean up
    Mix_FreeChunk(soundStart);
    Mix_FreeChunk(soundExplode);
    Mix_FreeChunk(soundGameOver);
    Mix_FreeChunk(soundHit);
    Mix_FreeChunk(soundShoot);
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
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
