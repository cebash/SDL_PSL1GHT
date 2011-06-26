/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "SDL_config.h"

#if SDL_VIDEO_RENDER_PSL1GHT

#include "../SDL_sysrender.h"
#include "../../video/SDL_sysvideo.h"
#include "../../video/psl1ght/SDL_PSL1GHTvideo.h"

#include "../software/SDL_draw.h"
#include "../software/SDL_blendfillrect.h"
#include "../software/SDL_blendline.h"
#include "../software/SDL_blendpoint.h"
#include "../software/SDL_drawline.h"
#include "../software/SDL_drawpoint.h"

#include <rsx/rsx.h>
#include <unistd.h>
#include <assert.h>

/* SDL surface based renderer implementation */

static SDL_Renderer *PSL1GHT_CreateRenderer(SDL_Window * window, Uint32 flags);
static int PSL1GHT_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int PSL1GHT_SetTextureColorMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int PSL1GHT_SetTextureAlphaMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int PSL1GHT_SetTextureBlendMode(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int PSL1GHT_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * rect, const void *pixels,
                            int pitch);
static int PSL1GHT_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * rect, void **pixels, int *pitch);
static void PSL1GHT_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int PSL1GHT_UpdateViewport(SDL_Renderer * renderer);
static int PSL1GHT_RenderClear(SDL_Renderer * renderer);
static int PSL1GHT_RenderDrawPoints(SDL_Renderer * renderer,
                               const SDL_Point * points, int count);
static int PSL1GHT_RenderDrawLines(SDL_Renderer * renderer,
                              const SDL_Point * points, int count);
static int PSL1GHT_RenderFillRects(SDL_Renderer * renderer,
                              const SDL_Rect * rects, int count);
static int PSL1GHT_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                         const SDL_Rect * srcrect, const SDL_Rect * dstrect);
static int PSL1GHT_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                               Uint32 format, void * pixels, int pitch);
static void PSL1GHT_RenderPresent(SDL_Renderer * renderer);
static void PSL1GHT_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static void PSL1GHT_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver PSL1GHT_RenderDriver = {
    PSL1GHT_CreateRenderer,
    {
     "PSL1GHT",
     SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC,
     1,
     { SDL_PIXELFORMAT_ARGB8888 },
     0,
     0}
};

typedef struct
{
    bool first_fb; // Is this the first flip ?
    int current_screen;
    SDL_Surface *screens[3];
    void *textures[3];
    gcmContextData *context; // Context to keep track of the RSX buffer.
} PSL1GHT_RenderData;

static void waitFlip()
{
    while (gcmGetFlipStatus() != 0)
      usleep(200);
    gcmResetFlipStatus();
}

static SDL_Surface *
PSL1GHT_ActivateRenderer(SDL_Renderer * renderer)
{
    PSL1GHT_RenderData *data = (PSL1GHT_RenderData *) renderer->driverdata;

    return data->screens[data->current_screen];
}

SDL_Renderer *
PSL1GHT_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_Renderer *renderer;
    PSL1GHT_RenderData *data;
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    SDL_DisplayMode *displayMode = &display->current_mode;
    int i, n;
    int bpp;
    int pitch;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!SDL_PixelFormatEnumToMasks(displayMode->format, &bpp,
                                    &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown display format");
        return NULL;
    }

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (PSL1GHT_RenderData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        PSL1GHT_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    
    SDL_zerop(data);
    
    deprintf (1, "\tMem allocated\n");
    
    // Get a copy of the command buffer
    data->context = ((SDL_DeviceData*) display->device->driverdata)->_CommandBuffer;
    data->current_screen = 0;
    data->first_fb = true;
    
    pitch = displayMode->w * SDL_BYTESPERPIXEL(displayMode->format);
    
    n = 2;
    deprintf (1, "\tCreate the %d screen(s):\n", n);
    for (i = 0; i < n; ++i) {
        deprintf (1,  "\t\tAllocate RSX memory for pixels\n");
        /* Allocate RSX memory for pixels */
        data->textures[i] = rsxMemalign(64, displayMode->h * pitch);
        if (!data->textures[i]) {
            deprintf (1, "ERROR\n");
            PSL1GHT_DestroyRenderer(renderer);
            SDL_OutOfMemory();
            return NULL;
        }

        memset(data->textures[i], 0, displayMode->h * pitch);

        deprintf (1,  "\t\tSDL_CreateRGBSurfaceFrom( w: %d, h: %d)\n", displayMode->w, displayMode->h);
        data->screens[i] =
            SDL_CreateRGBSurfaceFrom(data->textures[i], displayMode->w, displayMode->h, bpp, pitch, Rmask, Gmask,
                                 Bmask, Amask);
        if (!data->screens[i]) {
            deprintf (1, "ERROR\n");
            PSL1GHT_DestroyRenderer(renderer);
            return NULL;
        }

        u32 offset = 0;
        deprintf (1,  "\t\tPrepare RSX offsets (%16X, %08X) \n", (unsigned int) data->screens[i]->pixels, (unsigned int) &offset);
        if (rsxAddressToOffset(data->screens[i]->pixels, &offset) != 0) {
            deprintf (1, "ERROR\n");
            PSL1GHT_DestroyRenderer(renderer);
            SDL_OutOfMemory();
            return NULL;
        }
        
        deprintf (1,  "\t\tSetup the display buffers\n");
        // Setup the display buffers
        if (gcmSetDisplayBuffer(i, offset, data->screens[i]->pitch, data->screens[i]->w, data->screens[i]->h) != 0) {
            deprintf (1, "ERROR\n");
            PSL1GHT_DestroyRenderer(renderer);
            SDL_OutOfMemory();
            return NULL;
        }
    }

    deprintf (1,  "\tFinished\n");

    renderer->CreateTexture = PSL1GHT_CreateTexture;
    renderer->SetTextureColorMod = PSL1GHT_SetTextureColorMod;
    renderer->SetTextureAlphaMod = PSL1GHT_SetTextureAlphaMod;
    renderer->SetTextureBlendMode = PSL1GHT_SetTextureBlendMode;
    renderer->UpdateTexture = PSL1GHT_UpdateTexture;
    renderer->LockTexture = PSL1GHT_LockTexture;
    renderer->UnlockTexture = PSL1GHT_UnlockTexture;
    renderer->UpdateViewport = PSL1GHT_UpdateViewport;
    renderer->DestroyTexture = PSL1GHT_DestroyTexture;
    renderer->RenderClear = PSL1GHT_RenderClear;
    renderer->RenderDrawPoints = PSL1GHT_RenderDrawPoints;
    renderer->RenderDrawLines = PSL1GHT_RenderDrawLines;
    renderer->RenderFillRects = PSL1GHT_RenderFillRects;
    renderer->RenderCopy = PSL1GHT_RenderCopy;
    renderer->RenderReadPixels = PSL1GHT_RenderReadPixels;
    renderer->RenderPresent = PSL1GHT_RenderPresent;
    renderer->DestroyRenderer = PSL1GHT_DestroyRenderer;
    renderer->info = PSL1GHT_RenderDriver.info;
    renderer->driverdata = data;

    PSL1GHT_UpdateViewport(renderer);
    PSL1GHT_ActivateRenderer(renderer);
    
    return renderer;
}

static int
PSL1GHT_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    int bpp;
    int pitch;
    void *pixels;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!SDL_PixelFormatEnumToMasks
        (texture->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown texture format");
        return -1;
    }

    // Allocate GFX memory for textures
    pitch = texture->w * SDL_BYTESPERPIXEL(texture->format);
    pixels = rsxMemalign(64, texture->h * pitch);

    texture->driverdata =
        SDL_CreateRGBSurfaceFrom(pixels, texture->w, texture->h, bpp, pitch,
                            Rmask, Gmask, Bmask, Amask);

    SDL_SetSurfaceColorMod(texture->driverdata, texture->r, texture->g,
                           texture->b);
    SDL_SetSurfaceAlphaMod(texture->driverdata, texture->a);
    SDL_SetSurfaceBlendMode(texture->driverdata, texture->blendMode);

    if (!texture->driverdata) {
        return -1;
    }
    return 0;
}

static int
PSL1GHT_SetTextureColorMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceColorMod(surface, texture->r, texture->g,
                                  texture->b);
}

static int
PSL1GHT_SetTextureAlphaMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceAlphaMod(surface, texture->a);
}

static int
PSL1GHT_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceBlendMode(surface, texture->blendMode);
}

static int
PSL1GHT_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                 const SDL_Rect * rect, const void *pixels, int pitch)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    Uint8 *src, *dst;
    int row;
    size_t length;

    src = (Uint8 *) pixels;
    dst = (Uint8 *) surface->pixels +
                        rect->y * surface->pitch +
                        rect->x * surface->format->BytesPerPixel;
    length = rect->w * surface->format->BytesPerPixel;
    for (row = 0; row < rect->h; ++row) {
        SDL_memcpy(dst, src, length);
        src += pitch;
        dst += surface->pitch;
    }
    return 0;
}

static int
PSL1GHT_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * rect, void **pixels, int *pitch)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;

    *pixels =
        (void *) ((Uint8 *) surface->pixels + rect->y * surface->pitch +
                  rect->x * surface->format->BytesPerPixel);
    *pitch = surface->pitch;
    return 0;
}

static void
PSL1GHT_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
}

static int
PSL1GHT_UpdateViewport(SDL_Renderer * renderer)
{
    PSL1GHT_RenderData *data = (PSL1GHT_RenderData *) renderer->driverdata;
    SDL_Surface *surface = data->screens[0];

    if (!renderer->viewport.w && !renderer->viewport.h) {
        /* There may be no window, so update the viewport directly */
        renderer->viewport.w = surface->w;
        renderer->viewport.h = surface->h;
    }
    
    SDL_SetClipRect(data->screens[0], &renderer->viewport);
    SDL_SetClipRect(data->screens[1], &renderer->viewport);
    return 0;
}

static int
PSL1GHT_RenderClear(SDL_Renderer * renderer)
{
    SDL_Surface *surface = PSL1GHT_ActivateRenderer(renderer);
    Uint32 color;
    SDL_Rect clip_rect;

    if (!surface) {
        return -1;
    }

    color = SDL_MapRGBA(surface->format,
                        renderer->r, renderer->g, renderer->b, renderer->a);

    /* By definition the clear ignores the clip rect */
    clip_rect = surface->clip_rect;
    SDL_SetClipRect(surface, NULL);
    SDL_FillRect(surface, NULL, color);
    SDL_SetClipRect(surface, &clip_rect);
    return 0;
}

static int
PSL1GHT_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points,
                    int count)
{
    SDL_Surface *surface = PSL1GHT_ActivateRenderer(renderer);
    SDL_Point *temp = NULL;
    int status;

    if (!surface) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        int i;
        int x = renderer->viewport.x;
        int y = renderer->viewport.y;

        temp = SDL_stack_alloc(SDL_Point, count);
        for (i = 0; i < count; ++i) {
            temp[i].x = x + points[i].x;
            temp[i].y = y + points[i].x;
        }
        points = temp;
    }

    /* Draw the points! */
    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(surface->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        status = SDL_DrawPoints(surface, points, count, color);
    } else {
        status = SDL_BlendPoints(surface, points, count,
                                renderer->blendMode,
                                renderer->r, renderer->g, renderer->b,
                                renderer->a);
    }

    if (temp) {
        SDL_stack_free(temp);
    }
    return status;
}

static int
PSL1GHT_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points,
                   int count)
{
    SDL_Surface *surface = PSL1GHT_ActivateRenderer(renderer);
    SDL_Point *temp = NULL;
    int status;

    if (!surface) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        int i;
        int x = renderer->viewport.x;
        int y = renderer->viewport.y;

        temp = SDL_stack_alloc(SDL_Point, count);
        for (i = 0; i < count; ++i) {
            temp[i].x = x + points[i].x;
            temp[i].y = y + points[i].y;
        }
        points = temp;
    }

    /* Draw the lines! */
    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(surface->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        status = SDL_DrawLines(surface, points, count, color);
    } else {
        status = SDL_BlendLines(surface, points, count,
                                renderer->blendMode,
                                renderer->r, renderer->g, renderer->b,
                                renderer->a);
    }

    if (temp) {
        SDL_stack_free(temp);
    }
    return status;
}

static int
PSL1GHT_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect * rects, int count)
{
    SDL_Surface *surface = PSL1GHT_ActivateRenderer(renderer);
    SDL_Rect *temp = NULL;
    int status;

    if (!surface) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        int i;
        int x = renderer->viewport.x;
        int y = renderer->viewport.y;

        temp = SDL_stack_alloc(SDL_Rect, count);
        for (i = 0; i < count; ++i) {
            temp[i].x = x + rects[i].x;
            temp[i].y = y + rects[i].y;
            temp[i].w = rects[i].w;
            temp[i].h = rects[i].h;
        }
        rects = temp;
    }

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(surface->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);
        status = SDL_FillRects(surface, rects, count, color);
    } else {
        status = SDL_BlendFillRects(surface, rects, count,
                                    renderer->blendMode,
                                    renderer->r, renderer->g, renderer->b,
                                    renderer->a);
    }

    if (temp) {
        SDL_stack_free(temp);
    }
    return status;
}

static int
PSL1GHT_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
              const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    PSL1GHT_RenderData *data = (PSL1GHT_RenderData *) renderer->driverdata;
    SDL_Surface *dst = PSL1GHT_ActivateRenderer(renderer);
    SDL_Surface *src = (SDL_Surface *) texture->driverdata;
    SDL_Rect final_rect = *dstrect;
    u32 src_offset, dst_offset;

    if (!dst) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        final_rect.x += renderer->viewport.x;
        final_rect.y += renderer->viewport.y;
    }

    rsxAddressToOffset(dst->pixels, &dst_offset);
    rsxAddressToOffset(src->pixels, &src_offset);

    gcmTransferScale scale;
    scale.conversion = GCM_TRANSFER_CONVERSION_TRUNCATE;
    scale.format = GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8;
    scale.operation = GCM_TRANSFER_OPERATION_SRCCOPY;
    scale.clipX = final_rect.x;
    scale.clipY = final_rect.y;
    scale.clipW = final_rect.w;
    scale.clipH = final_rect.h;
    scale.outX = final_rect.x;
    scale.outY = final_rect.y;
    scale.outW = final_rect.w;
    scale.outH = final_rect.h;
    scale.ratioX = (srcrect->w << 20) / final_rect.w;
    scale.ratioY = (srcrect->h << 20) / final_rect.h;
    scale.inX = srcrect->x;
    scale.inY = srcrect->y;
    scale.inW = srcrect->w;
    scale.inH = srcrect->h;
    scale.offset = src_offset;
    scale.pitch = src->pitch;
    scale.origin = GCM_TRANSFER_ORIGIN_CORNER;
    scale.interp = GCM_TRANSFER_INTERPOLATOR_NEAREST;

    gcmTransferSurface surface;
    surface.format = GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8;
    surface.pitch = dst->pitch;
    surface.offset = dst_offset;

    // Hardware accelerated blit with scaling
    rsxSetTransferScaleMode(data->context, GCM_TRANSFER_LOCAL_TO_LOCAL, GCM_TRANSFER_SURFACE);
    rsxSetTransferScaleSurface(data->context, &scale, &surface);

    // TODO: Blending / clipping

    return 0;
}

static int
PSL1GHT_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                    Uint32 format, void * pixels, int pitch)
{
    SDL_Surface *surface = PSL1GHT_ActivateRenderer(renderer);
    Uint32 src_format;
    void *src_pixels;
    SDL_Rect final_rect;

    if (!surface) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        final_rect.x = renderer->viewport.x + rect->x;
        final_rect.y = renderer->viewport.y + rect->y;
        final_rect.w = rect->w;
        final_rect.h = rect->h;
        rect = &final_rect;
    }

    if (rect->x < 0 || rect->x+rect->w > surface->w ||
        rect->y < 0 || rect->y+rect->h > surface->h) {
        SDL_SetError("Tried to read outside of surface bounds");
        return -1;
    }

    src_format = surface->format->format;
    src_pixels = (void*)((Uint8 *) surface->pixels +
                    rect->y * surface->pitch +
                    rect->x * surface->format->BytesPerPixel);

    return SDL_ConvertPixels(rect->w, rect->h,
                             src_format, src_pixels, surface->pitch,
                             format, pixels, pitch);
}

static void
PSL1GHT_RenderPresent(SDL_Renderer * renderer)
{
    PSL1GHT_RenderData *data = (PSL1GHT_RenderData *) renderer->driverdata;

    if (!data->first_fb)
        waitFlip();
    else
        gcmResetFlipStatus();

    gcmSetFlip(data->context, data->current_screen);
    rsxFlushBuffer(data->context);

    gcmSetWaitFlip(data->context);

    data->first_fb = false;

    // Update the flipping chain, if any
    data->current_screen = (data->current_screen + 1) % 2;
}

static void
PSL1GHT_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;

    // TODO: Wait for the DMA transfer to complete
    rsxFree(surface->pixels);
    SDL_FreeSurface(surface);
}

static void
PSL1GHT_DestroyRenderer(SDL_Renderer * renderer)
{
    PSL1GHT_RenderData *data = (PSL1GHT_RenderData *) renderer->driverdata;
    int i;

    deprintf (1, "SDL_PSL1GHT_DestroyRenderer()\n");

    if (data) {
        for (i = 0; i < SDL_arraysize(data->screens); ++i) {
            if (data->screens[i]) {
               SDL_FreeSurface(data->screens[i]);
            }
            if (data->textures[i]) {
                rsxFree(data->textures[i]);
            }
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* SDL_VIDEO_RENDER_PSL1GHT */

/* vi: set ts=4 sw=4 expandtab: */
