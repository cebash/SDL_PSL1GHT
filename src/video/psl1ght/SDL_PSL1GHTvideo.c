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

/* PSL1GHT SDL video driver implementation (for PS3). Based on Dummy driver.
 *
 * Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "DUMMY" by Sam Lantinga.
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_PSL1GHTvideo.h"
#include "SDL_PSL1GHTevents_c.h"
#include "SDL_PSL1GHTmodes_c.h"


#include <malloc.h>
#include <assert.h>
#include <unistd.h>

#include <rsx/rsx.h>

#define PSL1GHTVID_DRIVER_NAME "psl1ght"

/* Initialization/Query functions */
static int PSL1GHT_VideoInit(_THIS);
static void PSL1GHT_VideoQuit(_THIS);

/* PS3GUI init functions : */
static void initializeGPU(SDL_DeviceData * devdata);

/* PSL1GHT driver bootstrap functions */

static int
PSL1GHT_Available(void)
{
    return (1);
}

static void
PSL1GHT_DeleteDevice(SDL_VideoDevice * device)
{
    deprintf (1, "PSL1GHT_DeleteDevice( %16X)\n", device);
    SDL_free(device);
}

static SDL_VideoDevice *
PSL1GHT_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;
    deprintf (1, "PSL1GHT_CreateDevice( %16X)\n", devindex);

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (device) {
        SDL_memset(device, 0, (sizeof *device));
    }
    else {
        SDL_OutOfMemory();
        SDL_free(device);
        return (0);
    }

    /* Set the function pointers */
    device->VideoInit = PSL1GHT_VideoInit;
    device->VideoQuit = PSL1GHT_VideoQuit;
    device->SetDisplayMode = PSL1GHT_SetDisplayMode;
    device->GetDisplayModes = PSL1GHT_GetDisplayModes;
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
    SDL_DeviceData *devdata = NULL;

    devdata = (SDL_DeviceData*) SDL_calloc(1, sizeof(SDL_DeviceData));
    if (devdata == NULL) { 
        /* memory allocation problem */  
        SDL_OutOfMemory();
        return -1;
    } 

    _this->driverdata = devdata;

    PSL1GHT_InitSysEvent(_this);

    initializeGPU(devdata);
    PSL1GHT_InitModes(_this);

    gcmSetFlipMode(GCM_FLIP_VSYNC); // Wait for VSYNC to flip

    /* We're done! */
    return 0;
}

void
PSL1GHT_VideoQuit(_THIS)
{
    deprintf (1, "PSL1GHT_VideoQuit()\n");
    PSL1GHT_QuitModes(_this);
    PSL1GHT_QuitSysEvent(_this);
    SDL_free( _this->driverdata);

}

void initializeGPU( SDL_DeviceData * devdata)
{
    deprintf (1, "initializeGPU()\n");
   // Allocate a 1Mb buffer, alligned to a 1Mb boundary to be our shared IO memory with the RSX.
    void *host_addr = memalign(1024*1024, 1024*1024);
    assert(host_addr != NULL);

    // Initilise Reality, which sets up the command buffer and shared IO memory
    devdata->_CommandBuffer = rsxInit(0x10000, 1024*1024, host_addr);
    assert(devdata->_CommandBuffer != NULL);
}

/* vi: set ts=4 sw=4 expandtab: */
