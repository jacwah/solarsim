#ifndef TRACERS_H
#define TRACERS_H

#include "body.h"
#define TRACERS_MIN_DIST 1.0e3

typedef struct {
	Vec2d *points;
	Vec2d *head;
	size_t len;
	size_t size;
} Tracers;

void Tracers_Init(Tracers *tracers, size_t size);
bool Tracers_AddPoint(Tracers *tracers, Vec2d point);
#endif
