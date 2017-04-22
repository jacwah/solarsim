#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>

#define _SDLERR() do { fprintf(stderr, "error: %s\n", SDL_GetError()); return 1; }  while (0)
#define SDLRET(ret_code) do { if (ret_code != 0) { _SDLERR(); } } while (0)
#define SDLPTR(ptr) do { if (ptr == NULL) { _SDLERR(); } } while (0)

#define WINDOW_FLAGS SDL_WINDOW_SHOWN
#define RENDERER_FLAGS SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC

#define SCREEN_SIZE 800

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

double Vec2d_Angle(Vec2d *vec)
{
	return atan2(vec->y, vec->x);
}

double Vec2d_Length(Vec2d *vec)
{
	return hypot(vec->x, vec->y);
}

void Vec2d_SetAngleAndLength(Vec2d *vec, double angle, double length)
{
	vec->x = length * cos(angle);
	vec->y = length * sin(angle);
}

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

void Body_ReflectWithinBounds(Body *body, double xmin, double ymin, double xmax, double ymax)
{
	if (body->position.x < xmin) {
		body->velocity.x *= -1;
		body->position.x = xmin;
	}

	if (body->position.y < ymin) {
		body->velocity.y *= -1;
		body->position.y = ymin;
	}

	if (body->position.x > xmax) {
		body->velocity.x *= -1;
		body->position.x = xmax;
	}

	if (body->position.y > ymax) {
		body->velocity.y *= -1;
		body->position.y = ymax;
	}
}

double GaussianNoise(double mean, double stddev)
{
	static double z0, z1;
	static bool should_generate = false;

	should_generate = !should_generate;

	if (!should_generate) {
		return z1 * stddev + mean;
	}

	double u1, u2;

	// u1 has to be distinguishable from 0 for log(u1)
	do {
		u1 = rand() * (1.0 / RAND_MAX);
		u2 = rand() * (1.0 / RAND_MAX);
	} while (u1 <= DBL_EPSILON);

	z0 = sqrt(-2.0 * log(u1)) * cos(2 * M_PI * u2);
	z1 = sqrt(-2.0 * log(u1)) * sin(2 * M_PI * u2);

	return z0 * stddev + mean;
}

void Body_Randomize(Body *body)
{
	body->position.x = GaussianNoise(SCREEN_SIZE / 2.0, SCREEN_SIZE / 6.0);
	body->position.y = GaussianNoise(SCREEN_SIZE / 2.0, SCREEN_SIZE / 6.0);
	body->velocity.x = GaussianNoise(0.0, 0.0);
	body->velocity.y = GaussianNoise(0.0, 0.0);
	body->acceleration.x = GaussianNoise(0.0, 0.0);
	body->acceleration.y = GaussianNoise(0.0, 0.0);
	body->mass = GaussianNoise(1.0e10, 1.0);
}

void RenderDebug(SDL_Renderer *renderer, size_t count, Body bodies[], Label labels[])
{
	SDL_SetRenderDrawColor(renderer, 0, 0xff, 0, 0xff);
	for (int i = 0; i < count; i++) {
		SDL_RenderDrawLine(renderer,
				PX(bodies[i].position.x),
				PY(bodies[i].position.y),
				PX(bodies[i].position.x + 1.0e-2 * bodies[i].velocity.x),
				PY(bodies[i].position.y + 1.0e-2 * bodies[i].velocity.y));
	}

	SDL_SetRenderDrawColor(renderer, 0, 0, 0xff, 0xff);
	for (int i = 0; i < count; i++) {
		SDL_RenderDrawLine(renderer,
				PX(bodies[i].position.x),
				PY(bodies[i].position.y),
				PX(bodies[i].position.x + 1.0e4 * bodies[i].acceleration.x),
				PY(bodies[i].position.y + 1.0e4 * bodies[i].acceleration.y));
	}

	for (int i = 0; i < count; i++) {
		SDL_Rect rect = { .x = PX(bodies[i].position.x) + 10,
				  .y = PY(bodies[i].position.y) - 10,
				  .w = labels[i].width,
				  .h = labels[i].height };
		SDL_RenderCopy(renderer, labels[i].texture, NULL, &rect);
	}
}

int MainLoop(SDL_Renderer *renderer)
{
	SDL_Event event;

	bool exiting = false;
	bool debug = true;

	Body *bodies = g_solar_system;
	size_t body_count = sizeof(g_solar_system) / sizeof(g_solar_system[0]);
	Label labels[body_count];

	SDL_Color text_color = {0, 0, 0};

	for (int i = 0; i < body_count; i++) {
		char buf[32] = "";

		sprintf(buf, "%s (%d)", bodies[i].name, i);
		SDL_Surface *surf = TTF_RenderUTF8_Blended(g_font, buf, text_color);
		SDLPTR(surf);
		SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surf);
		SDLPTR(texture);

		labels[i].texture = texture;
		labels[i].width = surf->w;
		labels[i].height = surf->h;

		SDL_FreeSurface(surf);
	}

	// 10 weeks per second
	double delta_time = 1.0 / 60.0 * 60.0 * 60.0 * 24.0 * 7.0 * 10.0;
	const double time_factor = 2.0;
	const double scale_factor = 2.0;

	while (!exiting) {
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

		SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);

		for (int i = 0; i < body_count; i++) {
			SDL_Rect rect = { .x = 0, .y = 0, .w = 10, .h = 10 };
			rect.x = PX(bodies[i].position.x) - rect.w / 2;
			rect.y = PY(bodies[i].position.y) - rect.h / 2;
			SDL_RenderDrawRect(renderer, &rect);
		}

		if (debug) {
			RenderDebug(renderer, body_count, bodies, labels);
		}

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
