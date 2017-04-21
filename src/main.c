#include <SDL2/SDL.h>
#include <stdio.h>

#define _SDLERR() do { fprintf(stderr, "error: %s\n", SDL_GetError()); return 1; }  while (0)
#define SDLRET(ret_code) do { if (ret_code != 0) { _SDLERR(); } } while (0)
#define SDLPTR(ptr) do { if (ptr == NULL) { _SDLERR(); } } while (0)

int main()
{
	SDLRET(SDL_Init(SDL_INIT_VIDEO));

	SDL_Window *window = SDL_CreateWindow("Hello, world!", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
	SDLPTR(window);

	SDL_Quit();
}
