#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "sdl.h"
#include "system.h"

Level* initLevel(int startCurrency, int nodeCount, int maxTurrets, Position* nodes, SDL_Texture* texture) {
    Level* level = malloc(sizeof(Level));
    level->startCurrency = startCurrency;
    level->nodeCount = nodeCount;
    level->maxTurrets = maxTurrets;
    level->nodes = nodes;
    level->texture = texture;
    return level;
}


GAME_STATE* initGame() {
    GAME_STATE* game = malloc(sizeof(GAME_STATE));
    game->wave = 1;
    game->health = 100;
    game->currency = 300;
    game->enemies = NULL;
    game->turrets = NULL;
    game->level = NULL;
    return game;
}
int calculateEnemiesToSpawn(int wave) {
    return (int)(((pow(1.4, wave) * 2) / pow(1.5, wave)) + wave * 1.5);
}
void turretShoot(Turret* turret, Enemy* enemies, int enemyCount, GAME_STATE* game) {
    for (int i = 0; i < enemyCount; i++) {
        if (enemies[i].alive) {
            int distance_x = enemies[i].position.x - turret->position.x;
            int distance_y = enemies[i].position.y - turret->position.y;
            int distance = sqrt(distance_x * distance_x + distance_y * distance_y);

            if (distance <= turret->range && turret->cooldown == 0) {
                //ELECTRIC TURRET
                if (turret->type == 1) {
                    enemies[i].health -= turret->damage;
                    if (enemies[i].health <= 0) {
                        enemies[i].alive = false;
                        game->currency += enemies[i].reward;
                    }
                    turret->cooldown = turret->speed;
                    Mix_PlayChannel(-1, turret->turretShootSound, 0);
                }
                else if (turret->type == 2) {
                    enemies[i].health -= turret->damage*1.5;
                    if (enemies[i].health <= 0) {
                        enemies[i].alive = false;
                        game->currency += enemies[i].reward;
                    }
                    turret->cooldown = turret->speed;
                    Mix_PlayChannel(-1, turret->turretShootSound, 0);
                } 
                else if (turret->type == 3) {
                    enemies[i].health -= turret->damage*2;
                    if (enemies[i].health <= 0) {
                        enemies[i].alive = false;
                        game->currency += enemies[i].reward;
                    }
                    turret->cooldown = turret->speed;
                    Mix_PlayChannel(-1, turret->turretShootSound, 0);
                } 
                //SNIPER TURRET
                else if (turret->type == 5) {
                    enemies[i].health -= turret->damage; 
                    if (enemies[i].health <= 0) {
                        enemies[i].alive = false;
                        game->currency += enemies[i].reward;
                    }
                    turret->cooldown = turret->speed;
                    Mix_PlayChannel(-1, turret->turretShootSound, 0);
                } 
                else if (turret->type == 6) {
                    enemies[i].health -= turret->damage*2; 
                    if (enemies[i].health <= 0) {
                        enemies[i].alive = false;
                        game->currency += enemies[i].reward;
                    }
                    turret->cooldown = turret->speed;
                    Mix_PlayChannel(-1, turret->turretShootSound, 0);
                } 
                else if (turret->type == 7) {
                    enemies[i].health -= turret->damage*4; 
                    if (enemies[i].health <= 0) {
                        enemies[i].alive = false;
                        game->currency += enemies[i].reward;
                    }
                    turret->cooldown = turret->speed;
                    Mix_PlayChannel(-1, turret->turretShootSound, 0);
                }
            }
        }
    }
    if (turret->cooldown > 0) {
        turret->cooldown--;
    }
}
//position, destination, speed, health, damage, reward, alive, texture
Enemy* createEnemies(int wave, SDL_Texture* enemyTexture) {
    int enemyCount = calculateEnemiesToSpawn(wave);
    Enemy* enemies = malloc(sizeof(Enemy) * enemyCount);
    srand(time(NULL));
    for (int i = 0; i < enemyCount; i++) {
        if (i !=0)
        {
            enemies[i] = (Enemy){{enemies[i-1].position.x - (rand() % 101 + 60), 480}, 0, (rand() % 2 + 2)+pow(1.005,wave-1),((140*pow(1.2,wave-1))/(pow(1.12,wave))) , wave, 5, true, enemyTexture};
        }
        else{
            enemies[i] = (Enemy){{-100 - (rand() % 251 + 50), 480},  0, (rand() % 2 + 2)+pow(1.005,wave-1),((140*pow(1.2,wave-1))/(pow(1.12,wave))) , wave, 5, true, enemyTexture};
        }
    }
    return enemies;
}
//interactions
bool positionOnTurret(int mouseX, int mouseY, Turret* turret) {
    return mouseX >= turret->position.x - 20 && mouseX <= turret->position.x + 20 &&
           mouseY >= turret->position.y - 20 && mouseY <= turret->position.y + 20;
}
//SDL

//LOGIC FOR UPGRADING TURRETS AND THEIR TYPES --- ALSO HANDLES CURRENCEY DEDUCTION && TEXTURE CHANGES && STATS CHANGES
void upgradeTurret(Turret* turret,GAME_STATE* game, SDL_Renderer* renderer, Mix_Chunk* uiAudio[4]) {
    //"ZAP  TURRET"
    if (turret->type == 0 && game->currency >= turret->price) {
        turret->type = 1;
        game->currency -= turret->price;
        turret->price = turret->price*1.5;
        SDL_DestroyTexture(turret->texture);
        Mix_FreeChunk(turret->turretShootSound);
        turret->texture = loadTexture("assets/sprites/electricTurretT1.png", renderer);
        turret->turretShootSound = Mix_LoadWAV("assets/sfx/zapTowerA.wav");
        Mix_PlayChannel(-1, uiAudio[0], 0);
    }
    else if (turret->type == 1 && game->currency >= turret->price) {
        turret->type = 2;
        game->currency -= turret->price;
        turret->price = turret->price*1.5;
        turret->speed = turret->speed/2;
        SDL_DestroyTexture(turret->texture);
        Mix_FreeChunk(turret->turretShootSound);
        turret->texture = loadTexture("assets/sprites/electricTurretT2.png", renderer);
        turret->turretShootSound = Mix_LoadWAV("assets/sfx/zapTowerA.wav");
        Mix_PlayChannel(-1, uiAudio[0], 0);
    }
    else if (turret->type == 2 && game->currency >= turret->price) {
        turret->type = 3;
        game->currency -= turret->price;
        turret->price = 50;
        turret->speed = turret->speed + 2;
        turret->range = turret->range + 20;
        turret->damage = turret->damage + 10;
        SDL_DestroyTexture(turret->texture);
        Mix_FreeChunk(turret->turretShootSound);
        turret->texture = loadTexture("assets/sprites/electricTurretT3.png", renderer);
        turret->turretShootSound = Mix_LoadWAV("assets/sfx/zapTowerA.wav");
        Mix_PlayChannel(-1, uiAudio[0], 0);
    }
    //"SNIPER TURRET"
    else if (turret->type == 4 && game->currency >= turret->price) {
        turret->type = 5;
        game->currency -= turret->price;
        turret->price = turret->price*2;
        SDL_DestroyTexture(turret->texture);
        Mix_FreeChunk(turret->turretShootSound);
        turret->texture = loadTexture("assets/sprites/sniperTurretT1.png", renderer);
        turret->turretShootSound = Mix_LoadWAV("assets/sfx/sniperTowerB.wav");
        Mix_PlayChannel(-1, uiAudio[0], 0);
    }
    else if (turret->type == 5 && game->currency >= turret->price) {
        turret->type = 6;
        game->currency -= turret->price;
        turret->price = turret->price*1.5;
        turret->damage = turret->damage*2;
        SDL_DestroyTexture(turret->texture);
        Mix_FreeChunk(turret->turretShootSound);
        turret->texture = loadTexture("assets/sprites/sniperTurretT2.png", renderer);
        turret->turretShootSound = Mix_LoadWAV("assets/sfx/sniperTowerB.wav");
        Mix_PlayChannel(-1, uiAudio[0], 0);
    }
    else if (turret->type == 6 && game->currency >= turret->price) {
        turret->type = 7;
        game->currency -= turret->price;
        turret->price = 100;
        turret->damage = turret->damage*1.5;
        turret->range = turret->range + 100;
        turret->speed = turret->speed - 2;
        SDL_DestroyTexture(turret->texture);
        Mix_FreeChunk(turret->turretShootSound);
        turret->texture = loadTexture("assets/sprites/sniperTurretT3.png", renderer);
        turret->turretShootSound = Mix_LoadWAV("assets/sfx/sniperTowerB.wav");
        Mix_PlayChannel(-1, uiAudio[0], 0);
    }
    //UPGRADING FINAL TIERS
    else if (turret->type == 7 && game->currency >= turret->price)
    {
        game->currency -= turret->price;
        turret->price = turret->price*2;
        turret->damage = turret->damage*1.1;
        Mix_PlayChannel(-1, uiAudio[0], 0);
    }
    else if (turret->type == 3 && game->currency >= turret->price)
    {
        game->currency -= turret->price;
        turret->price = turret->price*2;
        turret->damage = turret->damage*1.2;
        Mix_PlayChannel(-1, uiAudio[0], 0);
    }
    else{
        Mix_PlayChannel(-1, uiAudio[1], 0);
    } 
}
void move(Enemy* enemy, Level* level, GAME_STATE* game,Mix_Chunk* enemySound) {
    if (abs(enemy->position.x - level->nodes[enemy->dest].x) < 20 && abs(enemy->position.y - level->nodes[enemy->dest].y) < 20) {
        if (enemy->dest < level->nodeCount - 1) {
            enemy->dest++;
        } else {
            game->health -= enemy->damage;
            enemy->alive = false;
            Mix_PlayChannel(-1, enemySound, 0);
        }
    }
    if (enemy->dest < level->nodeCount) {
        int distance_x = level->nodes[enemy->dest].x - enemy->position.x;
        int distance_y = level->nodes[enemy->dest].y - enemy->position.y;
        float distance = sqrt(distance_x * distance_x + distance_y * distance_y);
        if (distance != 0) {
            float move_x = (distance_x / distance) * enemy->speed;
            float move_y = (distance_y / distance) * enemy->speed;
            enemy->position.x += move_x;
            enemy->position.y += move_y;
        }
    }
}