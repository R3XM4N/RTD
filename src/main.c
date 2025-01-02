#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "sdl.h"
#include "system.h"

const int WINDOW_WIDTH = 1472;
const int WINDOW_HEIGHT = 768;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* enemyTexture = NULL;
SDL_Texture* backgroundTexture = NULL;
Mix_Chunk* enemySound = NULL;
Mix_Music* backgroundMusic = NULL;
Mix_Chunk* uiAudio[4] = {NULL,NULL,NULL,NULL}; // 0 yes 1 no 2 win 3 lose
SDL_Color redWhiteColor = {255, 128, 128, 255};
SDL_Color darkColor = {0, 0, 0, 255};

int main() {
    GAME_STATE* game = initGame();
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL Initialization Error: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() == -1) {
        printf("TTF_Init: %s\n", TTF_GetError());
        return 1;
    }
    window = SDL_CreateWindow("RTD: A Tower Defense!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window Creation Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer Creation Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image Initialization Error: %s\n", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    backgroundTexture = loadTexture("assets/sprites/backgroundv4.png", renderer);
    if (!backgroundTexture) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer Initialization Error: %s\n", Mix_GetError());
        SDL_DestroyTexture(enemyTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    backgroundMusic = Mix_LoadMUS("assets/sfx/background.wav");
    if (!backgroundMusic) {
        printf("Loading background music failed! SDL_mixer Error: %s\n", Mix_GetError());
        Mix_CloseAudio();
        SDL_DestroyTexture(enemyTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    enemyTexture = loadTexture("assets/sprites/t1enemy.png", renderer);
    if (!enemyTexture) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    Mix_PlayMusic(backgroundMusic, -1);
    enemySound =  Mix_LoadWAV("assets/sfx/enemy.wav");
    
    Position nodes[] = {{0, 64*9}, {64*3, 64*9}, {64*3, 64*3}, {64*6, 64*3},{64*6,64*7},{64*19,64*7},{64*19,64*4},{64*16,64*4},{64*16,64*9},{64*13,64*12}};
    game->level = initLevel(0, 10, 7, nodes, backgroundTexture);
    
    //Turrets position, cooldown, speed, type, damage, range, price, texture, SFX
    game->turrets = malloc(sizeof(Turret) * game->level->maxTurrets);
    game->turrets[0] = (Turret){{32*9, 32*9}, 0, 12, 0, 20, 160, 125, loadTexture("assets/sprites/electricTurretBox.png",renderer),NULL};
    game->turrets[1] = (Turret){{32*17, 32*11}, 0, 12, 0, 20, 160, 125, loadTexture("assets/sprites/electricTurretBox.png",renderer),NULL};
    game->turrets[2] = (Turret){{32*35, 32*11}, 0, 12, 0, 20, 160, 125, loadTexture("assets/sprites/electricTurretBox.png",renderer),NULL};
    game->turrets[3] = (Turret){{32*29, 32*17}, 0, 12, 0, 20, 160, 125, loadTexture("assets/sprites/electricTurretBox.png",renderer),NULL};
    game->turrets[4] = (Turret){{32*29, 32*11}, 0, 24, 4, 200, 280, 1000, loadTexture("assets/sprites/sniperTurretBox.png",renderer),NULL};
    game->turrets[5] = (Turret){{32*9, 32*17}, 0, 24, 4, 200, 280, 1000, loadTexture("assets/sprites/sniperTurretBox.png",renderer),NULL};
    game->turrets[6] = (Turret){{32*1, 32*23}, 0, 24, 4, 200, 280, 400, loadTexture("assets/sprites/sniperTurretBox.png",renderer),NULL};

    //UI SFX
    uiAudio[0] = Mix_LoadWAV("assets/sfx/yes.wav");
    uiAudio[1] = Mix_LoadWAV("assets/sfx/no.wav");
    uiAudio[2] = Mix_LoadWAV("assets/sfx/win.wav");
    uiAudio[3] = Mix_LoadWAV("assets/sfx/loose.wav");
    //GAME LOOP
    bool quit = false;
    bool gameover = false;
    bool endScreen = false;
    SDL_Event e;
    while (!quit) {
        //SDL_Log("Game loop");
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = e.button.x;
                int mouseY = e.button.y;
                for (int i = 0; i < game->level->maxTurrets; i++) {
                    if (positionOnTurret(mouseX, mouseY, &game->turrets[i])) {
                        upgradeTurret(&game->turrets[i],game,renderer,uiAudio);
                    }
                }
            }
        }
        if (!gameover)
        {
                int time = SDL_GetTicks64();
            int enemiesLeft = 0;
            int enemyCount = calculateEnemiesToSpawn(game->wave);
                if (game->enemies == NULL) {
                    game->enemies = createEnemies(game->wave, enemyTexture);
                }

            for (int i = 0; i < enemyCount; i++) {
                if (game->enemies[i].alive) {
                    enemiesLeft++;
                }
            }

            if (enemiesLeft == 0) {
                game->currency += game->wave * 10;
                enemyCount = calculateEnemiesToSpawn(++game->wave);
                free(game->enemies);
                game->enemies = createEnemies(game->wave, enemyTexture);
            }

            for (int i = 0; i < enemyCount; i++) {
                if (game->enemies[i].alive) {
                    move(&game->enemies[i], game->level, game, enemySound);
                }
            }

            // Turret shooting logic
            for (int i = 0; i < game->level->maxTurrets; i++) {
                turretShoot(&game->turrets[i], game->enemies, enemyCount, game);
            }
            
            SDL_SetRenderDrawColor(renderer, 172, 79, 198, 255);
            SDL_RenderClear(renderer);

            SDL_Rect backgroundRect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderCopy(renderer, game->level->texture, NULL, &backgroundRect);
            
            for (int i = 0; i < enemyCount; i++) {
                if (game->enemies[i].alive) {
                    SDL_Rect enemyRect = {game->enemies[i].position.x-20, game->enemies[i].position.y-20, 40, 40};
                    SDL_RenderCopy(renderer, game->enemies[i].texture, NULL, &enemyRect);
                    SDL_Rect healthBarRect = {game->enemies[i].position.x - 20, game->enemies[i].position.y - 30, (int)(40 * ((float)game->enemies[i].health / ((140*pow(1.2,game->wave-1))/(pow(1.12,game->wave))))), 5};
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    SDL_RenderFillRect(renderer, &healthBarRect);
                }
            }
            
            for (int i = 0; i < game->level->maxTurrets; i++) {
                if (game->turrets[i].texture != NULL) {
                    SDL_Rect turretRect = {game->turrets[i].position.x - 20, game->turrets[i].position.y - 20, 40, 40};
                    SDL_RenderCopy(renderer, game->turrets[i].texture, NULL, &turretRect);
                }
            }

            //ON SCREEN TEXT
            char buffer[50];
            SDL_Texture* gui[4];
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            sprintf(buffer, "Wave: %d", game->wave);
            gui[0] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 40, renderer);
            sprintf(buffer, "HP: %d", game->health);
            gui[1] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 30, renderer);
            sprintf(buffer, "Currency: %d", game->currency);
            gui[2] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 30, renderer);
            sprintf(buffer, "Enemies left: %d", enemiesLeft);
            gui[3] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 30, renderer);
            //hud
            if (gui[0])
            {
                int texW = 0, texH = 0;
                SDL_QueryTexture(gui[0], NULL, NULL, &texW, &texH);
                SDL_Rect dstRect = {WINDOW_WIDTH/2-texW/2, 10, texW, texH};
                SDL_RenderCopy(renderer, gui[0], NULL, &dstRect);
                SDL_DestroyTexture(gui[0]);
            }
            for (int i = 0; i < 3; i++) {
                if (gui[i+1]) {
                int texW = 0, texH = 0;
                SDL_QueryTexture(gui[i+1], NULL, NULL, &texW, &texH);
                SDL_Rect dstRect = {10, 10 + i * 35, texW, texH};
                SDL_RenderCopy(renderer, gui[i+1], NULL, &dstRect);
                SDL_DestroyTexture(gui[i+1]);
                }
            }
            //moouse position
            sprintf(buffer, "Mouse: %d, %d", mouseX, mouseY);
            SDL_Texture* mouseText = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 24, renderer);
            if (mouseText) {
                int texW = 0, texH = 0;
                SDL_QueryTexture(mouseText, NULL, NULL, &texW, &texH);
                SDL_Rect dstRect = {WINDOW_WIDTH - texW - 10, 10, texW, texH};
                SDL_RenderCopy(renderer, mouseText, NULL, &dstRect);
                SDL_DestroyTexture(mouseText);
            }
            for (int i = 0; i < game->level->maxTurrets; i++) {
                if (positionOnTurret(mouseX, mouseY, &game->turrets[i])) {
                    //speed, damage, range, price
                    SDL_Texture* turretInfo[4] = {NULL, NULL, NULL, NULL};
                    sprintf(buffer, "Speed: %d", game->turrets[i].speed);
                    turretInfo[0] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 24, renderer);
                    sprintf(buffer, "Damage: %d", game->turrets[i].damage);
                    turretInfo[1] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 24, renderer);
                    sprintf(buffer, "Range: %d", game->turrets[i].range);
                    turretInfo[2] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 24, renderer);
                    sprintf(buffer, "Price: %d", game->turrets[i].price);
                    turretInfo[3] = renderText(buffer, "assets/fonts/Arial.ttf", darkColor, 24, renderer);
                    for (int i = 0; i < 4; i++) {
                        if (turretInfo[i]) {
                            int texW = 0, texH = 0;
                            SDL_QueryTexture(turretInfo[i], NULL, NULL, &texW, &texH);
                            SDL_Rect dstRect = {WINDOW_WIDTH-texW-10, 40 + i * 30, texW, texH};
                            SDL_RenderCopy(renderer, turretInfo[i], NULL, &dstRect);
                            SDL_DestroyTexture(turretInfo[i]);
                        }
                    }
                }
            }
            if (game->health <= 0) {
                gameover = true;
            }
        }
        else{
            Mix_HaltMusic();
            char buffer[50];
            if (game->wave > 30){
                sprintf(buffer, "You've won!");
                if (uiAudio[2]!=NULL && !endScreen){
                    Mix_PlayChannel(-1, uiAudio[2], 0);
                    endScreen = true;
                }
            }
            else{
                sprintf(buffer, "You've lost!");
                if (uiAudio[3]!=NULL && !endScreen){
                    Mix_PlayChannel(-1, uiAudio[3], 0);
                    endScreen = true;
                }
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_Texture* gameOverTexture = renderText(buffer, "assets/fonts/Arial.ttf", redWhiteColor, 72, renderer);
            if (gameOverTexture) {
                int texW = 0, texH = 0;
                SDL_QueryTexture(gameOverTexture, NULL, NULL, &texW, &texH);
                SDL_Rect dstRect = {WINDOW_WIDTH / 2 - texW / 2, WINDOW_HEIGHT / 2 - texH / 2 - 50, texW, texH};
                SDL_RenderCopy(renderer, gameOverTexture, NULL, &dstRect);
                SDL_DestroyTexture(gameOverTexture);
            }
            if (game->wave < 30){
                sprintf(buffer, "Loosing wave: %d", game->wave);
            }
            else{
                sprintf(buffer, "Beaten waves: %d", game->wave);
            }
            SDL_Texture* lastWaveTexture = renderText(buffer, "assets/fonts/Arial.ttf", redWhiteColor, 48, renderer);
            if (lastWaveTexture) {
                int texW = 0, texH = 0;
                SDL_QueryTexture(lastWaveTexture, NULL, NULL, &texW, &texH);
                SDL_Rect dstRect = {WINDOW_WIDTH / 2 - texW / 2, WINDOW_HEIGHT / 2 - texH / 2 + 50, texW, texH};
                SDL_RenderCopy(renderer, lastWaveTexture, NULL, &dstRect);
                SDL_DestroyTexture(lastWaveTexture);
            }
        }

        SDL_RenderPresent(renderer);
        //Funny buisness
        if (game->wave <= 10){
             SDL_Delay(16); //+-60fps
        }
        else if (game->wave > 10 && game->wave <= 20){
             SDL_Delay(15); 
        }
        else if (game->wave > 20 && game->wave <= 30){
            SDL_Delay(12);
        }
        else if (game->wave > 20 && game->wave <= 30){
            SDL_Delay(12);
        }
        else if (game->wave > 30){
            SDL_Delay(8);
        }
        else{
            SDL_Delay(16);
        }
    }
    // FREEING MEMORY
    for (int i = 0; i < game->level->maxTurrets; i++)
    {
        SDL_DestroyTexture(game->turrets[i].texture);
        Mix_FreeChunk(game->turrets[i].turretShootSound);
    }
    free(game->enemies);
    free(game->turrets);
    free(game->level);
    free(game);
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_FreeMusic(backgroundMusic);
    Mix_FreeChunk(enemySound);
    for (int i = 0; i < 4; i++) {
        Mix_FreeChunk(uiAudio[i]);
    }
    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();

    return 0;
}