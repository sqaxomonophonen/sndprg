#pragma once

#include <string.h>

#include "assert.h"


struct Slice {
	char* data;
	size_t size;

	Slice() = default;

	Slice(void* data, size_t size) : data((char*)data), size(size) {}

	Slice at(int index)
	{
		ASSERT(index >= 0 && index <= size);
		return Slice((void*)(data + index), size - index);
	}

	Slice trim(size_t new_size)
	{
		ASSERT(new_size <= size);
		return Slice((void*)data, new_size);
	}

	template <typename T>
	T asLE()
	{
		// TODO ensure little endian
		ASSERT(size == sizeof(T));
		return *((T*)data);
	}

	// TODO asBE ?

	template <typename T>
	T shiftLE()
	{
		// TODO ensure little endian
		ASSERT(size >= sizeof(T));
		T* result = (T*)data;
		data += sizeof(T);
		size -= sizeof(T);
		return *result;
	}

	bool matches_string(const char* other)
	{
		size_t l = strlen(other);
		if (l != size) return false;
		return memcmp(data, other, size) == 0;
	}
};

