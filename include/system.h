#ifndef DEF_H
#define DEF_H

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position position;
    int dest;
    int speed;
    int health;
    int damage;
    int reward;
    bool alive;
    SDL_Texture* texture;
} Enemy;
//position, destination, speed, health, damage, reward, alive, texture
typedef struct {
    Position position;
    int cooldown;
    int speed;
    int type;
    int damage;
    int range;
    int price;
    SDL_Texture* texture;
    Mix_Chunk* turretShootSound;
} Turret;
//position, cooldown, speed, type, damage, range, price, texture
typedef struct {
    int startCurrency;
    int nodeCount;
    int maxTurrets;
    Position* nodes;
    SDL_Texture* texture;
} Level;
typedef struct {
    int wave;
    int health;
    int currency;
    Enemy* enemies;
    Turret* turrets;
    Level* level;
} GAME_STATE;

void move(Enemy* enemy, Level* level, GAME_STATE* game,Mix_Chunk* enemySound);
void upgradeTurret(Turret* turret,GAME_STATE* game, SDL_Renderer* renderer, Mix_Chunk* uiAudio[4]);
bool positionOnTurret(int mouseX, int mouseY, Turret* turret);
Enemy* createEnemies(int wave, SDL_Texture* enemyTexture);
void turretShoot(Turret* turret, Enemy* enemies, int enemyCount, GAME_STATE* game);
int calculateEnemiesToSpawn(int wave);
GAME_STATE* initGame();
Level* initLevel(int startCurrency, int nodeCount, int maxTurrets, Position* nodes, SDL_Texture* texture);

#endif