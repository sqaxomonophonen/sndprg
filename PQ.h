#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

template <typename T>
struct PQE {
	uint64_t t;
	T value;
};

template <typename T>
struct PQ {
	size_t sz;
	PQE<T>* es;
	int n;

	PQ(size_t size = 16)
	{
		sz = size;
		es = (decltype(es)) malloc(sizeof(decltype(*es)) * sz);
		n = 0;
	}

	void _swap(PQE<T>* a, PQE<T>* b)
	{
		PQE<T> tmp;
		memcpy(&tmp, a, sizeof(PQE<T>));
		memcpy(a, b, sizeof(PQE<T>));
		memcpy(b, &tmp, sizeof(PQE<T>));
	}

	void insert(uint64_t t, T* value)
	{
		int i = n;
		n++;
		if (n > sz) {
			// grow
			sz = n << 1;
			es = (decltype(es)) realloc(es, sizeof(decltype(*es)) * sz);
		}
		PQE<T> pqe;
		pqe.t = t;
		memcpy(&pqe.value, value, sizeof(T));

		memcpy(&es[i], &pqe, sizeof(decltype(*es)));
		while (i > 0) {
			int pi = (i-1) >> 1;
			PQE<T>* e = &es[i];
			PQE<T>* p = &es[pi];
			if(p->t <= e->t) break;
			_swap(p, e);
			i = pi;
		}
	}

	int64_t next_t()
	{
		return es[0].t;
	}

	void shift(T* value)
	{
		if (value != NULL) {
			memcpy(value, &es[0].value, sizeof(T));
		}

		n--;
		if (n == 0) return;
		memcpy(&es[0], &es[n], sizeof(PQE<T>));
		int i = 0;
		while (true) {
			int left_i = (i<<1) + 1;
			int right_i = (i<<1) + 2;
			int i2 = i;
			if (left_i < n && es[left_i].t < es[i2].t) {
				i2 = left_i;
			}
			if (right_i < n && es[right_i].t < es[i2].t) {
				i2 = right_i;
			}
			if (i2 == i) break;
			_swap(&es[i], &es[i2]);
			i = i2;
		}
	}
};

