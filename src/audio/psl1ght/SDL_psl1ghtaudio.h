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
*/
#include "SDL_config.h"

#pragma once

#include "../SDL_sysaudio.h"

#include <audio/audio.h>
/* Hidden "this" pointer for the audio functions */
#define _THIS	SDL_AudioDevice *this

struct SDL_PrivateAudioData
{
    //:TransferSoundData *sound;
    /* The file descriptor for the audio device */

	AudioPortParam params;
	AudioPortConfig config;
	u32 portNum;
	u32 last_filled_buf;
};

#define _params this->hidden->params
#define _config this->hidden->config
#define _portNum this->hidden->portNum
#define _last_filled_buf this->hidden->last_filled_buf

/* vi: set ts=4 sw=4 expandtab: */
