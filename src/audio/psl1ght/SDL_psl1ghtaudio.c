/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org

    This file written by Ryan C. Gordon (icculus@icculus.org)
*/
#include "SDL_config.h"

/* Output audio to PSL1GHT */


#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_psl1ghtaudio.h"

#define AUDIO_DEBUG

#ifdef AUDIO_DEBUG
#define deprintf(...) printf(__VA_ARGS__)
#else
#define deprintf(...)
#endif

static int
PSL1GHT_AUD_OpenDevice(_THIS, const char *devname, int iscapture)
{
	deprintf( "PSL1GHT_AUD_OpenDevice(%08X, %s, %d)\n", (u32)this, devname, iscapture);
    SDL_AudioFormat test_format = SDL_FirstAudioFormat(this->spec.format);
    int valid_datatype = 0;

    this->hidden = SDL_malloc(sizeof(*(this->hidden)));
    if (!this->hidden) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));


	// PS3 Libaudio only handles floats
    while ((!valid_datatype) && (test_format)) {
        this->spec.format = test_format;
        switch (test_format) {
        case AUDIO_F32LSB: // FIXME maybe the float is MSB ?
            valid_datatype = 1;
            break;
        default:
            test_format = SDL_NextAudioFormat();
            break;
        }
    }

    int ret=audioInit();

	//set some parameters we want
	//either 2 or 8 channel
	_params.numChannels = AUDIO_PORT_2CH;
	//8 16 or 32 block buffer
	_params.numBlocks = AUDIO_BLOCK_8;
	//extended attributes
	_params.attr = 0;
	//sound level (1 is default)
	_params.level = 1;

	ret=audioPortOpen(&_params, &_portNum);
	deprintf("audioPortOpen: %d\n",ret);
	deprintf("  portNum: %d\n",_portNum);

	ret=audioGetPortConfig(_portNum, &_config);
	deprintf("audioGetPortConfig: %d\n",ret);
	deprintf("  readIndex: 0x%8X\n",_config.readIndex);
	deprintf("  status: %d\n",_config.status);
	deprintf("  channelCount: %ld\n",_config.channelCount);
	deprintf("  numBlocks: %ld\n",_config.numBlocks);
	deprintf("  portSize: %d\n",_config.portSize);
	deprintf("  audioDataStart: 0x%8X\n",_config.audioDataStart);

	ret=audioPortStart(_portNum);
	deprintf("audioPortStart: %d\n",ret);

	_last_filled_buf = 1;

	this->spec.format = test_format;
	this->spec.size = sizeof(float) * AUDIO_BLOCK_SAMPLES * _config.channelCount;
	this->spec.samples = AUDIO_BLOCK_SAMPLES;
	this->spec.channels = _config.channelCount;

    return ret == 0;
}

static void
PSL1GHT_AUD_PlayDevice(_THIS)
{
	deprintf( "PSL1GHT_AUD_PlayDevice(%08X)\n", (u32)this);
	
	while( _config.readIndex == _last_filled_buf) // FIXME this is a mess to remove when queued event will me integrated
	{
		deprintf( "\tplaying too fast... waiting a ms\n");
		sleep(1);
	}
    /*TransferSoundData *sound = SDL_malloc(sizeof(TransferSoundData));
    if (!sound) {
        SDL_OutOfMemory();
    }

    playGenericSound(this->hidden->mixbuf, this->hidden->mixlen);*/
}


static void
PSL1GHT_AUD_CloseDevice(_THIS)
{
	deprintf( "PSL1GHT_AUD_CloseDevice(%08X)\n", (u32)this);
	int ret = 0;
	ret=audioPortStop(this->hidden->portNum);

	ret=audioPortClose(this->hidden->portNum);

	ret=audioQuit();
    SDL_free(this->hidden);
}

static Uint8 *
PSL1GHT_AUD_GetDeviceBuf(_THIS)
{
	
	deprintf( "PSL1GHT_AUD_GetDeviceBuf(%08X) at %d ms\n", (u32)this, SDL_GetTicks());

    //int playing = _config.readIndex;
    int playing = _config.readIndex;
    int filling = (playing + 1) % _config.numBlocks;
	deprintf( "\tWriting to buffer %d\n", filling);
	Uint8 * dma_buf = (Uint8 *)(void *)_config.audioDataStart;

	_last_filled_buf = filling;
    return (dma_buf + (filling * this->spec.size));
}

/* This function waits until it is possible to write a full sound buffer */
static void
ALSA_WaitDevice(_THIS)
{
    /* We're in blocking mode, so there's nothing to do here */
	deprintf( "ALSA_WaitDevice(%08X)\n", (u32)this);
	
	int i = 5; // Wait 5ms max
	while( _config.readIndex == _last_filled_buf && i != 0)  // FIXME this is a mess to remove when queued event will me integrated
	{
		deprintf( "\tplaying too fast... waiting a ms\n");
		sleep(1);
		i--;
	}
}


	static int
PSL1GHT_AUD_Init(SDL_AudioDriverImpl * impl)
{
	deprintf( "PSL1GHT_AUD_Init(%08X)\n", (u32)impl);
	/* Set the function pointers */
	impl->OpenDevice = PSL1GHT_AUD_OpenDevice;
	impl->PlayDevice = PSL1GHT_AUD_PlayDevice;
    impl->WaitDevice = ALSA_WaitDevice;
	impl->CloseDevice = PSL1GHT_AUD_CloseDevice;
	impl->GetDeviceBuf = PSL1GHT_AUD_GetDeviceBuf;

    /* and the capabilities */
    impl->OnlyHasDefaultOutputDevice = 1;

    return 1;   /* this audio target is available. */
}

AudioBootStrap PSL1GHT_AUD_bootstrap = {
    "psl1ght", "SDL PSL1GHT audio driver", PSL1GHT_AUD_Init, 0       /*1? */
};

/* vi: set ts=4 sw=4 expandtab: */
