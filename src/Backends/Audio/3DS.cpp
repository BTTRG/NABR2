#include "../Audio.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <3ds.h>

#include "../Misc.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, y, z) MIN(MAX((x), (y)), (z))

typedef struct AudioBackend_Sound
{
	signed char *samples;
	ndspWaveBuf wave_buffer;
	unsigned int frequency;
	float volume;
	float pan_l;
	float pan_r;
	bool looping;
	int channel;
	unsigned int identifier;
} AudioBackend_Sound;

static struct
{
	unsigned int sound_identifier;
	AudioBackend_Sound *sound;
} channels[24];

static void (*organya_callback)(void);
static unsigned int organya_callback_timer;

static Thread audio_thread;
static bool audio_thread_die;

static void OrganyaThread(void *user_data)
{
	(void)user_data;

	while (!audio_thread_die)
	{
		if (organya_callback_timer == 0)
		{
			// If Organya isn't currently playing, idle for 10ms and check again
			svcSleepThread(10 * 1000000);
		}
		else
		{
			organya_callback();

			svcSleepThread(organya_callback_timer * 1000000);
		}
	}
}

static float MillibelToScale(long volume)
{
	// Volume is in hundredths of a decibel, from 0 to -10000
	volume = CLAMP(volume, -10000, 0);
	return pow(10.0f, volume / 2000.0f);
}

static int AllocateChannel(AudioBackend_Sound *sound)
{
	// Search for a channel which either doesn't have an assigned sound,
	// or whose assigned sound has since stopped playing.
	for (int i = 0; i < 24; ++i)
	{
		if (channels[i].sound_identifier == 0
		 || channels[i].sound->wave_buffer.status == NDSP_WBUF_FREE
		 || channels[i].sound->wave_buffer.status == NDSP_WBUF_DONE)
		{
			channels[i].sound_identifier = sound->identifier;
			channels[i].sound = sound;

			return i;
		}
	}

	Backend_PrintInfo("Ran out of sound channels - hey you, whatever you're doing, stop it!");

	return -1;
}

bool AudioBackend_Init(void)
{
	ndspInit();

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);

	audio_thread_die = false;
	audio_thread = threadCreate(OrganyaThread, NULL, 32 * 1024, 0x18, -1, false);

	return true;
}

void AudioBackend_Deinit(void)
{
	audio_thread_die = true;
	threadJoin(audio_thread, UINT64_MAX);
	threadFree(audio_thread);

	ndspExit();
}

AudioBackend_Sound* AudioBackend_CreateSound(unsigned int frequency, const unsigned char *samples, size_t length)
{
	static unsigned int identifier_allocator;

	AudioBackend_Sound *sound = (AudioBackend_Sound*)malloc(sizeof(AudioBackend_Sound));

	if (sound != NULL)
	{
		sound->samples = (signed char*)linearAlloc(length);

		if (sound->samples != NULL)
		{
			for (size_t i = 0; i < length; ++i)
				sound->samples[i] = samples[i] - 0x80;

			DSP_FlushDataCache(sound->samples, length);

			memset(&sound->wave_buffer, 0, sizeof(sound->wave_buffer));
			sound->wave_buffer.data_vaddr = sound->samples;
			sound->wave_buffer.nsamples = length;

			sound->frequency = frequency;
			sound->volume = 1.0f;
			sound->pan_l = 1.0f;
			sound->pan_r = 1.0f;
			sound->looping = false;

			sound->channel = -1;

			do
			{
				sound->identifier = ++identifier_allocator;
			} while (sound->identifier == 0);	// 0 is reserved

			return sound;
		}
		else
		{
			Backend_PrintError("linearAlloc failed in AudioBackend_CreateSound");
		}

		free(sound);
	}
	else
	{
		Backend_PrintError("malloc failed in AudioBackend_CreateSound");
	}

	return NULL;
}

void AudioBackend_DestroySound(AudioBackend_Sound *sound)
{
	if (sound->channel != -1 && channels[sound->channel].sound_identifier == sound->identifier)
	{
		ndspChnWaveBufClear(sound->channel);
		channels[sound->channel].sound_identifier = 0;
		channels[sound->channel].sound = NULL;
	}

	linearFree(sound->samples);
	free(sound);
}

void AudioBackend_PlaySound(AudioBackend_Sound *sound, bool looping)
{
	if (sound->channel == -1 || channels[sound->channel].sound_identifier != sound->identifier)
		sound->channel = AllocateChannel(sound);

	bool previous_looping = sound->looping;
	sound->looping = looping;

	if (sound->channel != -1)
	{
		if (sound->wave_buffer.status == NDSP_WBUF_FREE
		 || sound->wave_buffer.status == NDSP_WBUF_DONE
		 || previous_looping != looping)
		{
			ndspChnWaveBufClear(sound->channel);

			ndspChnSetInterp(sound->channel, NDSP_INTERP_LINEAR);
			ndspChnSetRate(sound->channel, sound->frequency);
			ndspChnSetFormat(sound->channel, NDSP_FORMAT_MONO_PCM8);

			sound->wave_buffer.looping = looping;

			float mix[12];
			memset(mix, 0, sizeof(mix));
			mix[0] = sound->pan_l * sound->volume;
			mix[1] = sound->pan_r * sound->volume;
			ndspChnSetMix(sound->channel, mix);

			ndspChnWaveBufAdd(sound->channel, &sound->wave_buffer);
		}
	}
}

void AudioBackend_StopSound(AudioBackend_Sound *sound)
{
	if (sound->channel != -1 && channels[sound->channel].sound_identifier == sound->identifier)
		ndspChnWaveBufClear(sound->channel);
}

void AudioBackend_RewindSound(AudioBackend_Sound *sound)
{
	(void)sound;
}

void AudioBackend_SetSoundFrequency(AudioBackend_Sound *sound, unsigned int frequency)
{
	sound->frequency = frequency;

	if (sound->channel != -1 && channels[sound->channel].sound_identifier == sound->identifier)
		ndspChnSetRate(sound->channel, frequency);
}

void AudioBackend_SetSoundVolume(AudioBackend_Sound *sound, long volume)
{
	sound->volume = MillibelToScale(volume);

	if (sound->channel != -1 && channels[sound->channel].sound_identifier == sound->identifier)
	{
		float mix[12];
		memset(mix, 0, sizeof(mix));
		mix[0] = sound->pan_l * sound->volume;
		mix[1] = sound->pan_r * sound->volume;
		ndspChnSetMix(sound->channel, mix);
	}
}

void AudioBackend_SetSoundPan(AudioBackend_Sound *sound, long pan)
{
	sound->pan_l = MillibelToScale(-pan);
	sound->pan_r = MillibelToScale(pan);

	if (sound->channel != -1 && channels[sound->channel].sound_identifier == sound->identifier)
	{
		float mix[12];
		memset(mix, 0, sizeof(mix));
		mix[0] = sound->pan_l * sound->volume;
		mix[1] = sound->pan_r * sound->volume;
		ndspChnSetMix(sound->channel, mix);
	}
}

void AudioBackend_SetOrganyaCallback(void (*callback)(void))
{
	organya_callback = callback;
}

void AudioBackend_SetOrganyaTimer(unsigned int milliseconds)
{
	organya_callback_timer = milliseconds;
}
