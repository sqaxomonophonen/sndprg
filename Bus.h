#pragma once

#include <string.h>

#include "assert.h"

template <typename _T, int _CH>
struct Bus {
	typedef _T T;
	static constexpr int CH = _CH;

	T value[CH];

	Bus() = default;

	T& operator[](int index)
	{
		ASSERT(index >= 0 && index < CH);
		return value[index];
	}

	void accumulate(Bus<T,CH> other)
	{
		for (int i = 0; i < CH; i++) {
			value[i] += other.value[i];
		}
	}

	Bus<T,CH> scale(T scalar)
	{
		Bus<T,CH> result;
		for (int i = 0; i < CH; i ++) {
			result.value[i] = value[i] * scalar;
		}
		return result;
	}

	T sum()
	{
		T result = T();
		for (int i = 0; i < CH; i++) {
			result += value[i];
		}
		return result;
	}

	Bus<T,CH> operator*(T scalar)
	{
		return scale(scalar);
	}

	void operator+=(Bus<T,CH> other)
	{
		accumulate(other);
	}

	void operator=(const T& v)
	{
		static_assert(CH == 1, "assignment from values of 'typename T' only applicable to mono signals");
		value[0] = v;
	}
};


typedef Bus<float, 1> FloatMono;
typedef Bus<float, 2> FloatStereo;


