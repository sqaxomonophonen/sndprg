#pragma once

#include <type_traits>

#include "Bus.h"
#include "Tables.h"
#include "assert.h"

template <typename BUS>
struct Smpl {
	static_assert(std::is_pod<BUS>::value, "typename 'BUS' is not POD");

	static constexpr float DEFAULT_SAMPLE_RATE = 44100;
	static constexpr float DEFAULT_BASE = 440;

	uint64_t frames;
	float sample_rate;
	float base;

	BUS data[1];

	static size_t calc_size(uint64_t frames)
	{
		return sizeof(Smpl<BUS>) + sizeof(BUS) * (frames - 1);
	}

	size_t get_size()
	{
		return calc_size(frames);
	}

	static Smpl<BUS>* alloc(uint64_t frames)
	{
		ASSERT(frames > 0);
		size_t sz = calc_size(frames);
		Smpl<BUS>* ptr = (Smpl<BUS>*) malloc(sz);
		AN(ptr);
		ptr->frames = frames;
		ptr->sample_rate = DEFAULT_SAMPLE_RATE;
		ptr->base = DEFAULT_BASE;
		return ptr;
	}

	BUS operator[](int64_t index)
	{
		if (index < 0 || index >= frames) {
			return BUS();
		} else {
			return BUS(data[index]);
		}
	}
};

static_assert(std::is_pod<Smpl<FloatStereo>>::value, "Smpl is not POD");


template <typename BUS>
struct Smplr {
	static constexpr int FRAC_EXP = 20;

	Smpl<BUS>* smpl = nullptr;

	int32_t inc_fx = 0;
	int64_t pos_fx = 0;
	float sample_rate;

	Smplr() {}

	int64_t pos()
	{
		return pos_fx >> FRAC_EXP;
	}

	void set_pos(float value)
	{
		pos_fx = value * (1 << FRAC_EXP);
	}

	void set_sample_rate(float value)
	{
		sample_rate = value;
	}

	void set_hz(float value)
	{
		float inc = value / smpl->base * smpl->sample_rate / sample_rate;
		inc_fx = inc * (1 << FRAC_EXP);
	}

	void advance()
	{
		pos_fx += inc_fx;
	}

};

template <typename BUS, int SINC_WIDTH_EXP = 3, int SINC_PHASES_EXP = 12>
struct PolyphaseSmplr : public Smplr<BUS> {
	static constexpr int SINC_WIDTH = 1 << SINC_WIDTH_EXP;
	static constexpr int SINC_PHASES = 1 << SINC_PHASES_EXP;
	static constexpr int SINC_MASK = SINC_PHASES - 1;

	static constexpr int OFFSET = -(SINC_WIDTH >> 1) + 1;
	static constexpr int POS_MIN = -(SINC_WIDTH >> 1) - 1;
	static constexpr int POS_MAX_OFFSET = (SINC_WIDTH >> 1) - 1;

	typedef BUS T;
	typedef typename BUS::T Q;

	Q* _pp_kaiser_sinc;
	Q* _pp_down2x;
	Q* _pp_down1_333x;

	PolyphaseSmplr()
	{
		Tables<Q>& tables = Tables<Q>::get_instance();
		_pp_kaiser_sinc = tables.get_phased_sinc(9.6377, 0.97, SINC_WIDTH_EXP, SINC_PHASES_EXP);
		_pp_down2x = tables.get_phased_sinc(2.7625, 0.425, SINC_WIDTH_EXP, SINC_PHASES_EXP);
		_pp_down1_333x = tables.get_phased_sinc(8.5, 0.5, SINC_WIDTH_EXP, SINC_PHASES_EXP);
	}

	inline Q* _pp_get_sinc_table()
	{
		int d = abs(this->inc_fx >> (this->FRAC_EXP - 4));
		if (d > 0x18) {
			return _pp_down2x;
		} else if(d > 0x12) {
			return _pp_down1_333x;
		} else {
			return _pp_kaiser_sinc;
		}
	}

	BUS sample()
	{
		const int64_t p = this->pos();
		if (p < POS_MIN || p > (this->smpl->frames + POS_MAX_OFFSET)) {
			this->advance();
			return BUS();
		}

		Q* lut = _pp_get_sinc_table() + ((this->pos_fx >> (this->FRAC_EXP - SINC_PHASES_EXP )) & SINC_MASK) * SINC_WIDTH;

		BUS value = BUS();
		for (int i = 0; i < SINC_WIDTH; i++) {
			value.accumulate( ( (*this->smpl)[p + i + OFFSET]).scale(lut[i]));
		}
		this->advance();
		return value;
	}
};


