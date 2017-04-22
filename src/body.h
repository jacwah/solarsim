#ifndef BODY_H
#define BODY_H

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

void Body_ApplyVelocity(Body *body, double delta_time);
void Body_ApplyAcceleration(Body *body, double delta_time);
void Body_ApplyGravity(Body *a, Body *b);

#endif
