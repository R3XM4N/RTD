#ifndef SDL_H
#define SDL_H

SDL_Texture* loadTexture(const char* path, SDL_Renderer* renderer);
SDL_Texture* renderText(const char* message, const char* fontFile, SDL_Color color, int fontSize, SDL_Renderer* renderer);

#endif