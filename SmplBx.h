#pragma once

#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "Slice.h"
#include "assert.h"


struct SmplBx_Loader {
	const char* path;
	int fd = -1;
	struct stat st;
	void* mapptr = MAP_FAILED;
	Slice slice;
	std::string error;

	enum {
		UNKNOWN,
		WAVE
	} fmt = UNKNOWN;

	union {
		struct {
			Slice data;
			uint16_t audio_format;
			uint16_t num_channels;
			uint32_t sample_rate;
			uint32_t byte_rate;
			uint16_t block_align;
			uint16_t bits_per_sample;
		} wav;
	};

	SmplBx_Loader(const char* path) : path(path) {}

	~SmplBx_Loader()
	{
		if (fd != -1) {
			AZ(close(fd));
		}
		if (mapptr != MAP_FAILED) {
			AZ(munmap(mapptr, st.st_size));
		}
	}

	bool open()
	{
		fd = ::open(path, O_RDONLY);
		if (fd == -1) {
			return false;
		}
		int e = fstat(fd, &st);
		if (e == -1) {
			return false;
		}

		return true;
	}

	bool chkfmt_wav_fmt(Slice body)
	{
		ASSERT(body.size >= 16);

		wav.audio_format = body.shiftLE<uint16_t>();

		if (wav.audio_format != 1) {
			error = "unhandled WAVE audio format";
			return false;
		}

		wav.num_channels = body.shiftLE<uint16_t>();
		wav.sample_rate = body.shiftLE<uint32_t>();
		wav.byte_rate = body.shiftLE<uint32_t>();
		wav.block_align = body.shiftLE<uint16_t>();
		wav.bits_per_sample = body.shiftLE<uint16_t>();

		return true;
	}

	void chkfmt_wav_data(Slice body)
	{
		wav.data = body;
	}

	bool chkfmt_wav()
	{
		uint32_t tsz = slice.at(4).trim(4).asLE<uint32_t>();
		if (!slice.at(8).trim(4).matches_string("WAVE")) {
			error = "unknown format (RIFF but not WAVE)";
			return false;
		}

		bool got_fmt = false;
		bool got_data = false;

		Slice chunks = slice.at(8 + 4).trim(tsz - 4);
		while (chunks.size > 0) {
			Slice magic = chunks.trim(4);
			uint32_t sz = chunks.at(4).trim(4).asLE<uint32_t>();
			Slice body = chunks.at(8).trim(sz);

			if (magic.matches_string("fmt ")) {
				got_fmt = true;
				if (!chkfmt_wav_fmt(body)) return false;
			} else if(magic.matches_string("data")) {
				got_data = true;
				chkfmt_wav_data(body);
			}

			chunks = chunks.at(8 + sz);
		}

		if (!got_fmt) {
			error = "'fmt' missing in RIFF WAVE";
			return false;
		}

		if (!got_data) {
			error = "'data' missing in RIFF WAVE";
			return false;
		}

		switch (wav.bits_per_sample) {
			case 16:
			case 8:
				break;
			default:
				error = "unhandled bits per samples in WAVE";
				return false;

		}

		if (wav.byte_rate != (wav.sample_rate * wav.num_channels * (wav.bits_per_sample >> 3))) {
			error = "unexpected WAVE fmt byte rate";
			return false;
		}

		if (wav.block_align != (wav.num_channels * (wav.bits_per_sample >> 3))) {
			error = "unexpected WAVE fmt block align";
			return false;
		}

		if ((wav.data.size % wav.block_align) != 0) {
			error = "data length vs block alignment mismatch in WAVE";
			return false;
		}

		return true;
	}

	bool chkfmt()
	{
		mapptr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
		if (mapptr == MAP_FAILED) {
			error = strerror(errno);
			return false;
		}

		slice = Slice(mapptr, st.st_size);

		Slice magic = slice.trim(4);
		if (magic.matches_string("RIFF")) {
			fmt = WAVE;
			return chkfmt_wav();
		} else {
			error = "unknown audio format";
			return false;
		}
	}

	int get_num_channels()
	{
		switch (fmt) {
			case WAVE: return wav.num_channels;
			default: arghf("no fmt!");
		}
	}

	uint64_t get_num_frames()
	{
		switch (fmt) {
			case WAVE: return wav.data.size / wav.block_align;
			default: arghf("no fmt!");
		}
	}

	float get_sample_rate()
	{
		switch (fmt) {
			case WAVE: return wav.sample_rate;
			default: arghf("no fmt!");
		}
	}

	float get_base()
	{
		switch (fmt) {
			case WAVE: return 440;
			default: arghf("no fmt!");
		}
	}

	template <typename BUS>
	void populate_wav(BUS* data)
	{
		for (int i = 0; i < get_num_frames(); i++) {
			BUS value;
			for (int j = 0; j < get_num_channels(); j++) {
				typename BUS::T v = 0;
				if (wav.bits_per_sample == 16) {
					int16_t iv = wav.data.at(i * 4 + j * 2).trim(2).asLE<int16_t>();
					v = ((decltype(v)) iv) / (decltype(v))32768.0;
				} else if(wav.bits_per_sample == 8) {
					int8_t iv = wav.data.at(i * 2 + j).trim(1).asLE<int8_t>();
					v = ((decltype(v)) iv) / (decltype(v))128.0;
				}
				value[j] = v;
			}
			data[i] = value;
		}
	}

	template <typename BUS>
	void populate(BUS* data)
	{
		ASSERT(get_num_channels() == BUS::CH);
		switch (fmt) {
			case WAVE:
				populate_wav(data);
				break;
			default:
				arghf("unknown format");
		}
	}
};



template <typename T>
struct SmplBxTypeId;


template <>
struct SmplBxTypeId<float> {
	static constexpr const char* value = "f32";
};

template <>
struct SmplBxTypeId<double> {
	static constexpr const char* value = "f64";
};


struct SmplBx {
	template <typename BUS>
	std::string get_cache_path(const char* path)
	{
		std::string p(path);

		int i;
		for (i = p.length() - 1; i >= 0; i--) {
			if (p[i] == '/') {
				break;
			}
		}
		i++;

		ASSERT(i >= 0 && i < p.length());

		p.insert(i, ".cache4smplbx.");

		char buf[64];
		snprintf(buf, sizeof(buf), ".ch%d%s", BUS::CH, SmplBxTypeId<typename BUS::T>::value);
		p += std::string(buf);

		return p;
	}

	template <typename BUS>
	Smpl<BUS>* load(const char* path)
	{
		SmplBx_Loader ldr(path);
		if (!ldr.open()) arghf("could not open %s: %s", path, strerror(errno));

		std::string cache_path = get_cache_path<BUS>(path);

		int fd = open(cache_path.c_str(), O_RDWR);
		bool do_populate = false;
		uint64_t frames = 0;
		if (fd == -1) {
			if (errno != ENOENT) {
				arghf("%s: %s", path, strerror(errno));
			}

			if (!ldr.chkfmt()) arghf("%s in %s", ldr.error.c_str(), path);
			if (ldr.get_num_channels() != BUS::CH) arghf("expected %d channel(s) but found %d in %s", BUS::CH, ldr.get_num_channels(), path);

			fd = open(cache_path.c_str(), O_CREAT | O_RDWR, 0777);
			if (fd == -1) {
				arghf("%s: %s", cache_path.c_str(), strerror(errno));
			}
			frames = ldr.get_num_frames();
			size_t sz = Smpl<BUS>::calc_size(frames);
			if (ftruncate(fd, sz) == -1) {
				arghf("%s: %s", cache_path.c_str(), strerror(errno));
			}

			do_populate = true;
		}

		struct stat st;
		if (fstat(fd, &st) == -1) {
			arghf("fstat: %s: %s", path, strerror(errno));
		}

		void* ptr = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (ptr == MAP_FAILED) {
			arghf("mmap: %s: %s", cache_path.c_str(), strerror(errno));
		}

		auto* smpl = (Smpl<BUS>*) ptr;
		if (do_populate) {
			smpl->frames = frames;
			smpl->sample_rate = ldr.get_sample_rate();
			smpl->base = ldr.get_base();
			ldr.populate<BUS>(smpl->data);
		}

		return smpl;
	}

	template <typename BUS>
	bool can_load(const char* path)
	{
		SmplBx_Loader ldr(path);
		if (!ldr.open()) return false;
		if (!ldr.chkfmt()) return false;
		if (ldr.get_num_channels() != BUS::CH) return false;
		return true;
	}
};



