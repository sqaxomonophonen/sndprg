#pragma once

#include <stdint.h>

#include "Math.h"

template <typename ENV, int WAVETABLE_SIZE_EXP = 8>
struct SkaarOsc
{
	static constexpr int PERIOD_EXP = 28;
	static constexpr int PERIOD = 1 << PERIOD_EXP;
	static constexpr int MSB = PERIOD >> 1;
	static constexpr int PERIOD_MASK = PERIOD - 1;
	static constexpr int WAVETABLE_SIZE = 1 << WAVETABLE_SIZE_EXP;

	static constexpr int SAW = 1<<0;
	static constexpr int SQR = 1<<1;
	static constexpr int TRI = 1<<2;
	static constexpr int NOI = 1<<3;
	static constexpr int WAV = 1<<4;
	static constexpr int SYN = 1<<5;
	static constexpr int OFF = 1<<6;
	static constexpr int SYN_HI = 1<<16;
	static constexpr int LFSR_SYNC = 1<<17;

	float sample_rate = 0.0f;
	int flags = 0;
	uint32_t phase = 0;
	uint32_t inc = 0;
	int32_t shift = 0;
	uint32_t lfsr = 1;
	int16_t wavetable[WAVETABLE_SIZE];
	float gain = 1.0f;
	float gain_filter = 0.0f;
	ENV env;

	void set_sample_rate(float value)
	{
		sample_rate = value;
		env.set_sample_rate(value);
	}

	void set_shift(float value)
	{
		shift = clampf(value, -1.0f, 1.0f) * PERIOD;
	}

	inline void apply_hard_sync(SkaarOsc<ENV, WAVETABLE_SIZE_EXP>* master)
	{
		if (flags & SYN) {
			if ((master->flags & SYN_HI) == 0 && master->phase & MSB) {
				master->flags |= SYN_HI;
				phase = 0;
			} else if ((master->phase & MSB) == 0) {
				master->flags &= ~SYN_HI;
			}
		}
	}

	inline uint32_t sample_osc()
	{
		phase = (phase + inc) & PERIOD_MASK;

		if (flags == 0) {
			return PERIOD >> 1;
		}

		// shift phase into x
		int32_t x = (int32_t)phase + shift;
		if (x < 0) x = 0;
		if (x >= PERIOD) x = PERIOD - 1;

		// square
		if((flags & SQR) && (x & MSB)) {
			return 0;
		}

		uint32_t out = PERIOD_MASK;

		// saw
		if (flags & SAW) {
			out &= x;
		}

		// triangle
		if (flags & TRI) {
			out &= (x & MSB) ? ((x<<1) ^ PERIOD_MASK) : (x<<1);
		}

		// noise (linear feedback shift register)
		if (flags & NOI) {
			int b19 = (x>>(PERIOD_EXP - 5)) & 1;
			int sync = (flags & LFSR_SYNC) ? 1 : 0;
			if(b19 ^ sync) {
				int lsb = 1 & ((lfsr>>17) ^ (lfsr>>22));
				lfsr <<= 1;
				lfsr |= lsb;
				lfsr &= ((1<<23)-1);
				flags ^= LFSR_SYNC;
			}
			out &= (lfsr<<1);
		}

		// wavetable
		if (flags & WAV) {
			int i = x >> (PERIOD_EXP - WAVETABLE_SIZE_EXP);
			out &= (wavetable[i] + (1 << ((sizeof(*wavetable)*8)-1))) << (PERIOD_EXP - sizeof(*wavetable)*8);
		}

		return out;
	}

	inline float sample()
	{
		if (flags & OFF) {
			return 0.0f;
		}
		return ((float)sample_osc() / (float)(PERIOD >> 1) - 0.5f) * env.sample();
	}

	void set_hz(float hz)
	{
		inc = (float)PERIOD * hz / sample_rate;
	}


	void wavetable_fn(float (*fn)(float))
	{
		for (int i = 0; i < WAVETABLE_SIZE; i++) {
			float x = (float)i * (1.0f / (float)WAVETABLE_SIZE) * M_PI * 2.0f;
			float v = clampf(fn(x), -1.0f, 1.0f);
			wavetable[i] = (int16_t)(v * 32767.0f);
		}
	}
};

template <int OSC_COUNT, typename FILTER, typename OSC>
struct Skaar
{
	typedef float T;
	typedef float Q;

	OSC osc[OSC_COUNT];
	FILTER filter;
	float gain = 1.0f;

	void set_sample_rate(float sample_rate)
	{
		for (auto& o : osc) {
			o.set_sample_rate(sample_rate);
		}
		filter.set_sample_rate(sample_rate);
	}

	inline float sample()
	{
		auto* oprev = &osc[OSC_COUNT - 1];
		for (auto& o : osc) {
			o.apply_hard_sync(oprev);
			oprev = &o;
		}

		float vf_out = 0.0f;
		float vf_filter = 0.0f;
		for (auto& o : osc) {
			float vf = o.sample();
			vf_out += vf * o.gain;
			vf_filter += vf * o.gain_filter;
		}

		float vf = filter.sample(vf_out, vf_filter);

		return vf * gain;
	}

};


