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

#define WINDOW_TITLE "Solar system simulation | Jacob Wahlgren 2017"
#define HELP_TEXT "+/- zoom, UP/DOWN control time, D show/hide vectors, 0-9 center body, H show/hide"

//					R     G     B     A
#define COLOR_BACKGROUND		0xff, 0xff, 0xff, 0xff
#define COLOR_DEBUG_VELOCITY		0x00, 0xff, 0x00, 0xff
#define COLOR_DEBUG_ACCELERATION	0xff, 0x00, 0x00, 0xff
#define COLOR_BODY			0x00, 0x00, 0xff, 0xff

#define PX(c) (g_render_scale * ((c) - g_center_point->x) + SCREEN_SIZE / 2)
#define PY(c) (SCREEN_SIZE - (g_render_scale * ((c) - g_center_point->y) + SCREEN_SIZE / 2))

double g_render_scale = SCREEN_SIZE / 1.0e13;
Vec2d *g_center_point = &g_solar_system[0].position;

TTF_Font *g_font = NULL;

typedef struct {
	SDL_Texture *texture;
	int width;
	int height;
} Label;

static void RenderBodyLabels(SDL_Renderer *renderer, Body bodies[], Label labels[], size_t count)
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
				PX(bodies[i].position.x + 1.0e8 * bodies[i].velocity.x),
				PY(bodies[i].position.y + 1.0e8 * bodies[i].velocity.y));
	}

	SDL_SetRenderDrawColor(renderer, COLOR_DEBUG_ACCELERATION);
	for (int i = 0; i < count; i++) {
		SDL_RenderDrawLine(renderer,
				PX(bodies[i].position.x),
				PY(bodies[i].position.y),
				PX(bodies[i].position.x + 1.0e16 * bodies[i].acceleration.x),
				PY(bodies[i].position.y + 1.0e16 * bodies[i].acceleration.y));
	}
}

static void UpdateBodies(Body bodies[], size_t body_count, double delta_time, bool orbital_gravity)
{
	for(int i = 0; i < body_count; i++) {
		bodies[i].acceleration.x = 0.0;
		bodies[i].acceleration.y = 0.0;
	}

	if (orbital_gravity) {
		for (int i = 1; i < body_count; i++) {
			Body_ApplyOrbitalGravity(bodies + 0, bodies + i);
			Body_ApplyAcceleration(bodies + i, delta_time);
			Body_ApplyVelocity(bodies + i, delta_time);
		}
	} else {
		for (int i = 0; i < body_count; i++) {
			for (int j = i + 1; j < body_count; j++) {
				Body_ApplyGravity(bodies + i, bodies + j);
			}
			Body_ApplyAcceleration(bodies + i, delta_time);
			Body_ApplyVelocity(bodies + i, delta_time);
		}
	}
}

static void PrerenderLabel(SDL_Renderer *renderer, Label *label, const char *text)
{
	SDL_Color text_color = {0, 0, 0};
	SDL_Surface *surf = TTF_RenderUTF8_Blended(g_font, text, text_color);
	SDLPTR(surf);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surf);
	SDLPTR(texture);

	label->texture = texture;
	label->width = surf->w;
	label->height = surf->h;

	SDL_FreeSurface(surf);
}

static void PrerenderBodyLabels(SDL_Renderer *renderer, Body bodies[], Label labels[], size_t body_count)
{
	for (int i = 0; i < body_count; i++) {
		PrerenderLabel(renderer, labels + i, bodies[i].name);
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

void SetCenterPoint(Body bodies[], size_t body_count, int body_index)
{
	if (0 <= body_index && body_index < body_count) {
		g_center_point = &bodies[body_index].position;
		printf("Center point: %s\n", bodies[body_index].name);
	}
}

/* Result can only be used until next call! */
static char *SecondsAsText(double time)
{
	static char buf[256];
	double weeks = time / (60 * 60 * 24 * 7);

	sprintf(buf, "%.1lf weeks per second", weeks);

	return buf;
}

static int MainLoop(SDL_Renderer *renderer)
{
	bool exiting = false;
	bool debug = false;
	bool show_info = true;
	bool orbital_gravity = false;

	// Assumes 60 Hz monitor
	const double frame_length = 1.0 / 60.0;
	// 8 weeks per second
	double delta_time = frame_length * 60.0 * 60.0 * 24.0 * 7.0 * 8.0;
	const double time_factor = 2.0;
	const double scale_factor = 2.0;

	Body *bodies = g_solar_system;
	size_t body_count = sizeof(g_solar_system) / sizeof(g_solar_system[0]);
	Label body_labels[body_count];
	Label help_label, time_label;

	PrerenderBodyLabels(renderer, bodies, body_labels, body_count);
	PrerenderLabel(renderer, &help_label, HELP_TEXT);
	PrerenderLabel(renderer, &time_label, SecondsAsText(delta_time / frame_length));

	SDL_Rect help_label_rect = {
		.x = SCREEN_SIZE - help_label.width - 10,
		.y = SCREEN_SIZE - help_label.height - 10,
		.w = help_label.width,
		.h = help_label.height
	};

	SDL_Rect time_label_rect = {
		.x = SCREEN_SIZE - time_label.width - 10,
		.y = SCREEN_SIZE - time_label.height - 25,
		.w = time_label.width,
		.h = time_label.height
	};

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
				case SDLK_h:
					show_info = !show_info;
					break;
				case SDLK_o:
					orbital_gravity = !orbital_gravity;
					break;
				case SDLK_UP:
					delta_time *= time_factor;
					PrerenderLabel(renderer, &time_label, SecondsAsText(delta_time / frame_length));
					break;
				case SDLK_DOWN:
					delta_time /= time_factor;
					PrerenderLabel(renderer, &time_label, SecondsAsText(delta_time / frame_length));
					break;
				case SDLK_PLUS:
					g_render_scale *= scale_factor;
					break;
				case SDLK_MINUS:
					g_render_scale /= scale_factor;
					break;
				default:
					{
						int sym = event.key.keysym.sym;
						if (SDLK_0 <= sym && SDLK_9 >= sym) {
							SetCenterPoint(bodies, body_count, sym - SDLK_0);
						}
						break;
					}
				}
				break;
			}
		}

		UpdateBodies(bodies, body_count, delta_time, orbital_gravity);

		SDL_SetRenderDrawColor(renderer, COLOR_BACKGROUND);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, COLOR_BODY);
		RenderBodies(renderer, bodies, body_count);

		if (debug) {
			RenderDebug(renderer, body_count, bodies);
		}

		if (show_info) {
			SDL_RenderCopy(renderer, help_label.texture, NULL, &help_label_rect);
			SDL_RenderCopy(renderer, time_label.texture, NULL, &time_label_rect);
		}

		RenderBodyLabels(renderer, bodies, body_labels, body_count);

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

	SDL_Window *window = SDL_CreateWindow(WINDOW_TITLE, 100, 100, SCREEN_SIZE, SCREEN_SIZE, WINDOW_FLAGS);
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
