
#include "Skaar.h"
#include "PQ.h"
#include "Math.h"
#include "F6581.h"
#include "FirOversampler.h"
#include "ADSR.h"

#include <SDL.h>

struct state {
	KaiserBesselFirOversampler<Skaar<2, F6581<>, SkaarOsc<ADSR>>, 16, 2> skaar;
	PQ<void(*)(struct state*)> pq;
	int64_t t = 0;
	int tick = 0;
	float t2 = 0;

	void queue(void(*callback)(struct state* state), int64_t dt)
	{
		pq.insert(t + dt, &callback);
	}

};


#define OFF (-1000)
#define IDLE (-1001)

static int some_notes[] = {
	0,
	3,
	7,
	12,
	OFF,
	10,
	3,
	7,
	-2,
	OFF,
	2,
	3,
	5,
	IDLE,
	OFF,
	IDLE
};

static int some_notes_bass[] = {
	0,
	0,
	3,
	3,
	OFF,
	3,
	3,
	5,
	IDLE,
	-5,
	OFF,
	7,
	-2,
	-2,
	5,
	7
};


static void song_tick(struct state* state)
{
	printf("\r%sTICK\033[00m %d", (state->tick&1?"\033[37m\033[40m":"\033[30m\033[47m"), state->tick);
	fflush(stdout);
	int tick = state->tick++;
	auto& skaar = state->skaar;
	int note = some_notes[tick % 16];
	if(note == IDLE) {
	} else if(note == OFF) {
		skaar.osc[0].env.off();
	} else {
		skaar.osc[0].set_hz(note_to_hz(200.0f, note));
		skaar.osc[0].env.on();
	}
	int noteb = some_notes_bass[tick % 16];
	if(noteb == IDLE) {
	} else if(noteb == OFF) {
		skaar.osc[1].env.off();
	} else {
		skaar.osc[1].set_hz(note_to_hz(100.0f, noteb));
		skaar.osc[1].env.on();
	}
	state->queue(song_tick, 10000);
}

static void song_motion_tick(struct state* state)
{
	auto& skaar = state->skaar;
	skaar.osc[0].set_shift(sinf(state->t2) * 0.48f);
	skaar.filter.set_fc(sinf(state->t2*1.3f) * 1000 + 1000);
	state->t2+=0.006f;
	state->queue(song_motion_tick, 1000);
}

static void song_init(struct state* state)
{
	auto& skaar = state->skaar;

	skaar.gain = 0.05f;
	{
		auto& osc = skaar.osc[0];

		osc.flags = osc.SQR;
		osc.gain_filter = 1.0f;
		osc.gain = 0.1f;

		auto& env = osc.env;
		env.set_attack(0.01f, 0.01f);
		env.set_decay(0.1f, 1.0f);
		env.set_sustain(0.1f);
		env.set_release(0.5f, 1.0f);
	}
	{
		auto& osc = skaar.osc[1];

		osc.flags = osc.SAW;
		osc.gain_filter = 0.5f;
		osc.gain = 0.01f;

		auto& env = osc.env;
		env.set_attack(0.1f, 0.01f);
		env.set_decay(0.2f, 1.0f);
		env.set_sustain(0.3f);
		env.set_release(0.5f, 1.0f);
	}

	auto& filter = skaar.filter;
	filter.set_fc(2048.0f - 900);
	filter.set_q(0.4f);
	filter.lowpass_gain = 1.0f;
	filter.highpass_gain = 0.3f;

	state->queue(song_tick, 0);
	state->queue(song_motion_tick, 0);
}


void state_init(struct state* state, int sample_rate)
{
	state->skaar.set_sample_rate(sample_rate);
	state->queue(song_init, 0);

}

static void audio_callback(struct state* state, float* q, int n)
{
	auto& skaar = state->skaar;
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
			float v = skaar.sample();
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


////

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

	//SDL_Delay(1000);
	fgetc(stdin);

	return 0;
}
