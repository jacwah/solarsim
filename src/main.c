#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "body.h"
#include "solar_system_data.h"

#define _SDLERR() do { fprintf(stderr, "error: %s\n", SDL_GetError()); exit(1); }  while (0)
#define SDLRET(ret_code) do { if (ret_code != 0) { _SDLERR(); } } while (0)
#define SDLPTR(ptr) do { if (ptr == NULL) { _SDLERR(); } } while (0)

#define WINDOW_FLAGS SDL_WINDOW_SHOWN
#define RENDERER_FLAGS SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC

#define SCREEN_SIZE 800
#define BODY_SIZE 4

//					R     G     B     A
#define COLOR_BACKGROUND		0xff, 0xff, 0xff, 0xff
#define COLOR_DEBUG_VELOCITY		0x00, 0xff, 0x00, 0xff
#define COLOR_DEBUG_ACCELERATION	0xff, 0x00, 0x00, 0xff
#define COLOR_BODY			0x00, 0x00, 0xff, 0xff

#define PX(x) (g_render_scale * x + SCREEN_SIZE / 2)
#define PY(y) (SCREEN_SIZE - (g_render_scale * y + SCREEN_SIZE / 2))

double g_render_scale = SCREEN_SIZE / 1.0e13;

TTF_Font *g_font = NULL;

typedef struct {
	SDL_Texture *texture;
	int width;
	int height;
} Label;

static void RenderLabels(SDL_Renderer *renderer, Body bodies[], Label labels[], size_t count)
{
	for (int i = 0; i < count; i++) {
		SDL_Rect rect = { .x = PX(bodies[i].position.x) + 10,
				  .y = PY(bodies[i].position.y) - 10,
				  .w = labels[i].width,
				  .h = labels[i].height };
		SDL_RenderCopy(renderer, labels[i].texture, NULL, &rect);
	}
}

static void RenderDebug(SDL_Renderer *renderer, size_t count, Body bodies[])
{
	SDL_SetRenderDrawColor(renderer, COLOR_DEBUG_VELOCITY);
	for (int i = 0; i < count; i++) {
		SDL_RenderDrawLine(renderer,
				PX(bodies[i].position.x),
				PY(bodies[i].position.y),
				PX(bodies[i].position.x + 1.0e-2 * bodies[i].velocity.x),
				PY(bodies[i].position.y + 1.0e-2 * bodies[i].velocity.y));
	}

	SDL_SetRenderDrawColor(renderer, COLOR_DEBUG_ACCELERATION);
	for (int i = 0; i < count; i++) {
		SDL_RenderDrawLine(renderer,
				PX(bodies[i].position.x),
				PY(bodies[i].position.y),
				PX(bodies[i].position.x + 1.0e4 * bodies[i].acceleration.x),
				PY(bodies[i].position.y + 1.0e4 * bodies[i].acceleration.y));
	}
}

static void UpdateBodies(Body bodies[], size_t body_count, double delta_time)
{
	for(int i = 0; i < body_count; i++) {
		bodies[i].acceleration.x = 0.0;
		bodies[i].acceleration.y = 0.0;
	}

	for (int i = 0; i < body_count; i++) {
		for (int j = i + 1; j < body_count; j++) {
			Body_ApplyGravity(bodies + i, bodies + j);
		}
		Body_ApplyAcceleration(bodies + i, delta_time);
		Body_ApplyVelocity(bodies + i, delta_time);
	}
}

static void PrerenderLabels(SDL_Renderer *renderer, Body bodies[], Label labels[], size_t body_count)
{
	SDL_Color text_color = {0, 0, 0};

	for (int i = 0; i < body_count; i++) {
		char buf[32] = "";

		sprintf(buf, "%s", bodies[i].name);
		SDL_Surface *surf = TTF_RenderUTF8_Blended(g_font, buf, text_color);
		SDLPTR(surf);
		SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surf);
		SDLPTR(texture);

		labels[i].texture = texture;
		labels[i].width = surf->w;
		labels[i].height = surf->h;

		SDL_FreeSurface(surf);
	}
}

static void RenderBodies(SDL_Renderer *renderer, Body bodies[], size_t body_count)
{
	for (int i = 0; i < body_count; i++) {
		SDL_Rect rect = {
			.x = PX(bodies[i].position.x) - BODY_SIZE / 2,
			.y = PY(bodies[i].position.y) - BODY_SIZE / 2,
			.w = BODY_SIZE,
			.h = BODY_SIZE
		};

		SDL_RenderFillRect(renderer, &rect);
	}
}

static int MainLoop(SDL_Renderer *renderer)
{
	bool exiting = false;
	bool debug = false;

	Body *bodies = g_solar_system;
	size_t body_count = sizeof(g_solar_system) / sizeof(g_solar_system[0]);
	Label labels[body_count];

	PrerenderLabels(renderer, bodies, labels, body_count);

	// Assumes 60 Hz monitor
	const double frame_length = 1.0 / 60.0;
	// 10 weeks per second
	double delta_time = frame_length * 60.0 * 60.0 * 24.0 * 7.0 * 10.0;
	const double time_factor = 2.0;
	const double scale_factor = 2.0;

	while (!exiting) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				exiting = true;
				break;
			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
				case SDLK_d:
					debug = !debug;
					break;
				case SDLK_UP:
					delta_time *= time_factor;
					break;
				case SDLK_DOWN:
					delta_time /= time_factor;
					break;
				case SDLK_PLUS:
					g_render_scale *= scale_factor;
					break;
				case SDLK_MINUS:
					g_render_scale /= scale_factor;
					break;
				}
				break;
			}
		}

		UpdateBodies(bodies, body_count, delta_time);

		SDL_SetRenderDrawColor(renderer, COLOR_BACKGROUND);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, COLOR_BODY);
		RenderBodies(renderer, bodies, body_count);

		if (debug) {
			RenderDebug(renderer, body_count, bodies);
		}

		RenderLabels(renderer, bodies, labels, body_count);

		SDL_RenderPresent(renderer);
	}

	return 0;
}

int main()
{
	SDLRET(SDL_Init(SDL_INIT_VIDEO));

	if (0 != TTF_Init()) {
		fprintf(stderr, "ttf error: %s\n", TTF_GetError());
		return 1;
	}

	g_font = TTF_OpenFont("/Library/Fonts/Arial.ttf", 12);

	if (!g_font) {
		fprintf(stderr, "ttf error: %s\n", TTF_GetError());
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow("Pong", 100, 100, SCREEN_SIZE, SCREEN_SIZE, WINDOW_FLAGS);
	SDLPTR(window);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);
	SDLPTR(renderer);

	MainLoop(renderer);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	TTF_CloseFont(g_font);
	TTF_Quit();
	SDL_Quit();
}
