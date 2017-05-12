#include <math.h>
#include "body.h"

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

void Body_ApplyOrbitalGravity(Body *a, Body *sun)
{
	double xdelta = a->position.x - sun->position.x;
	double ydelta = a->position.y - sun->position.y;
	double dist = hypot(xdelta, ydelta);
	double angular_velocity = 2 * M_PI / a->orbital_period;

	a->acceleration.x += angular_velocity * angular_velocity * xdelta;
	a->acceleration.y += angular_velocity * angular_velocity * ydelta;
	/*double acceleration = angular_velocity * angular_velocity * dist;*/
	/*double angle = atan2(ydelta, xdelta);*/

	/*a->acceleration.x += cos(angle) * acceleration;*/
	/*a->acceleration.y += sin(angle) * acceleration;*/
}
