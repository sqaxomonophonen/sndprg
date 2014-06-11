
#include "Bus.h"
#include "Smpl.h"
#include "SmplBx.h"
#include "FirOversampler.h"
#include "PQ.h"
#include "Math.h"
#include "Tables.h"

#include <SDL.h>

struct state {
	SmplBx smplbx;
	KaiserBesselFirOversampler<PolyphaseSmplr<FloatStereo>, 10, 2> smplr;

	PQ<void(*)(struct state*)> pq;
	int64_t t = 0;
	int tick = 0;
	int tick_delay = 0;

	void queue(void(*callback)(struct state* state), int64_t dt)
	{
		pq.insert(t + dt, &callback);
	}

	FloatStereo sample()
	{
		return smplr.sample() * 0.1f;
	}
};


static void audio_callback(struct state* state, float* q, int n)
{
	int i = 0;
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
			auto v = state->sample();
			q[i<<1] = v[0];
			q[(i<<1)+1] = v[1];
			i++;
			dt--;
			n--;
			state->t++;
		}
	}
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
	state->smplr.set_hz(440);
	state->smplr.set_pos(0);
	state->queue(song_tick, state->tick_delay);
	state->tick_delay += 10;
	state->tick_delay *= 1.1;
}

void state_init(struct state* state, int sample_rate)
{
	auto* smplr = &state->smplr;
	smplr->set_sample_rate(sample_rate);
	state->smplr.smpl = state->smplbx.load<FloatStereo>("WilhelmScream.wav");

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

