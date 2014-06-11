#pragma once

#include <stdlib.h>
#include <math.h>

static inline double bessel_I0(double x)
{
	double d = 0.0;
	double ds = 1.0;
	double s = 1.0;
	do {
		d += 2.0;
		ds *= (x*x) / (d*d);
		s += ds;
	} while (ds > s*1e-7);
	return s;
}

static inline double kaiser_bessel(double x)
{
	double alpha = 0.0;
	double att = 40.;
	if (att > 50.0f) {
		alpha = 0.1102f * (att - 8.7f);
	} else if (att > 20.0f) {
		alpha = 0.5842f * pow(att - 21.0f, 0.4f) + 0.07886f * (att - 21.0f);
	}
	return bessel_I0(alpha * sqrt(1.0f - x*x)) / bessel_I0(alpha);
}


static inline float slope_coef(float rate, float slope)
{
	return expf(-logf((1.0f + slope) / slope) / rate);
}

static inline float note_to_hz(float base_hz, float note)
{
	return base_hz * powf(2.0f, note / 12.0f);
}

static inline float clampf(float x, float min, float max)
{
	if(x < min) return min;
	if(x > max) return max;
	return x;
}

static inline float randf(float min, float max)
{
	float r = (float)rand() / (float)RAND_MAX;
	return min + r * (max - min);
}

