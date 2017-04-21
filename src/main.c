#include <SDL2/SDL.h>
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

#define WIDTH 640
#define HEIGHT 480

#define BODY_COUNT 32

typedef struct {
	double x;
	double y;
} Vec2d;

typedef struct {
	Vec2d position;
	Vec2d velocity;
} Body;

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

void Body_ApplyVelocity(Body *body)
{
	body->position.x += body->velocity.x;
	body->position.y += body->velocity.y;
}

void Body_ReflectWithinBounds(Body *body, double xmin, double ymin, double xmax, double ymax)
{
	if (body->position.x < xmin) {
		double new_angle = M_PI - Vec2d_Angle(&body->velocity);
		double length = Vec2d_Length(&body->velocity);

		Vec2d_SetAngleAndLength(&body->velocity, new_angle, length);
		body->position.x = xmin;
	}

	if (body->position.y < ymin) {
		double new_angle = -Vec2d_Angle(&body->velocity);
		double length = Vec2d_Length(&body->velocity);

		Vec2d_SetAngleAndLength(&body->velocity, new_angle, length);
		body->position.y = ymin;
	}

	if (body->position.x > xmax) {
		double new_angle = M_PI - Vec2d_Angle(&body->velocity);
		double length = Vec2d_Length(&body->velocity);

		Vec2d_SetAngleAndLength(&body->velocity, new_angle, length);
		body->position.x = xmax;
	}

	if (body->position.y > ymax) {
		double new_angle = -Vec2d_Angle(&body->velocity);
		double length = Vec2d_Length(&body->velocity);

		Vec2d_SetAngleAndLength(&body->velocity, new_angle, length);
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
	body->position.x = GaussianNoise(WIDTH / 2.0, WIDTH / 12.0);
	body->position.y = GaussianNoise(HEIGHT / 2.0, HEIGHT / 12.0);
	body->velocity.x = GaussianNoise(0.0, 5.0);
	body->velocity.y = GaussianNoise(0.0, 5.0);
}

int MainLoop(SDL_Renderer *renderer)
{
	SDL_Event event;

	memset(&event, 0, sizeof(event));

	bool exiting = false;
	unsigned ticks = 0;

	Body bodies[BODY_COUNT];

	for (int i = 0; i < BODY_COUNT; i++) {
		Body_Randomize(bodies + i);
	}

	while (!exiting) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				exiting = true;
				break;
			}
		}

		for (int i = 0; i < BODY_COUNT; i++) {
			Body_ApplyVelocity(bodies + i);
			Body_ReflectWithinBounds(bodies + i, 0, 0, WIDTH , HEIGHT);
		}

		SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);

		for (int i = 0; i < BODY_COUNT; i++) {
			SDL_Rect rect = { .x = 0, .y = 0, .w = 10, .h = 10 };

			rect.x = bodies[i].position.x - rect.w / 2;
			rect.y = bodies[i].position.y - rect.h / 2;
			SDL_RenderDrawRect(renderer, &rect);
		}

		SDL_RenderPresent(renderer);

		ticks += 1;
	}

	return 0;
}

int main()
{
	SDLRET(SDL_Init(SDL_INIT_VIDEO));

	SDL_Window *window = SDL_CreateWindow("Pong", 100, 100, WIDTH, HEIGHT, WINDOW_FLAGS);
	SDLPTR(window);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);
	SDLPTR(renderer);

	srand(time(NULL));

	MainLoop(renderer);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
}
