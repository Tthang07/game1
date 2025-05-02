#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "SDL_mixer.h"
#include "game.h"
#include "player.h"
#include "enemy.h"
#include <iostream>
#include <fstream>

using namespace std;

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
    SDL_Texture* explosionTexture = IMG_LoadTexture(renderer, "Explosion.png");

    // Initialize player, enemies, bullets
    Player player;
    vector<GameObject> bullets;
    vector<GameObject> enemies;
    vector<GameObject> enemyBullets;
    vector<Explosion> explosions;

    int enemyWaveCount = 0;
    bool gameRunning = true;

    // Main game loop
    while (gameRunning) {
        // Handle events and game logic here
    }

    // Clean up and quit
    Mix_FreeChunk(soundStart);
    Mix_FreeChunk(soundExplode);
    Mix_FreeChunk(soundGameOver);
    Mix_FreeChunk(soundHit);
    Mix_FreeChunk(soundShoot);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(explosionTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    IMG_Quit();
    TTF_Quit();
    Mix_Quit();

    return 0;
}
