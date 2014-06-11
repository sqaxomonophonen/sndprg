#pragma once

#include <math.h>

struct F6581Params {
	//static constexpr float MAP_NEG() { return 293760.0f; }
	//static constexpr float MAP_POS() { return 1338240.0f; }
	static constexpr float MAP_NEG() { return 293760.0f; }
	static constexpr float MAP_POS() { return 13382400.0f; }
	static constexpr float SATURATION_THRESHOLD() { return 3.2e6f; }
	static constexpr float SATURATION_SLOPE() { return 0.4f; }
	static constexpr float DISTORTION_RATE() { return 0.5f; }
	static constexpr float DISTORTION_CF_THRESHOLD() { return 1.2e-4f; }
	static constexpr float STEEPNESS() { return 1.0065f; }
	static constexpr float DISTORTION_POINT() { return 2048.0f; }
	static float LOG_STEEPNESS() { return -logf(STEEPNESS()) / 256.0f; }
	static constexpr float MIN_FET_RESISTANCE() { return 1.6e4f; }
	static constexpr float BASE_RESISTANCE() { return 1.37e6f; }
	static constexpr float OUTPUT_DIFFERENCE() { return 1.5f; }
	static constexpr float OFFSET() { return 4.5e8f; }
	static constexpr float SIDCAPS() { return 470e-12f; } // 470pF
};

template <typename PARAMS = F6581Params>
struct F6581 {
	float lowpass_gain;
	float bandpass_gain;
	float highpass_gain;

	float fc, q;

	float vlp, vbp, vhp;

	float distortion_ct;
	float fc_exp;
	float fc_distortion_offset;
	float rq;

	void set_sample_rate(int sample_rate)
	{
		distortion_ct = 1.0f / (PARAMS::SIDCAPS() * (float)sample_rate);
	}

	static float map(float x)
	{
		return PARAMS::MAP_NEG() + (x + 1.0f) * ((PARAMS::MAP_POS() - PARAMS::MAP_NEG()) / 2.0f);
	}

	static float unmap(float x)
	{
		return x * (2.0f / (PARAMS::MAP_POS() - PARAMS::MAP_NEG()));
	}

	float distortion(float input)
	{
		float dist = input - fc_distortion_offset;
		float fet_resistance = fc_exp;
		if(dist > 0.0f) {
			fet_resistance *= expf(dist * PARAMS::LOG_STEEPNESS()); // XXX slow?
		}
		float dynamic_resistance = PARAMS::MIN_FET_RESISTANCE() + fet_resistance;
		float one_div_resistance = (PARAMS::BASE_RESISTANCE() + dynamic_resistance) / (PARAMS::BASE_RESISTANCE() * dynamic_resistance);
		return distortion_ct * one_div_resistance;
	}

	inline float sample(float vf, float vi)
	{
		vi = map(vi);
		vf = map(vf);

		vf += vlp * lowpass_gain;
		vf += vbp * bandpass_gain;
		vf += vhp * highpass_gain;

		if(vf > PARAMS::SATURATION_THRESHOLD()) {
			vf -= (vf - PARAMS::SATURATION_THRESHOLD()) * PARAMS::SATURATION_SLOPE();
		}

		vf -= (vi * PARAMS::DISTORTION_RATE() + vhp + vlp - vbp * rq) * 0.5f * bandpass_gain;
		vbp += (vf - vbp) * PARAMS::DISTORTION_CF_THRESHOLD() * bandpass_gain;
		vlp += (vf - vlp) * PARAMS::DISTORTION_CF_THRESHOLD() * lowpass_gain;
		vhp += (vf - vhp) * PARAMS::DISTORTION_CF_THRESHOLD() * highpass_gain;
		vlp -= vbp * distortion(vbp) * PARAMS::OUTPUT_DIFFERENCE();
		vbp -= vhp * distortion(vhp);
		vhp = vbp * rq - (vlp * (1.0f / PARAMS::OUTPUT_DIFFERENCE())) - vi * PARAMS::DISTORTION_RATE();

		return unmap(vf);
	}

	void fc_update()
	{
		fc_exp = PARAMS::OFFSET() * expf(fc * PARAMS::LOG_STEEPNESS() * 256.0f);
		fc_distortion_offset = (PARAMS::DISTORTION_POINT() - fc) * 256.0f * PARAMS::DISTORTION_RATE();
	}

	void q_update()
	{
		rq = 1.0f / (0.707f + q);
	}

	void set_fc(float value)
	{
		fc = value;
		fc_update();
	}

	void set_q(float value)
	{
		q = value;
		q_update();
	}
};

