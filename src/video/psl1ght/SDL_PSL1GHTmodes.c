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

#include "SDL_PSL1GHTvideo.h"
#include "../SDL_sysvideo.h"
#include "SDL_timer.h"

#include <sysutil/video.h>

#include <assert.h>

void
PSL1GHT_InitModes(_THIS)
{
    deprintf(1, "+PSL1GHT_InitModes()\n");
    SDL_DisplayMode mode;
    PSL1GHT_DisplayModeData *modedata;
    videoState state;

    modedata = (PSL1GHT_DisplayModeData *) SDL_malloc(sizeof(*modedata));
    if (!modedata) {
        return;
    }

    assert(videoGetState(0, 0, &state) == 0); // Get the state of the display
    assert(state.state == 0); // Make sure display is enabled

    // Get the current resolution
	videoResolution res;
    assert(videoGetResolution(state.displayMode.resolution, &res) == 0);

    /* Setting up the DisplayMode based on current settings */
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.refresh_rate = 0;
    mode.w = res.width;
    mode.h = res.height;

    modedata->vconfig.resolution = state.displayMode.resolution;
    modedata->vconfig.format = VIDEO_BUFFER_FORMAT_XRGB;
    modedata->vconfig.pitch = res.width * 4;
    mode.driverdata = modedata;

    /* Setup the display to it's  default mode */
    assert(videoConfigure(0, &modedata->vconfig, NULL, 1) == 0);

	// Wait until RSX is ready
	do{
		SDL_Delay(10);
		assert( videoGetState(0, 0, &state) == 0);
	}while ( state.state == 3);

    /* Set display's videomode and add it */
    SDL_AddBasicVideoDisplay(&mode);

    deprintf(1, "-PSL1GHT_InitModes()\n");
}

/* DisplayModes available on the PS3 */
static SDL_DisplayMode ps3fb_modedb[] = {
    /* Native resolutions (progressive, "fullscreen") */
    {SDL_PIXELFORMAT_ARGB8888, 1920, 1080, 0, NULL}, // 1080p
    {SDL_PIXELFORMAT_ARGB8888, 1280, 720, 0, NULL}, // 720p
    {SDL_PIXELFORMAT_ARGB8888, 720, 480, 0, NULL}, // 480p
    {SDL_PIXELFORMAT_ARGB8888, 720, 576, 0, NULL}, // 576p
};

/* PS3 videomode number according to ps3fb_modedb */
static PSL1GHT_DisplayModeData ps3fb_data[] = {
	// { resolution, format, aspect, padding, pitch }
	{{
		VIDEO_RESOLUTION_1080, 
		VIDEO_BUFFER_FORMAT_XRGB,
		VIDEO_ASPECT_16_9, 
		{0, 0, 0, 0, 0, 0, 0, 0, 0},
		1920 * 4
	}},
	{{
		VIDEO_RESOLUTION_720, 
		VIDEO_BUFFER_FORMAT_XRGB,
		VIDEO_ASPECT_16_9, 
		{0, 0, 0, 0, 0, 0, 0, 0, 0},
		1280 * 4
	}},
	{{
		VIDEO_RESOLUTION_480, 
		VIDEO_BUFFER_FORMAT_XRGB,
		VIDEO_ASPECT_16_9, 
		{0, 0, 0, 0, 0, 0, 0, 0, 0},
		720 * 4
	}},
	{{
		VIDEO_RESOLUTION_576, 
		VIDEO_BUFFER_FORMAT_XRGB,
		VIDEO_ASPECT_16_9, 
		{0, 0, 0, 0, 0, 0, 0, 0, 0},
		720 * 4
	}},
};

void
PSL1GHT_GetDisplayModes(_THIS, SDL_VideoDisplay * display)
{
    deprintf(1, "+PSL1GHT_GetDisplayModes()\n");
    unsigned int nummodes;

    nummodes = sizeof(ps3fb_modedb) / sizeof(SDL_DisplayMode);

    int n;
    for (n=0; n<nummodes; ++n) {
        /* Get driver specific mode data */
        ps3fb_modedb[n].driverdata = &ps3fb_data[n];

        /* Add DisplayMode to list */
        deprintf(2, "Adding resolution %u x %u\n", ps3fb_modedb[n].w, ps3fb_modedb[n].h);
        SDL_AddDisplayMode(display, &ps3fb_modedb[n]);
    }
    deprintf(1, "-PSL1GHT_GetDisplayModes()\n");
}

int
PSL1GHT_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    deprintf(1, "+PSL1GHT_SetDisplayMode()\n");
    PSL1GHT_DisplayModeData *dispdata = (PSL1GHT_DisplayModeData *) mode->driverdata;
	videoState state;

    /* Set the new DisplayMode */
    deprintf(2, "Setting PS3_MODE to %u\n", dispdata->vconfig.resolution);
    if ( videoConfigure(0, &dispdata->vconfig, NULL, 0) != 0)
	{
        deprintf(2, "Could not set PS3FB_MODE\n");
        SDL_SetError("Could not set PS3FB_MODE\n");
        return -1;
    }

	// Wait until RSX is ready
	do{
		SDL_Delay(10);
		assert( videoGetState(0, 0, &state) == 0);
	}while ( state.state == 3);

    deprintf(1, "-PSL1GHT_SetDisplayMode()\n");
    return 0;
}

void
PSL1GHT_QuitModes(_THIS)
{
    deprintf(1, "+PSL1GHT_QuitModes()\n");

    /* There was no mem allocated for driverdata */
    int i, j;
    for (i = 0; i < SDL_GetNumVideoDisplays(); ++i) {
        SDL_VideoDisplay *display = &_this->displays[i];
        for (j = display->num_display_modes; j--;) {
            display->display_modes[j].driverdata = NULL;
        }
    }
    // TODO : Free data
    deprintf(1, "-PSL1GHT_QuitModes()\n");
}

/* vi: set ts=4 sw=4 expandtab: */
