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

/* Dummy SDL video driver implementation; this is just enough to make an
 *  SDL-based application THINK it's got a working video driver, for
 *  applications that call SDL_Init(SDL_INIT_VIDEO) when they don't need it,
 *  and also for use as a collection of stubs when porting SDL to a new
 *  platform for which you haven't yet written a valid video driver.
 *
 * This is also a great way to determine bottlenecks: if you think that SDL
 *  is a performance problem for a given platform, enable this driver, and
 *  then see if your application runs faster without video overhead.
 *
 * Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "PSL1GHT" by Sam Lantinga.
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_PSL1GHTvideo.h"
#include "SDL_PSL1GHTevents_c.h"
#include "SDL_PSL1GHTrender_c.h"


#include <malloc.h>
#include <assert.h>

#include <rsx/reality.h>

#define PSL1GHTVID_DRIVER_NAME "psl1ght"

/* Initialization/Query functions */
static int PSL1GHT_VideoInit(_THIS);
static int PSL1GHT_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
static void PSL1GHT_VideoQuit(_THIS);

/* PS3GUI init functions : */
static void initializeDoubleBuffer(_THIS);
static void setupScreenMode(_THIS);
static void initializeGPU(_THIS);

/* PSL1GHT driver bootstrap functions */

static int
PSL1GHT_Available(void)
{
    const char *envr = SDL_getenv("SDL_VIDEODRIVER");
    if ((envr) && (SDL_strcmp(envr, PSL1GHTVID_DRIVER_NAME) == 0)) {
        return (1);
    }

    return (0);
}

static void
PSL1GHT_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device);
}

static SDL_VideoDevice *
PSL1GHT_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (device) {
        SDL_memset(device, 0, (sizeof *device));
        device->gl_data = (struct SDL_GLDriverData*)
            SDL_malloc(sizeof *device->gl_data );
    }
    if ((device == NULL) || (device->gl_data == NULL)) {
        SDL_OutOfMemory();
        if (device) {
            SDL_free(device);
        }
        return (0);
    }
	SDL_memset(device->gl_data, 0, (sizeof *device->gl_data));

    /* Set the function pointers */
    device->VideoInit = PSL1GHT_VideoInit;
    device->VideoQuit = PSL1GHT_VideoQuit;
    device->SetDisplayMode = PSL1GHT_SetDisplayMode;
    device->PumpEvents = PSL1GHT_PumpEvents;

    device->free = PSL1GHT_DeleteDevice;

    return device;
}

VideoBootStrap PSL1GHT_bootstrap = {
    PSL1GHTVID_DRIVER_NAME, "SDL psl1ght video driver",
    PSL1GHT_Available, PSL1GHT_CreateDevice
};

int
PSL1GHT_VideoInit(_THIS)
{
    SDL_DisplayMode mode;

	initializeDoubleBuffer(_this);
	setupScreenMode(_this);
	initializeGPU(_this);

    /* Use a fake 32-bpp desktop mode */
    mode.format = SDL_PIXELFORMAT_RGB888;
    mode.w = _this->gl_data->_resolution.width;
    mode.h = _this->gl_data->_resolution.height;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;
    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }
    SDL_AddRenderDriver(&_this->displays[0], &SDL_PSL1GHT_RenderDriver);

    SDL_zero(mode);
    SDL_AddDisplayMode(&_this->displays[0], &mode);

    /* We're done! */
    return 0;
}

static int
PSL1GHT_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    return 0;
}

void
PSL1GHT_VideoQuit(_THIS)
{
}

void initializeGPU(_THIS)
{
   // Allocate a 1Mb buffer, alligned to a 1Mb boundary to be our shared IO memory with the RSX.
    void *host_addr = memalign(1024*1024, 1024*1024);
    assert(host_addr != NULL);

    // Initilise Reality, which sets up the command buffer and shared IO memory
    _this->gl_data->_CommandBuffer = realityInit(0x10000, 1024*1024, host_addr);
    assert(_this->gl_data->_CommandBuffer != NULL);
}

void setupScreenMode(_THIS)
{
    VideoState state;
    assert(videoGetState(0, 0, &state) == 0); // Get the state of the display
    assert(state.state == 0); // Make sure display is enabled

    // Get the current resolution
    assert(videoGetResolution(state.displayMode.resolution, &_this->gl_data->_resolution) == 0);

    // Configure the buffer format to xRGB
    VideoConfiguration vconfig;
    memset(&vconfig, 0, sizeof(VideoConfiguration));
    vconfig.resolution = state.displayMode.resolution;
    vconfig.format = VIDEO_BUFFER_FORMAT_XRGB;
    vconfig.pitch = _this->gl_data->_resolution.width * 4;

    assert(videoConfigure(0, &vconfig, NULL, 0) == 0);
    assert(videoGetState(0, 0, &state) == 0);

    gcmSetFlipMode(GCM_FLIP_VSYNC); // Wait for VSYNC to flip
}

void initializeDoubleBuffer(_THIS)
{
	VideoResolution _resolution = _this->gl_data->_resolution;
    s32 buffer_size = 4 * _resolution.width * _resolution.height; // each pixel is 4 bytes
    //printf("buffers will be 0x%x bytes\n", buffer_size);

    // Allocate two buffers for the RSX to draw to the screen (double buffering)
    _this->gl_data->buffer[0] = (s32 *) rsxMemAlign(16, buffer_size);
    _this->gl_data->buffer[1] = (s32 *) rsxMemAlign(16, buffer_size);
    assert(_this->gl_data->buffer[0] != NULL && _this->gl_data->buffer[1] != NULL);

    u32 offset[2];
    assert(realityAddressToOffset(_this->gl_data->buffer[0], &offset[0]) == 0);
    assert(realityAddressToOffset(_this->gl_data->buffer[1], &offset[1]) == 0);
    // Setup the display buffers
    assert(gcmSetDisplayBuffer(0, offset[0], _resolution.width * 4, _resolution.width, _resolution.height) == 0);
    assert(gcmSetDisplayBuffer(1, offset[1], _resolution.width * 4, _resolution.width, _resolution.height) == 0);

    gcmResetFlipStatus();
}

/* vi: set ts=4 sw=4 expandtab: */
