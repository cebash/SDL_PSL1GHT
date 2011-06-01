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

#include "SDL_video.h"
#include "../SDL_sysvideo.h"
#include "../SDL_yuv_sw_c.h"
#include "../SDL_renderer_sw.h"

#include "SDL_PSL1GHTvideo.h"

#include <rsx/rsx.h>
#include <unistd.h>
#include <assert.h>


/* SDL surface based renderer implementation */

static SDL_Renderer *SDL_PSL1GHT_CreateRenderer(SDL_Window * window,
                                              Uint32 flags);
static int SDL_PSL1GHT_RenderDrawPoints(SDL_Renderer * renderer,
                                      const SDL_Point * points, int count);
static int SDL_PSL1GHT_RenderDrawLines(SDL_Renderer * renderer,
                                     const SDL_Point * points, int count);
static int SDL_PSL1GHT_RenderDrawRects(SDL_Renderer * renderer,
                                     const SDL_Rect ** rects, int count);
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
static int SDL_PSL1GHT_RenderWritePixels(SDL_Renderer * renderer,
                                       const SDL_Rect * rect,
                                       Uint32 format,
                                       const void * pixels, int pitch);
static void SDL_PSL1GHT_RenderPresent(SDL_Renderer * renderer);
static void SDL_PSL1GHT_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver SDL_PSL1GHT_RenderDriver = {
    SDL_PSL1GHT_CreateRenderer,
    {
        "psl1ght",
        ( 
//         SDL_RENDERER_SINGLEBUFFER | 
         SDL_RENDERER_PRESENTVSYNC |
         SDL_RENDERER_PRESENTCOPY |
         SDL_RENDERER_PRESENTFLIP2 | 
//         SDL_RENDERER_PRESENTFLIP3 |
         SDL_RENDERER_PRESENTDISCARD
        ),
    }
};

typedef struct
{
    int current_screen;
    SDL_Surface *screens[3];
    gcmContextData *context; // Context to keep track of the RSX buffer.    
} SDL_PSL1GHT_RenderData;

static void flip( gcmContextData *context, int current_screen)
{
    assert(gcmSetFlip(context, current_screen) == 0);
    rsxFlushBuffer(context);
    gcmSetWaitFlip(context); // Prevent the RSX from continuing until the flip has finished.
}

static void waitFlip()
{
    while(gcmGetFlipStatus() != 0)
      usleep(200);
    gcmResetFlipStatus();
}

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

    deprintf (1,  "SDL_PSL1GHT_CreateRenderer( %016X, %08X)\n", (unsigned long long) window, flags);

    if (!SDL_PixelFormatEnumToMasks
        (displayMode->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        deprintf (1, "ERROR\n");
        SDL_SetError("Unknown display format");
        return NULL;
    }

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        deprintf (1, "ERROR\n");
        SDL_OutOfMemory();
        return NULL;
    }

    data = (SDL_PSL1GHT_RenderData *) SDL_malloc(sizeof(*data));
    if (!data) {
        deprintf (1, "ERROR\n");
        SDL_PSL1GHT_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(data);
    
    deprintf (1, "\tMem allocated\n");
    // Get a copy of the command buffer
    data->context = ((SDL_DeviceData*) window->display->device->driverdata)->_CommandBuffer;

    renderer->RenderDrawPoints = SDL_PSL1GHT_RenderDrawPoints;
    renderer->RenderDrawLines = SDL_PSL1GHT_RenderDrawLines;
    renderer->RenderDrawRects = SDL_PSL1GHT_RenderDrawRects;
    renderer->RenderFillRects = SDL_PSL1GHT_RenderFillRects;
    renderer->RenderCopy = SDL_PSL1GHT_RenderCopy;
    renderer->RenderReadPixels = SDL_PSL1GHT_RenderReadPixels;
    renderer->RenderWritePixels = SDL_PSL1GHT_RenderWritePixels;
    renderer->RenderPresent = SDL_PSL1GHT_RenderPresent;
    renderer->DestroyRenderer = SDL_PSL1GHT_DestroyRenderer;
    renderer->info.name = SDL_PSL1GHT_RenderDriver.info.name;
    renderer->info.flags = 0;
    renderer->window = window;
    renderer->driverdata = data;
    deprintf (1,  "\tSetup_SoftwareRenderer()\n");
    Setup_SoftwareRenderer(renderer);

/*
    if (flags & SDL_RENDERER_PRESENTFLIP2) {
        renderer->info.flags |= SDL_RENDERER_PRESENTFLIP2;
        n = 2;
    } else if (flags & SDL_RENDERER_PRESENTFLIP3) {
        renderer->info.flags |= SDL_RENDERER_PRESENTFLIP3;
        n = 3;
    } else {
        renderer->info.flags |= SDL_RENDERER_PRESENTCOPY;
        n = 1;
    }
*/
    n = 2;
    deprintf (1, "\tCreate the %d screen(s):\n", n);
    for (i = 0; i < n; ++i) {
        deprintf (1,  "\t\tSDL_CreateRGBSurface( w: %d, h: %d)\n", displayMode->w, displayMode->h);
        data->screens[i] =
            SDL_CreateRGBSurface(0, displayMode->w, displayMode->h, bpp, Rmask, Gmask,
                                 Bmask, Amask);
        if (!data->screens[i]) {
            deprintf (1, "ERROR\n");
            SDL_PSL1GHT_DestroyRenderer(renderer);
            return NULL;
        }

        deprintf (1,  "\t\tAllocate RSX memory for pixels\n");
        /* Allocate RSX memory for pixels */
        SDL_free(data->screens[i]->pixels);
        data->screens[i]->pixels = rsxMemalign(64, data->screens[i]->h * data->screens[i]->pitch);
        if (!data->screens[i]->pixels) {
            deprintf (1, "ERROR\n");
            SDL_FreeSurface(data->screens[i]);
            SDL_OutOfMemory();
            return NULL;
        }

        u32 offset = 0;
        deprintf (1,  "\t\tPrepare RSX offsets (%16X, %08X) \n", (unsigned int) data->screens[i]->pixels, (unsigned int) &offset);
        if ( rsxAddressToOffset(data->screens[i]->pixels, &offset) != 0) {
            deprintf (1, "ERROR\n");
//            SDL_FreeSurface(data->screens[i]);
            SDL_OutOfMemory();
            return NULL;
        }
        deprintf (1,  "\t\tSetup the display buffers\n");
        // Setup the display buffers
        if ( gcmSetDisplayBuffer(i, offset, data->screens[i]->pitch, data->screens[i]->w,data->screens[i]->h) != 0) {
            deprintf (1, "ERROR\n");
//            SDL_FreeSurface(data->screens[i]);
            SDL_OutOfMemory();
            return NULL;
        }
        deprintf (1,  "\t\tSDL_SetSurfacePalette()\n");
        SDL_SetSurfacePalette(data->screens[i], display->palette);
	/* Center drawable region on screen */
	if (data->screens[i]->w > window->w) {
	    data->screens[i]->pixels =
	      ((Uint8 *)data->screens[i]->pixels) +
	      (data->screens[i]->w - window->w)/2*(bpp/8);
	    data->screens[i]->w = window->w;
	}
	if (data->screens[i]->h > window->h) {
	    data->screens[i]->pixels =
	      ((Uint8 *)data->screens[i]->pixels) +
	      (data->screens[i]->h - window->h)/2*data->screens[i]->pitch;
	    data->screens[i]->h = window->h;
	}
    }
    data->current_screen = 0;

    deprintf (1,  "\tReset Flip Status\n");
    gcmResetFlipStatus();
    deprintf (1,  "\tFinished\n");
    flip(data->context, data->current_screen);
    waitFlip();
    return renderer;
}

static int
SDL_PSL1GHT_RenderDrawPoints(SDL_Renderer * renderer,
                           const SDL_Point * points, int count)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;
    SDL_Surface *target = data->screens[data->current_screen];

    deprintf (1,  "SDL_PSL1GHT_RenderDrawPoints () \n");

    if (renderer->blendMode == SDL_BLENDMODE_NONE ||
        renderer->blendMode == SDL_BLENDMODE_MASK) {
        Uint32 color = SDL_MapRGBA(target->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        return SDL_DrawPoints(target, points, count, color);
    } else {
        return SDL_BlendPoints(target, points, count, renderer->blendMode,
                               renderer->r, renderer->g, renderer->b,
                               renderer->a);
    }
}

static int
SDL_PSL1GHT_RenderDrawLines(SDL_Renderer * renderer,
                          const SDL_Point * points, int count)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;
    SDL_Surface *target = data->screens[data->current_screen];

    deprintf (1,  "SDL_PSL1GHT_RenderDrawLines()\n");
    if (renderer->blendMode == SDL_BLENDMODE_NONE ||
        renderer->blendMode == SDL_BLENDMODE_MASK) {
        Uint32 color = SDL_MapRGBA(target->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        return SDL_DrawLines(target, points, count, color);
    } else {
        return SDL_BlendLines(target, points, count, renderer->blendMode,
                              renderer->r, renderer->g, renderer->b,
                              renderer->a);
    }
}

static int
SDL_PSL1GHT_RenderDrawRects(SDL_Renderer * renderer, const SDL_Rect ** rects,
                          int count)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;
    SDL_Surface *target = data->screens[data->current_screen];

    deprintf (1,  "SDL_PSL1GHT_RenderDrawRects()\n");
    if (renderer->blendMode == SDL_BLENDMODE_NONE ||
        renderer->blendMode == SDL_BLENDMODE_MASK) {
        Uint32 color = SDL_MapRGBA(target->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        deprintf (1, "\tSDL_DrawRects()\n");
        return SDL_DrawRects(target, rects, count, color);
    } else {
        deprintf (1, "\tSDL_BlendRects()\n");
        return SDL_BlendRects(target, rects, count,
                              renderer->blendMode,
                              renderer->r, renderer->g, renderer->b,
                              renderer->a);
    }
}

static int
SDL_PSL1GHT_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects,
                          int count)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;
    SDL_Surface *target = data->screens[data->current_screen];

    deprintf (1,  "SDL_PSL1GHT_RenderFillRects()\n");

    if (renderer->blendMode == SDL_BLENDMODE_NONE ||
        renderer->blendMode == SDL_BLENDMODE_MASK) {
        Uint32 color = SDL_MapRGBA(target->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);
        deprintf (1, "\tSDL_FillRects()\n");

        return SDL_FillRects(target, rects, count, color);
    } else {
        deprintf (1, "\tSDL_BlendFillRects()\n");
        return SDL_BlendFillRects(target, rects, count,
                                  renderer->blendMode,
                                  renderer->r, renderer->g, renderer->b,
                                  renderer->a);
    }
}

static int
SDL_PSL1GHT_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                     const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;

    deprintf (1,  "SDL_PSL1GHT_RenderCopy()\n");

    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        SDL_Surface *target = data->screens[data->current_screen];
        void *pixels =
            (Uint8 *) target->pixels + dstrect->y * target->pitch +
            dstrect->x * target->format->BytesPerPixel;
        deprintf (1, "\tSDL_SW_CopyYUVToRGB()\n");
        return SDL_SW_CopyYUVToRGB((SDL_SW_YUVTexture *) texture->driverdata,
                                   srcrect, display->current_mode.format,
                                   dstrect->w, dstrect->h, pixels,
                                   target->pitch);
    } else {
        Uint8 *src, *dst;
        int row;
        size_t length;
        Uint8 *dstpixels;

        src = (Uint8 *)((SDL_Surface *)texture->driverdata)->pixels;
        dst = (Uint8 *) data->screens[data->current_screen]->pixels +
            dstrect->y * data->screens[data->current_screen]->pitch + dstrect->x
                        * SDL_BYTESPERPIXEL(texture->format);
        length = dstrect->w * SDL_BYTESPERPIXEL(texture->format);
        if (length > data->screens[data->current_screen]->w * SDL_BYTESPERPIXEL(texture->format))
          length = data->screens[data->current_screen]->w * SDL_BYTESPERPIXEL(texture->format);
        for (row = 0; row < dstrect->h && row < data->screens[data->current_screen]->h; ++row) {
            SDL_memcpy(dst, src, length);
            src += ((SDL_Surface *)texture->driverdata)->pitch;
            dst += data->screens[data->current_screen]->pitch;
        }
		return 0;
/*        SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
        SDL_Surface *target = data->screens[data->current_screen];
        SDL_Rect real_srcrect = *srcrect;
        SDL_Rect real_dstrect = *dstrect;
        deprintf (1, "\tSDL_LowerBlit( (%d, %d) -> (%d, %d) (pitch %d))\n", srcrect->h, srcrect->w, dstrect->h, dstrect->w, surface->pitch);
        deprintf (1, "\t\tmemcpy( %08X, %08X, %d)\n", target->pixels, surface->pixels, srcrect->h * srcrect->w * 4);

        memcpy( target->pixels, surface->pixels, srcrect->h * srcrect->w * 4);
        return SDL_LowerBlit(surface, &real_srcrect, target, &real_dstrect);
*/    }
}

static int
SDL_PSL1GHT_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                           Uint32 format, void * pixels, int pitch)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    SDL_Surface *screen = data->screens[data->current_screen];
    Uint32 screen_format = display->current_mode.format;
    Uint8 *screen_pixels = (Uint8 *) screen->pixels +
                            rect->y * screen->pitch +
                            rect->x * screen->format->BytesPerPixel;
    int screen_pitch = screen->pitch;

    deprintf (1,  "SDL_PSL1GHT_RenderReadPixels()\n");

    return SDL_ConvertPixels(rect->w, rect->h,
                             screen_format, screen_pixels, screen_pitch,
                             format, pixels, pitch);
}

static int
SDL_PSL1GHT_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                            Uint32 format, const void * pixels, int pitch)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = window->display;
    SDL_Surface *screen = data->screens[data->current_screen];
    Uint32 screen_format = display->current_mode.format;
    Uint8 *screen_pixels = (Uint8 *) screen->pixels +
                            rect->y * screen->pitch +
                            rect->x * screen->format->BytesPerPixel;
    int screen_pitch = screen->pitch;

    deprintf (1,  "SDL_PSL1GHT_RenderWritePixels()\n");
    return SDL_ConvertPixels(rect->w, rect->h,
                             format, pixels, pitch,
                             screen_format, screen_pixels, screen_pitch);
}

static void
SDL_PSL1GHT_RenderPresent(SDL_Renderer * renderer)
{
    static int frame_number;
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;

    deprintf (1,  "SDL_PSL1GHT_RenderPresent()\n");

    deprintf (1,  "\tRendering to screen %d\n", data->current_screen);

    deprintf (1,  "\tPage flip\n");
    /* Page flip */
    flip(data->context, data->current_screen);

    deprintf (1,  "\tWait for vsync\n");
    /* Wait for vsync */
    //if (renderer->info.flags & SDL_RENDERER_PRESENTVSYNC) {
        waitFlip();
    //}

    deprintf (1,  "\tUpdate the flipping chain, if any\n");
    /* Update the flipping chain, if any */
    //if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP2) {
        data->current_screen = (data->current_screen + 1) % 2;
    /*} else if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP3) {
        data->current_screen = (data->current_screen + 1) % 3;
    }*/
    deprintf (1,  "\tUpdated the screen to %d\n", data->current_screen);
}

static void
SDL_PSL1GHT_DestroyRenderer(SDL_Renderer * renderer)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;
    int i;

    deprintf (1, "SDL_PSL1GHT_DestroyRenderer()\n");

    if (data) {
        for (i = 0; i < SDL_arraysize(data->screens); ++i) {
            if (data->screens[i]) {
             //   SDL_FreeSurface(data->screens[i]);
            }
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

/* vi: set ts=4 sw=4 expandtab: */
