#pragma once

#include <map>
#include <tuple>
#include <stdlib.h>

#include "Math.h"


template <typename T>
struct Tables {
	static Tables<T>& get_instance()
	{
		static Tables<T> instance;
		return instance;
	}

	Tables() {}
	Tables(Tables<T> const&);
	void operator=(Tables<T> const&);

	// compact downsampler FIR

	typedef std::tuple<int,int,double(*)(double)> CompactDownsamplerFIR_Key;
	std::map<CompactDownsamplerFIR_Key, T*> compact_downsampler_firs;

	T* mk_compact_downsampler_fir(int ratio, int zero_crossings, double (*window_fn)(double))
	{
		int real_fir_size = ratio * zero_crossings;
		int fir_size = real_fir_size - zero_crossings;
		T* tbl = (T*) malloc(sizeof(T) * fir_size);

		double l = 1.0;
		int n = ratio - 1;
		for(int i = 0; i < fir_size ; i++) {
			double x = (l * M_PI) / (double)ratio;
			double A = sin(x) / x; // sinc
			double W = window_fn(l / (double)real_fir_size);
			tbl[i] = A*W;
			l += 1.0;
			n--;
			if(n == 0) {
				// skip zero crossing
				l += 1.0;
				n = ratio - 1;
			}
		}

		return tbl;
	}

	T* get_compact_downsampler_fir(int ratio, int zero_crossings, double (*window_fn)(double))
	{
		CompactDownsamplerFIR_Key key(ratio, zero_crossings, window_fn);
		if (compact_downsampler_firs.count(key) == 0) {
			compact_downsampler_firs[key] = mk_compact_downsampler_fir(ratio, zero_crossings, window_fn);
		}
		return compact_downsampler_firs[key];
	}

	// sinc
	typedef std::tuple<double,double,int,int> PhasedSinc_Key;
	std::map<PhasedSinc_Key, T*> phased_sincs;

	T* mk_phased_sinc(double beta, double lowpass_factor, int width_exp, int phases_exp)
	{
		int width = 1 << width_exp;
		int width_mask = width - 1;
		int phases = 1 << phases_exp;
		double I0_beta = bessel_I0(beta);
		double kPi = 4.0 * atan(1.0) * lowpass_factor;

		T* tbl = (T*) malloc(sizeof(T) * phases * width);

		for (int isrc = 0; isrc < width * phases; isrc++) {
			double fsinc;
			int ix = (width_mask - (isrc & width_mask)) * phases + (isrc >> width_exp);
			if (ix == (4 * phases)) {
				fsinc = 1.0;
			} else {
				double x = (double)(ix - (4 * phases)) * (double)(1.0 / phases);
				fsinc = sin(x * kPi) * bessel_I0(beta * sqrt(1 - x*x*(1.0/16.0))) / (I0_beta*x*kPi); // Kaiser window
			}
			tbl[isrc] = fsinc * lowpass_factor;
		}

		return tbl;
	}

	T* get_phased_sinc(double beta, double lowpass_factor, int width_exp, int phases_exp)
	{
		PhasedSinc_Key key(beta, lowpass_factor, width_exp, phases_exp);
		if (phased_sincs.count(key) == 0) {
			phased_sincs[key] = mk_phased_sinc(beta, lowpass_factor, width_exp, phases_exp);
		}
		return phased_sincs[key];
	}
};

