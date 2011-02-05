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
/*
#include "SDL_video.h"
#include "../SDL_sysvideo.h"
#include "../../render/SDL_sysrender.h"
#include "../../render/software/SDL_draw.h"
#include "../../render/software/SDL_blendfillrect.h"
#include "../../render/software/SDL_blendline.h"
#include "../../render/software/SDL_blendpoint.h"
#include "../../render/software/SDL_drawline.h"
#include "../../render/software/SDL_drawpoint.h"
*/
#include "../SDL_sysvideo.h"
#include "SDL_PSL1GHTvideo.h"

#include <rsx/reality.h>
#include <unistd.h>
#include <assert.h>

struct PSL1GHTSurfaceData
{
	int buffer_num;
	SDL_DeviceData * devdata;
};
typedef struct PSL1GHTSurfaceData PSL1GHTSurfaceData;

#define PSL1GHT_SURFACE   "_SDL_PSL1GHTSurface"

#define MAX_FRAME_BUFFERS 2
static int piUsed[MAX_FRAME_BUFFERS] = {0, 0};
/* SDL surface based renderer implementation */
/*
static SDL_Renderer *SDL_PSL1GHT_CreateRenderer(SDL_Window * window,
                                              Uint32 flags);
static int SDL_PSL1GHT_RenderDrawPoints(SDL_Renderer * renderer,
                                      const SDL_Point * points, int count);
static int SDL_PSL1GHT_RenderDrawLines(SDL_Renderer * renderer,
                                     const SDL_Point * points, int count);
static int SDL_PSL1GHT_RenderFillRects(SDL_Renderer * renderer,
                                     const SDL_Rect ** rects, int count);
static int SDL_PSL1GHT_RenderCopy(SDL_Renderer * renderer,
                                SDL_Texture * texture,
                                const SDL_Rect * srcrect,
                                const SDL_Rect * dstrect);
static int SDL_PSL1GHT_RenderReadPixels(SDL_Renderer * renderer,
                                      const SDL_Rect * rect,
                                      Uint32 format,
                                      void * pixels, int pitch);
static void SDL_PSL1GHT_RenderPresent(SDL_Renderer * renderer);
static void SDL_PSL1GHT_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver PSL1GHT_RenderDriver = {
    SDL_PSL1GHT_CreateRenderer,
    {
        "psl1ght",
        ( 
         SDL_RENDERER_PRESENTVSYNC
        ),
    }
};

typedef struct
{
    int current_screen;
    SDL_Surface *screens[3];
    gcmContextData *context; // Context to keep track of the RSX buffer.    
} SDL_PSL1GHT_RenderData;
*/
static void flip( gcmContextData *context, int current_screen)
{
    assert(gcmSetFlip(context, current_screen) == 0);
    realityFlushBuffer(context);
    gcmSetWaitFlip(context); // Prevent the RSX from continuing until the flip has finished.
}

int SDL_PSL1GHT_CreateWindowFramebuffer(_THIS, SDL_Window * window, Uint32 * format, void ** pixels, int *pitch)
{
	SDL_Surface *surface;
	const Uint32 surface_format = SDL_PIXELFORMAT_RGB888;
	int w, h;
	int bpp;
	Uint32 Rmask, Gmask, Bmask, Amask;

	/* Free the old framebuffer surface */
	surface = (SDL_Surface *) SDL_GetWindowData(window, PSL1GHT_SURFACE);
	if (surface) {
		
		PSL1GHTSurfaceData * data  = (PSL1GHTSurfaceData*) surface->userdata;
		piUsed[data->buffer_num] = 0;
		SDL_free(surface->userdata);
		SDL_FreeSurface(surface);
	}

	/* Get first frame buf location available*/
	int i, buf_num = -1;
	for( i=0; i<MAX_FRAME_BUFFERS && buf_num < 0; i++)
	{
		if( piUsed[i] == 0)
		{
			buf_num = i;
			piUsed[i]=1;
		}
	}

	/* Create a new one */
	SDL_PixelFormatEnumToMasks(surface_format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
	SDL_GetWindowSize(window, &w, &h);
	surface = SDL_CreateRGBSurface(0, w, h, bpp, Rmask, Gmask, Bmask, Amask);
	if (!surface) {
		return -1;
	}

	/* Create userdata*/
	surface->userdata = SDL_malloc(sizeof(PSL1GHTSurfaceData));
	if(!surface->userdata)
	{
		printf("ERROR\n");
		SDL_FreeSurface(surface);
		SDL_OutOfMemory();
		return -1;
	}
	PSL1GHTSurfaceData * data  = (PSL1GHTSurfaceData*) surface->userdata;
	data->buffer_num = buf_num;

    data->devdata = _this->driverdata;

	printf( "\t\tAllocate RSX memory for pixels\n");
	/* Allocate RSX memory for pixels */
	SDL_free(surface->pixels);
	surface->pixels = rsxMemAlign(16, surface->h * surface->pitch);
	if (!surface->pixels) {
		printf("ERROR\n");
		SDL_free(surface->userdata);
		SDL_FreeSurface(surface);
		SDL_OutOfMemory();
		return -1;
	}

	u32 offset = 0;
	printf( "\t\tPrepare RSX offsets (%16X, %08X) \n", (unsigned int) surface->pixels, (unsigned int) &offset);
	if ( realityAddressToOffset(surface->pixels, &offset) != 0) {
		printf("ERROR\n");
		surface->pixels = 0; // FIXME when unalloc rsx will be available
		SDL_free(surface->userdata);
		SDL_FreeSurface(surface);
		SDL_OutOfMemory();
		return -1;
	}
	printf( "\t\tSetup the display buffers\n");
	// Setup the display buffers
	if ( gcmSetDisplayBuffer(buf_num, offset, surface->pitch, surface->w,surface->h) != 0) {
		printf("ERROR\n");
		surface->pixels = 0; // FIXME when unalloc rsx will be available
		SDL_free(surface->userdata);
		SDL_FreeSurface(surface);
		SDL_OutOfMemory();
		return -1;
	}

	/* Save the info and return! */
	SDL_SetWindowData(window, PSL1GHT_SURFACE, surface);
	*format = surface_format;
	*pixels = surface->pixels;
	*pitch = surface->pitch;
	printf( "\tReset Flip Status\n");
	gcmResetFlipStatus();
	printf( "\tFinished\n");
	flip(data->devdata->_CommandBuffer, buf_num);
	return 0;
}

/*
SDL_Renderer *
SDL_PSL1GHT_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = window->display;
    SDL_DisplayMode *displayMode = &display->current_mode;
    SDL_Renderer *renderer;
    SDL_PSL1GHT_RenderData *data;
    int i, n;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    printf( "SDL_PSL1GHT_CreateRenderer( %08X, %08X)\n", (unsigned int) window, flags);

    if (!SDL_PixelFormatEnumToMasks
        (displayMode->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        printf("ERROR\n");
        SDL_SetError("Unknown display format");
        return NULL;
    }

    n = 2;
    printf("\tCreate the %d screen(s):\n", n);
    for (i = 0; i < n; ++i) {
        printf( "\t\tSDL_CreateRGBSurface( w: %d, h: %d)\n", displayMode->w, displayMode->h);
        data->screens[i] =
            SDL_CreateRGBSurface(0, displayMode->w, displayMode->h, bpp, Rmask, Gmask,
                                 Bmask, Amask);
        if (!data->screens[i]) {
            printf("ERROR\n");
            SDL_PSL1GHT_DestroyRenderer(renderer);
            return NULL;
        }


        printf( "\t\tSDL_SetSurfacePalette()\n");
        //SDL_SetSurfacePalette(data->screens[i], display->palette);
    }
    data->current_screen = 0;

    printf( "\tReset Flip Status\n");
    gcmResetFlipStatus();
    printf( "\tFinished\n");
    flip(data->context, data->current_screen);
    return renderer;
}
*/


int SDL_PSL1GHT_UpdateWindowFramebuffer(_THIS, SDL_Window * window, int numrects, SDL_Rect * rects)
{
	static int frame_number;
	SDL_Surface *surface;

	surface = (SDL_Surface *) SDL_GetWindowData(window, PSL1GHT_SURFACE);
	if (!surface) {
		SDL_SetError("Couldn't find dummy surface for window");
		return -1;
	}

    /* Wait for vsync */
	while(gcmGetFlipStatus() != 0)
		usleep(200);
	gcmResetFlipStatus();

    printf( "\tPage flip\n");
    /* Page flip */
	PSL1GHTSurfaceData * data  = (PSL1GHTSurfaceData*) surface->userdata;
	flip(data->devdata->_CommandBuffer, data->buffer_num);
	return 0;
}

void SDL_PSL1GHT_DestroyWindowFramebuffer(_THIS, SDL_Window * window)
{
    SDL_Surface *surface;

    surface = (SDL_Surface *) SDL_SetWindowData(window, PSL1GHT_SURFACE, NULL);
    if (surface) {
        SDL_FreeSurface(surface);
    }
}

/* vi: set ts=4 sw=4 expandtab: */
