#include "body.h"

#define Y2S(yr) (yr * 60.0 * 60.0 * 24.0 * 365.25)

Body g_solar_system[] = {
	{
		.position = { .x = 4.5356e8, .y = 6.9903e8 },
		.velocity = { .x = -6.5789e0, .y = 1.1177e1 },
		.mass = 1.9885e30,
		.orbital_period = 0,
		.name = "Sol"
	}, {
		.position = { .x = -5.2726e10, .y = -3.7468e10 },
		.velocity = { .x = 1.8415e4, .y = -3.7402e4 },
		.mass = 3.302e23,
		.orbital_period = Y2S(0.241),
		.name = "Mercurius"
	}, {
		.position = { .x = -7.0490e10, .y = -8.1077e10 },
		.velocity = { .x = 2.6200e4, .y = -2.3102e4 },
		.mass = 48.685e23,
		.orbital_period = Y2S(0.615),
		.name = "Venus"
	}, {
		.position = { .x = -1.2732e11, .y = -7.9589e10 },
		.velocity = { .x = 1.5212e4, .y = -2.5423e4 },
		.mass = 5.9722e24,
		.orbital_period = Y2S(1),
		.name = "Tellus"
	}, {
		.position = { .x = 4.7562e10, .y = 2.2625e11 },
		.velocity = { .x = -2.2806e4, .y = 7.0263e3 },
		.mass = 6.4185e23,
		.orbital_period = Y2S(1.881),
		.name = "Mars"
	}, {
		.position = { .x = -7.7066e11, .y = -2.6598e11 },
		.velocity = { .x = 4.1098e3, -1.1732e4 },
		.mass = 1898.13e24,
		.orbital_period = Y2S(11.86),
		.name = "Jupiter"
	}, {
		.position = { .x = -1.9262e11, .y = -1.4907e12 },
		.velocity = { .x = 9.0486e3, .y = -1.2687e3 },
		.mass = 5.6832e26,
		.orbital_period = Y2S(29.42),
		.name = "Saturnus"
	}, {
		.position = { .x = 2.7173e12, .y = 1.2281e12 },
		.velocity = { .x = -2.8545e3, .y = 5.8881e3 },
		.mass = 86.8103e24,
		.orbital_period = Y2S(83.75),
		.name = "Uranus"
	}, {
		.position = { .x = 4.2558e12, .y = -1.3998e12 },
		.velocity = { .x = 1.6627e3, .y = 5.1961e3 },
		.mass = 102.41e24,
		.orbital_period = Y2S(163.7),
		.name = "Neptunus"
	}
};
