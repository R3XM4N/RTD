#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>        // for pow
#include <time.h>        // if you need time-based random
#include <stdlib.h>      // for malloc/free
#include "sdl.h"
#include "system.h"

// Window size constants
static const int WINDOW_WIDTH  = 1472;
static const int WINDOW_HEIGHT = 768;

// Forward declarations for any functions or types you have:
GAME_STATE* initGame(void);
Level* initLevel(int id, int pathCount, int maxTurrets, Position* nodes, SDL_Texture* bgTexture);
Enemy* createEnemies(int wave, SDL_Texture* enemyTex);
int calculateEnemiesToSpawn(int wave);
void move(Enemy* e, Level* level, GAME_STATE* game, Mix_Chunk* enemySound);
void turretShoot(Turret* turret, Enemy* enemies, int enemyCount, GAME_STATE* game);
bool positionOnTurret(int mouseX, int mouseY, Turret* turret);
void upgradeTurret(Turret* turret, GAME_STATE* game, SDL_Renderer* renderer, Mix_Chunk* uiAudio[]);
SDL_Texture* loadTexture(const char* filePath, SDL_Renderer* renderer);
SDL_Texture* renderText(const char* message, const char* fontFile, SDL_Color color, int fontSize, SDL_Renderer* renderer);

// The main SDL global variables (avoid global if possible, but consistent with your code)
static SDL_Window*   window            = NULL;
static SDL_Renderer* renderer          = NULL;
static SDL_Texture*  enemyTexture      = NULL;
static SDL_Texture*  backgroundTexture = NULL;

static Mix_Chunk* enemySound     = NULL;
static Mix_Music* backgroundMusic= NULL;
static Mix_Chunk* uiAudio[4]     = {NULL, NULL, NULL, NULL}; // 0=yes, 1=no, 2=win, 3=lose

static SDL_Color redWhiteColor = {255, 128, 128, 255};
static SDL_Color darkColor     = {  0,   0,   0, 255};

/* 
 * Main game entry point.
 */
int main(int argc, char* argv[]) 
{
    // 1. Initialize your game state struct.
    GAME_STATE* game = initGame();
    if (!game) {
        fprintf(stderr, "Game init failed!\n");
        return 1;
    }

    // 2. Initialize SDL subsystems (Video, TTF, Audio).
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL Initialization Error: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // 3. Create the main window.
    window = SDL_CreateWindow(
        "RTD: A Tower Defense!",
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "Window Creation Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // 4. Create an accelerated renderer.
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer Creation Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // 5. Initialize SDL_Image for PNG support
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "SDL_image Initialization Error: %s\n", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // 6. Load background texture
    backgroundTexture = loadTexture("assets/sprites/backgroundv4.png", renderer);
    if (!backgroundTexture) {
        fprintf(stderr, "Failed to load background texture\n");
        // cleanup
        IMG_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // 7. Initialize SDL_mixer for audio
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "SDL_mixer Initialization Error: %s\n", Mix_GetError());
        SDL_DestroyTexture(backgroundTexture);
        IMG_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // 8. Load background music
    backgroundMusic = Mix_LoadMUS("assets/sfx/background.wav");
    if (!backgroundMusic) {
        fprintf(stderr, "Loading background music failed! SDL_mixer Error: %s\n", Mix_GetError());
        Mix_CloseAudio();
        SDL_DestroyTexture(backgroundTexture);
        IMG_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // 9. Load enemy texture
    enemyTexture = loadTexture("assets/sprites/t1enemy.png", renderer);
    if (!enemyTexture) {
        fprintf(stderr, "Failed to load enemy texture\n");
        Mix_FreeMusic(backgroundMusic);
        Mix_CloseAudio();
        SDL_DestroyTexture(backgroundTexture);
        IMG_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // 10. Play background music (infinite loop)
    Mix_PlayMusic(backgroundMusic, -1);

    // 11. Load enemy sfx
    enemySound = Mix_LoadWAV("assets/sfx/enemy.wav");
    if (!enemySound) {
        fprintf(stderr, "Failed to load enemy sound: %s\n", Mix_GetError());
        // not a fatal error: you can keep going if you want
    }

    // 12. Define path nodes
    Position nodes[] = {
        {0, 64*9}, {64*3, 64*9}, {64*3, 64*3}, {64*6, 64*3},
        {64*6,64*7}, {64*19,64*7}, {64*19,64*4}, {64*16,64*4},
        {64*16,64*9}, {64*13,64*12}
    };

    // 13. Initialize the level 
    game->level = initLevel(0, 10, 7, nodes, backgroundTexture);

    // 14. Set up turrets
    game->turrets = (Turret*)malloc(sizeof(Turret) * game->level->maxTurrets);
    if (!game->turrets) {
        fprintf(stderr, "Failed to allocate turrets array!\n");
        // handle or bail out
    }

    // Example turrets
    // param format: (x,y) pos, current-cooldown, speed, type, dmg, range, price, texture, sfx
    // Electric turret
    game->turrets[0] = (Turret){{32*9, 32*9}, 0, 12, 0, 20, 160, 125, 
        loadTexture("assets/sprites/electricTurretBox.png",renderer),NULL};
    game->turrets[1] = (Turret){{32*17, 32*11}, 0, 12, 0, 20, 160, 125, 
        loadTexture("assets/sprites/electricTurretBox.png",renderer),NULL};
    game->turrets[2] = (Turret){{32*35, 32*11}, 0, 12, 0, 20, 160, 125, 
        loadTexture("assets/sprites/electricTurretBox.png",renderer),NULL};
    game->turrets[3] = (Turret){{32*29, 32*17}, 0, 12, 0, 20, 160, 125, 
        loadTexture("assets/sprites/electricTurretBox.png",renderer),NULL};
    // Sniper turret
    game->turrets[4] = (Turret){{32*29, 32*11}, 0, 24, 4, 200, 280, 1000,
        loadTexture("assets/sprites/sniperTurretBox.png",renderer),NULL};
    game->turrets[5] = (Turret){{32*9, 32*17}, 0, 24, 4, 200, 280, 1000, 
        loadTexture("assets/sprites/sniperTurretBox.png",renderer),NULL};
    game->turrets[6] = (Turret){{32*1, 32*23}, 0, 24, 4, 200, 280,  400, 
        loadTexture("assets/sprites/sniperTurretBox.png",renderer),NULL};

    // 15. Load UI SFX
    uiAudio[0] = Mix_LoadWAV("assets/sfx/yes.wav");
    uiAudio[1] = Mix_LoadWAV("assets/sfx/no.wav");
    uiAudio[2] = Mix_LoadWAV("assets/sfx/win.wav");
    uiAudio[3] = Mix_LoadWAV("assets/sfx/loose.wav");

    // 16. Game loop
    bool quit      = false;
    bool gameover  = false;
    bool endScreen = false;

    SDL_Event e; // for event polling

    while (!quit) {
        // poll events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = e.button.x;
                int mouseY = e.button.y;
                // turret upgrades if clicked
                for (int i = 0; i < game->level->maxTurrets; i++) {
                    if (positionOnTurret(mouseX, mouseY, &game->turrets[i])) {
                        upgradeTurret(&game->turrets[i], game, renderer, uiAudio);
                    }
                }
            }
        }

        if (!gameover) {
            // spawn or move enemies
            int enemyCount = calculateEnemiesToSpawn(game->wave);
            // if no enemies allocated, create them now
            if (game->enemies == NULL) {
                game->enemies = createEnemies(game->wave, enemyTexture);
            }

            // track how many alive
            int enemiesLeft = 0;
            for (int i = 0; i < enemyCount; i++) {
                if (game->enemies[i].alive) {
                    enemiesLeft++;
                }
            }

            // if all dead, next wave
            if (enemiesLeft == 0) {
                game->currency += game->wave * 10;
                game->wave++;
                free(game->enemies); 
                game->enemies = NULL;

                enemyCount = calculateEnemiesToSpawn(game->wave);
                game->enemies = createEnemies(game->wave, enemyTexture);
            }

            // move enemies
            for (int i = 0; i < enemyCount; i++) {
                if (game->enemies[i].alive) {
                    move(&game->enemies[i], game->level, game, enemySound);
                }
            }

            // turret shooting logic
            for (int i = 0; i < game->level->maxTurrets; i++) {
                turretShoot(&game->turrets[i], game->enemies, enemyCount, game);
            }

            // Rendering
            SDL_SetRenderDrawColor(renderer, 172, 79, 198, 255);
            SDL_RenderClear(renderer);

            // draw background
            SDL_Rect backgroundRect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderCopy(renderer, game->level->texture, NULL, &backgroundRect);

            // draw enemies
            for (int i = 0; i < enemyCount; i++) {
                if (game->enemies[i].alive) {
                    SDL_Rect enemyRect = {
                        game->enemies[i].position.x - 20, 
                        game->enemies[i].position.y - 20, 
                        40, 
                        40
                    };
                    SDL_RenderCopy(renderer, game->enemies[i].texture, NULL, &enemyRect);

                    // small healthbar above enemy
                    float waveScalingHealth = ((140.0f * powf(1.2f, (float)game->wave - 1)) / (powf(1.12f, (float)game->wave)));
                    SDL_Rect healthBarRect = {
                        game->enemies[i].position.x - 20,
                        game->enemies[i].position.y - 30,
                        (int)(40.0f * ( (float)game->enemies[i].health / waveScalingHealth)),
                        5
                    };
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    SDL_RenderFillRect(renderer, &healthBarRect);
                }
            }

            // draw turrets
            for (int i = 0; i < game->level->maxTurrets; i++) {
                if (game->turrets[i].texture) {
                    SDL_Rect turretRect = {
                        game->turrets[i].position.x - 20,
                        game->turrets[i].position.y - 20,
                        40,
                        40
                    };
                    SDL_RenderCopy(renderer, game->turrets[i].texture, NULL, &turretRect);
                }
            }

            // Overlays / HUD text
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);

            // wave
            {
                char buffer[50];
                snprintf(buffer, sizeof(buffer), "Wave: %d", game->wave);
                SDL_Texture* waveTex = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 40, renderer);
                if (waveTex) {
                    int texW=0, texH=0;
                    SDL_QueryTexture(waveTex, NULL, NULL, &texW, &texH);
                    SDL_Rect dstRect = {
                        (WINDOW_WIDTH/2) - (texW/2),
                        10,
                        texW,
                        texH
                    };
                    SDL_RenderCopy(renderer, waveTex, NULL, &dstRect);
                    SDL_DestroyTexture(waveTex);
                }
            }

            // HP, currency, enemies left
            {
                SDL_Texture* gui[3] = {NULL, NULL, NULL};
                char buffer[50];

                // HP
                snprintf(buffer, sizeof(buffer), "HP: %d", game->health);
                gui[0] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 30, renderer);
                
                // currency
                snprintf(buffer, sizeof(buffer), "Currency: %d", game->currency);
                gui[1] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 30, renderer);
                
                // enemies left
                int enemiesLeft = 0;
                int enemyCount = calculateEnemiesToSpawn(game->wave);
                if (game->enemies) {
                    for (int i = 0; i < enemyCount; i++) {
                        if (game->enemies[i].alive) enemiesLeft++;
                    }
                }
                snprintf(buffer, sizeof(buffer), "Enemies left: %d", enemiesLeft);
                gui[2] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 30, renderer);

                for (int i = 0; i < 3; i++) {
                    if (gui[i]) {
                        int texW=0, texH=0;
                        SDL_QueryTexture(gui[i], NULL, NULL, &texW, &texH);
                        SDL_Rect dstRect = {10, 10 + i*35, texW, texH};
                        SDL_RenderCopy(renderer, gui[i], NULL, &dstRect);
                        SDL_DestroyTexture(gui[i]);
                    }
                }
            }

            // mouse position
            {
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "Mouse: %d, %d", mouseX, mouseY);
                SDL_Texture* mouseText = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 24, renderer);
                if (mouseText) {
                    int texW=0, texH=0;
                    SDL_QueryTexture(mouseText, NULL, NULL, &texW, &texH);
                    SDL_Rect dstRect = {
                        WINDOW_WIDTH - texW - 10,
                        10,
                        texW,
                        texH
                    };
                    SDL_RenderCopy(renderer, mouseText, NULL, &dstRect);
                    SDL_DestroyTexture(mouseText);
                }
            }

            // turret info on hover
            for (int i = 0; i < game->level->maxTurrets; i++) {
                if (positionOnTurret(mouseX, mouseY, &game->turrets[i])) {
                    SDL_Texture* turretInfo[4] = {NULL, NULL, NULL, NULL};
                    char buffer[64];

                    snprintf(buffer, sizeof(buffer), "Speed: %d",  game->turrets[i].speed);
                    turretInfo[0] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 24, renderer);
                    
                    snprintf(buffer, sizeof(buffer), "Damage: %d", game->turrets[i].damage);
                    turretInfo[1] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 24, renderer);
                    
                    snprintf(buffer, sizeof(buffer), "Range: %d",  game->turrets[i].range);
                    turretInfo[2] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 24, renderer);
                    
                    snprintf(buffer, sizeof(buffer), "Price: %d",  game->turrets[i].price);
                    turretInfo[3] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 24, renderer);

                    for (int j = 0; j < 4; j++) {
                        if (turretInfo[j]) {
                            int texW=0, texH=0;
                            SDL_QueryTexture(turretInfo[j], NULL, NULL, &texW, &texH);
                            SDL_Rect dstRect = {
                                WINDOW_WIDTH - texW - 10,
                                40 + j * 30,
                                texW,
                                texH
                            };
                            SDL_RenderCopy(renderer, turretInfo[j], NULL, &dstRect);
                            SDL_DestroyTexture(turretInfo[j]);
                        }
                    }
                }
            }

            // check gameover
            if (game->health <= 0) {
                gameover = true;
            }

        } else {
            // Game Over or Win screen
            Mix_HaltMusic(); // stop music if not already stopped
            char buffer[64];
            if (game->wave > 30) {
                snprintf(buffer, sizeof(buffer), "You've won!");
                // play win sfx once
                if (uiAudio[2] && !endScreen) {
                    Mix_PlayChannel(-1, uiAudio[2], 0);
                    endScreen = true;
                }
            } else {
                snprintf(buffer, sizeof(buffer), "You've lost!");
                // play lose sfx once
                if (uiAudio[3] && !endScreen) {
                    Mix_PlayChannel(-1, uiAudio[3], 0);
                    endScreen = true;
                }
            }

            // Render black screen with large text
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            // "Game Over" or "You Won" text
            {
                SDL_Texture* gameOverTexture = renderText(buffer, "assets/fonts/Arial.ttf", redWhiteColor, 72, renderer);
                if (gameOverTexture) {
                    int texW=0, texH=0;
                    SDL_QueryTexture(gameOverTexture, NULL, NULL, &texW, &texH);
                    SDL_Rect dstRect = {
                        WINDOW_WIDTH/2 - texW/2,
                        WINDOW_HEIGHT/2 - texH/2 - 50,
                        texW,
                        texH
                    };
                    SDL_RenderCopy(renderer, gameOverTexture, NULL, &dstRect);
                    SDL_DestroyTexture(gameOverTexture);
                }
            }

            // Wave info
            {
                SDL_Texture* lastWaveTexture = NULL;
                if (game->wave < 30) {
                    snprintf(buffer, sizeof(buffer), "Losing wave: %d", game->wave);
                } else {
                    snprintf(buffer, sizeof(buffer), "Beaten waves: %d", game->wave);
                }
                lastWaveTexture = renderText(buffer, "assets/fonts/Arial.ttf", redWhiteColor, 48, renderer);
                if (lastWaveTexture) {
                    int texW=0, texH=0;
                    SDL_QueryTexture(lastWaveTexture, NULL, NULL, &texW, &texH);
                    SDL_Rect dstRect = {
                        WINDOW_WIDTH/2 - texW/2,
                        WINDOW_HEIGHT/2 - texH/2 + 50,
                        texW,
                        texH
                    };
                    SDL_RenderCopy(renderer, lastWaveTexture, NULL, &dstRect);
                    SDL_DestroyTexture(lastWaveTexture);
                }
            }
        }

        SDL_RenderPresent(renderer);

        // Minimal sleeping logic: slow down or speed up frames
        if (!gameover) {
            if      (game->wave <= 10)  SDL_Delay(16); // ~60 FPS
            else if (game->wave <= 20)  SDL_Delay(15);
            else if (game->wave <= 30)  SDL_Delay(12);
            else                        SDL_Delay(8);
        } else {
            SDL_Delay(16); // keep refreshing
        }
    } // end while(!quit)

    // Cleanup / freeing memory
    for (int i = 0; i < game->level->maxTurrets; i++) {
        if (game->turrets[i].texture) SDL_DestroyTexture(game->turrets[i].texture);
        if (game->turrets[i].turretShootSound) Mix_FreeChunk(game->turrets[i].turretShootSound);
    }
    if (game->turrets) free(game->turrets);
    if (game->enemies) free(game->enemies);

    if (game->level) {
        // free fields inside level if needed
        free(game->level);
    }
    free(game);

    if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);
    if (enemyTexture) SDL_DestroyTexture(enemyTexture);

    if (backgroundMusic) Mix_FreeMusic(backgroundMusic);
    if (enemySound)      Mix_FreeChunk(enemySound);

    for (int i = 0; i < 4; i++) {
        if (uiAudio[i]) Mix_FreeChunk(uiAudio[i]);
    }

    Mix_CloseAudio(); 
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
    return 0;
}
