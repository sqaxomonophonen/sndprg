#pragma once

#include "Math.h"

struct ADSR {
	float sample_rate;
	float value;
	enum {
		IDLE = 0,
		ATTACK,
		DECAY,
		SUSTAIN,
		RELEASE
	} state = IDLE;
	float attack_coef, attack_base;
	float decay_coef, decay_base;
	float release_coef, release_base;
	float sustain_level;

	void set_sample_rate(float value)
	{
		sample_rate = value;
	}

	void on()
	{
		state = ATTACK;
	}

	void off()
	{
		if (state != IDLE) {
			state = RELEASE;
		}
	}

	void set_attack(float value, float slope)
	{
		attack_coef = slope_coef(value * sample_rate, slope);
		attack_base = (1.0f + slope) * (1.0f - attack_coef);
	}

	void set_decay(float value, float slope)
	{
		decay_coef = slope_coef(value * sample_rate, slope);
		decay_base = (sustain_level - slope) * (1.0f - decay_coef);
	}

	void set_sustain(float value)
	{
		sustain_level = value;
	}

	void set_release(float value, float slope)
	{
		release_coef = slope_coef(value * sample_rate, slope);
		release_base = -slope * (1.0f - release_coef);
	}

	float sample()
	{
		switch (state) {
			case IDLE:
				break;
			case ATTACK:
				value = attack_base + value * attack_coef;
				if (value >= 1.0f) {
					value = 1.0f;
					state = DECAY;
				}
				break;
			case DECAY:
				value = decay_base + value * decay_coef;
				if (value < sustain_level) {
					value = sustain_level;
					state = SUSTAIN;
				}
				break;
			case SUSTAIN:
				break;
			case RELEASE:
				value = release_base + value * release_coef;
				if (value <= 0.0f) {
					value = 0.0f;
					state = IDLE;
				}
				break;
		}
		return value;
	}
};


