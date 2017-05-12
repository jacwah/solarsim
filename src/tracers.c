#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "tracers.h"

void Tracers_Init(Tracers *tracers, size_t size)
{
	tracers->points = malloc(sizeof(Vec2d) * size);
	tracers->head = tracers->points;
	tracers->len = 0;
	tracers->size = size;
}

bool Tracers_AddPoint(Tracers *tracers, Vec2d point)
{
	bool should_add = true;
	size_t head_index = tracers->head - tracers->points;

	for (int i = 0; i < tracers->len; i++) {
		Vec2d *other = tracers->points + ((head_index - i) % tracers->size);
		double xdelta = point.x - other->x;
		double ydelta = point.y - other->y;

		if (hypot(xdelta, ydelta) >= TRACERS_MIN_DIST) {
			should_add = false;
			break;
		}
	}

	if (should_add) {
		*tracers->head = point;
		Vec2d *next = tracers->points + ((head_index + 1) % tracers->size);
		tracers->head = next;

		if (tracers->len < tracers->size) {
			tracers->len += 1;
		}

		return true;
	} else {
		return false;
	}
}
