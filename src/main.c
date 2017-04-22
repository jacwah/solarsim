#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>

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
	double x;
	double y;
} Vec2d;

typedef struct {
	Vec2d position;
	Vec2d velocity;
	Vec2d acceleration;
	double mass;
	char *name;
} Body;

typedef struct {
	SDL_Texture *texture;
	int width;
	int height;
} Label;

Body g_solar_system[] = {
	{
		.position = { .x = 4.5356e8, .y = 6.9903e8 },
		.velocity = { .x = -6.5789e0, .y = 1.1177e1 },
		.mass = 1.9885e30,
		.name = "Sol"
	}, {
		.position = { .x = -5.2726e10, .y = -3.7468e10 },
		.velocity = { .x = 1.8415e4, .y = -3.7402e4 },
		.mass = 3.302e23,
		.name = "Mercurius"
	}, {
		.position = { .x = -7.0490e10, .y = -8.1077e10 },
		.velocity = { .x = 2.6200e4, .y = -2.3102e4 },
		.mass = 48.685e23,
		.name = "Venus"
	}, {
		.position = { .x = -1.2732e11, .y = -7.9589e10 },
		.velocity = { .x = 1.5212e4, .y = -2.5423e4 },
		.mass = 5.9722e24,
		.name = "Tellus"
	}, {
		.position = { .x = 4.7562e10, .y = 2.2625e11 },
		.velocity = { .x = -2.2806e4, .y = 7.0263e3 },
		.mass = 6.4185e23,
		.name = "Mars"
	}, {
		.position = { .x = -7.7066e11, .y = -2.6598e11 },
		.velocity = { .x = 4.1098e3, -1.1732e4 },
		.mass = 1898.13e24,
		.name = "Jupiter"
	}, {
		.position = { .x = -1.9262e11, .y = -1.4907e12 },
		.velocity = { .x = 9.0486e3, .y = -1.2687e3 },
		.mass = 5.6832e26,
		.name = "Saturnus"
	}, {
		.position = { .x = 2.7173e12, .y = 1.2281e12 },
		.velocity = { .x = -2.8545e3, .y = 5.8881e3 },
		.mass = 86.8103e24,
		.name = "Uranus"
	}, {
		.position = { .x = 4.2558e12, .y = -1.3998e12 },
		.velocity = { .x = 1.6627e3, .y = 5.1961e3 },
		.mass = 102.41e24,
		.name = "Neptunus"
	}
};

void Body_ApplyVelocity(Body *body, double delta_time)
{
	body->position.x += body->velocity.x * delta_time;
	body->position.y += body->velocity.y * delta_time;
}

void Body_ApplyAcceleration(Body *body, double delta_time)
{
	body->velocity.x += body->acceleration.x * delta_time;
	body->velocity.y += body->acceleration.y * delta_time;
}

void Body_ApplyGravity(Body *a, Body *b)
{
	const double constant = 6.67e-11;
	double xdelta = a->position.x - b->position.x;
	double ydelta = a->position.y - b->position.y;
	double dist_squared = xdelta * xdelta + ydelta * ydelta;
	double force = constant * a->mass * b->mass / dist_squared;
	double angle = atan2(ydelta, xdelta);

	a->acceleration.x += cos(angle + M_PI) * force / a->mass;
	a->acceleration.y += sin(angle + M_PI) * force / a->mass;
	b->acceleration.x += cos(angle) * force / b->mass;
	b->acceleration.y += sin(angle) * force / b->mass;
}

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

	srand(time(NULL));

	MainLoop(renderer);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	TTF_CloseFont(g_font);
	TTF_Quit();
	SDL_Quit();
}
