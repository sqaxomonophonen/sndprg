#pragma once

#include <math.h>

#include "Math.h"
#include "Tables.h"

template <typename SAMPLER, int RATIO, int ZERO_CROSSINGS, double (*WINDOW_FN)(double)>
struct FirOversampler : public SAMPLER {

	static constexpr int _FO_BUFFER_SIZE = RATIO * ZERO_CROSSINGS * 2 + 1;
	static constexpr int _FO_BUFFER_MID = RATIO * ZERO_CROSSINGS;
	static constexpr int _FO_REAL_FIR_SIZE = RATIO * ZERO_CROSSINGS;
	static constexpr int _FO_FIR_SIZE = _FO_REAL_FIR_SIZE - ZERO_CROSSINGS;

	typedef typename SAMPLER::T T;
	typedef typename SAMPLER::Q Q;

	T _fo_buffer[_FO_BUFFER_SIZE];
	int _fo_buffer_index = 0;
	Q* _fo_fir;

	void set_sample_rate(float sample_rate)
	{
		SAMPLER::set_sample_rate(sample_rate * RATIO);
	}

	FirOversampler()
	{
		_fo_fir = Tables<Q>::get_instance().get_compact_downsampler_fir(RATIO, ZERO_CROSSINGS, WINDOW_FN);
	}

	inline T sample()
	{
		for (int s = 0; s < RATIO; s++) {
			_fo_push(SAMPLER::sample());
		}
		return _fo_yield();
	}

	inline void _fo_push(T value)
	{
		_fo_buffer[_fo_buffer_index++] = value;
		if(_fo_buffer_index >= _FO_BUFFER_SIZE) _fo_buffer_index = 0;
	}

	inline void _fo_step(int i, int& idx, int& n, T& signal)
	{
		if (idx >= _FO_BUFFER_SIZE) {
			idx -= _FO_BUFFER_SIZE;
		}
		signal += _fo_buffer[idx] * _fo_fir[i];
		idx++;
		n--;
		if (n == 0) {
			// skip zero crossing
			idx++;
			n = RATIO - 1;
		}
	}

	inline T _fo_yield()
	{
		int idx = _fo_buffer_index + _FO_BUFFER_MID;
		if (idx >= _FO_BUFFER_SIZE) {
			idx -= _FO_BUFFER_SIZE;
		}
		T signal = _fo_buffer[idx];
		idx = _fo_buffer_index + 1;
		int n = RATIO - 1;
		for (int i = (_FO_FIR_SIZE - 1); i >= 0; i--) {
			_fo_step(i, idx, n, signal);
		}
		for (int i = 0; i < _FO_FIR_SIZE; i++) {
			_fo_step(i, idx, n, signal);
		}

		return signal;
	}
};

template <typename SAMPLER, int RATIO, int ZERO_CROSSINGS>
using KaiserBesselFirOversampler = FirOversampler<SAMPLER, RATIO, ZERO_CROSSINGS, kaiser_bessel>;
