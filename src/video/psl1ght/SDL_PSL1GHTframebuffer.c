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

#define PSL1GHT_SURFACE   "_SDL_PSL1GHTSurface"

/* SDL surface based renderer implementation */

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
        SDL_FreeSurface(surface);
    }

    /* Create a new one */
    SDL_PixelFormatEnumToMasks(surface_format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
    SDL_GetWindowSize(window, &w, &h);
    surface = SDL_CreateRGBSurface(0, w, h, bpp, Rmask, Gmask, Bmask, Amask);
    if (!surface) {
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
    flip(data->context, data->current_screen);
    return 0;
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

        printf( "\t\tAllocate RSX memory for pixels\n");
        /* Allocate RSX memory for pixels */
        SDL_free(data->screens[i]->pixels);
        data->screens[i]->pixels = rsxMemAlign(16, data->screens[i]->h * data->screens[i]->pitch);
        if (!data->screens[i]->pixels) {
            printf("ERROR\n");
            SDL_FreeSurface(data->screens[i]);
            SDL_OutOfMemory();
            return NULL;
        }

        u32 offset = 0;
        printf( "\t\tPrepare RSX offsets (%16X, %08X) \n", (unsigned int) data->screens[i]->pixels, (unsigned int) &offset);
        if ( realityAddressToOffset(data->screens[i]->pixels, &offset) != 0) {
            printf("ERROR\n");
//            SDL_FreeSurface(data->screens[i]);
            SDL_OutOfMemory();
            return NULL;
        }
        printf( "\t\tSetup the display buffers\n");
        // Setup the display buffers
        if ( gcmSetDisplayBuffer(i, offset, data->screens[i]->pitch, data->screens[i]->w,data->screens[i]->h) != 0) {
            printf("ERROR\n");
//            SDL_FreeSurface(data->screens[i]);
            SDL_OutOfMemory();
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

static int
SDL_PSL1GHT_RenderDrawPoints(SDL_Renderer * renderer,
                           const SDL_Point * points, int count)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;
    SDL_Surface *target = data->screens[data->current_screen];

    printf( "SDL_PSL1GHT_RenderDrawPoints () \n");

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
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

    printf( "SDL_PSL1GHT_RenderDrawLines()\n");
    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
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
SDL_PSL1GHT_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects,
                          int count)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;
    SDL_Surface *target = data->screens[data->current_screen];

    printf( "SDL_PSL1GHT_RenderFillRects()\n");

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(target->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);
        printf("\tSDL_FillRects()\n");

        return SDL_FillRects(target, rects, count, color);
    } else {
        printf("\tSDL_BlendFillRects()\n");
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

    printf( "SDL_PSL1GHT_RenderCopy()\n");

    if (SDL_ISPIXELFORMAT_FOURCC(texture->format)) {
        SDL_Surface *target = data->screens[data->current_screen];
        void *pixels =
            (Uint8 *) target->pixels + dstrect->y * target->pitch +
            dstrect->x * target->format->BytesPerPixel;
        printf("\tSDL_SW_CopyYUVToRGB()\n");
        return SDL_SW_CopyYUVToRGB((SDL_SW_YUVTexture *) texture->driverdata,
                                   srcrect, display->current_mode.format,
                                   dstrect->w, dstrect->h, pixels,
                                   target->pitch);
    } else {
        Uint8 *src, *dst;
        int row;
        size_t length;

        src = (Uint8 *)((SDL_Surface *)texture->driverdata)->pixels;
        dst = (Uint8 *) data->screens[data->current_screen]->pixels + dstrect->y * data->screens[data->current_screen]->pitch + dstrect->x
                        * SDL_BYTESPERPIXEL(texture->format);
        length = dstrect->w * SDL_BYTESPERPIXEL(texture->format);
        for (row = 0; row < dstrect->h; ++row) {
            SDL_memcpy(dst, src, length);
            src += texture->w * SDL_BYTESPERPIXEL(texture->format);
            dst += data->screens[data->current_screen]->pitch;
        }
		return 0;
/*        SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
        SDL_Surface *target = data->screens[data->current_screen];
        SDL_Rect real_srcrect = *srcrect;
        SDL_Rect real_dstrect = *dstrect;
        printf("\tSDL_LowerBlit( (%d, %d) -> (%d, %d) (pitch %d))\n", srcrect->h, srcrect->w, dstrect->h, dstrect->w, surface->pitch);
        printf("\t\tmemcpy( %08X, %08X, %d)\n", target->pixels, surface->pixels, srcrect->h * srcrect->w * 4);

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

    printf( "SDL_PSL1GHT_RenderReadPixels()\n");

    return SDL_ConvertPixels(rect->w, rect->h,
                             screen_format, screen_pixels, screen_pitch,
                             format, pixels, pitch);
}

static void
SDL_PSL1GHT_RenderPresent(SDL_Renderer * renderer)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;

    printf( "SDL_PSL1GHT_RenderPresent()\n");

    printf( "\tRendering to screen %d\n", data->current_screen);

    printf( "\tWait for vsync\n");
    /* Wait for vsync */
    //if (renderer->info.flags & SDL_RENDERER_PRESENTVSYNC) {
        while(gcmGetFlipStatus() != 0)
            usleep(200);
        gcmResetFlipStatus();
    //}

    printf( "\tPage flip\n");
    /* Page flip */
    flip(data->context, data->current_screen);

    printf( "\tUpdate the flipping chain, if any\n");
    /* Update the flipping chain, if any */
    //if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP2) {
        data->current_screen = (data->current_screen + 1) % 2;
    /*} else if (renderer->info.flags & SDL_RENDERER_PRESENTFLIP3) {
        data->current_screen = (data->current_screen + 1) % 3;
    }*/
    printf( "\tUpdated the screen to %d\n", data->current_screen);
}

static void
SDL_PSL1GHT_DestroyRenderer(SDL_Renderer * renderer)
{
    SDL_PSL1GHT_RenderData *data =
        (SDL_PSL1GHT_RenderData *) renderer->driverdata;
    int i;

    printf( "SDL_PSL1GHT_DestroyRenderer()\n");

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
