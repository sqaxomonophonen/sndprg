
#include "Bus.h"
#include "Smpl.h"
#include "FirOversampler.h"
#include "PQ.h"
#include "Math.h"
#include "Tables.h"

#include <SDL.h>

struct state {
	KaiserBesselFirOversampler<PolyphaseSmplr<FloatMono>, 10, 2> smplr;

	PQ<void(*)(struct state*)> pq;
	int64_t t = 0;
	int tick = 0;
	float hz = 4;

	void queue(void(*callback)(struct state* state), int64_t dt)
	{
		pq.insert(t + dt, &callback);
	}

	float sample()
	{
		return smplr.sample().sum() * 0.03f;
	}
};


static void audio_callback(struct state* state, float* q, int n)
{
	int i = 0;
	//float min = 1e6;
	//float max = -1e6;
	while(n > 0) {
		int64_t dt = 0;
		auto& pq = state->pq;
		if(pq.n > 0) {
			while((dt = (pq.next_t() - state->t)) <= 0) {
				void (*callback)(struct state*);
				pq.shift(&callback);
				callback(state);
			}
		}
		while(n > 0 && (pq.n == 0 || dt > 0)) {
			float v = state->sample();
			//if (v > max) max = v;
			//if (v < min) min = v;
			q[i<<1] = q[(i<<1)+1] = v;
			i++;
			dt--;
			n--;
			state->t++;
		}
	}
	//printf("[%f:%f]\n", min, max);
}


static void sdl_panic()
{
	fprintf(stderr, "SDL: %s\n", SDL_GetError());
	exit(EXIT_FAILURE);
}

static void sdl_audio_callback(void* usr, Uint8* stream, int len)
{
	struct state* state = (struct state*) usr;
	int n = len / (sizeof(float)*2);
	float* fstream = (float*) stream;
	audio_callback(state, fstream, n);
}

static void song_tick(struct state* state)
{
	state->smplr.set_hz(state->hz);
	state->hz *= 1.01f;
	state->queue(song_tick, 1000);
}

void state_init(struct state* state, int sample_rate)
{
	auto* smplr = &state->smplr;
	smplr->set_sample_rate(sample_rate);

	auto* smpl = state->smplr.smpl = Smpl<FloatMono>::alloc(10000000);

	for (int i = 0; i < smpl->frames; i++) {
		smpl->data[i] = randf(-1.0f, 1.0f) * (float)(i & 255) / 256.0f;
	}

	state->queue(song_tick, 0);
}

static void init_audio()
{
	struct state* state = new struct state;

	SDL_AudioSpec want, have;
	SDL_zero(want);
	want.freq = 44100;
	want.format = AUDIO_F32;
	want.channels = 2;
	want.samples = 256;
	want.callback = sdl_audio_callback;
	want.userdata = state;

	SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	if(dev == 0) sdl_panic();

	state_init(state, have.freq);

	SDL_PauseAudioDevice(dev, 0);
}

int main(int argc, char** argv)
{
	if(SDL_Init(SDL_INIT_AUDIO) != 0) sdl_panic();

	init_audio();

	fgetc(stdin);

	return 0;
}
