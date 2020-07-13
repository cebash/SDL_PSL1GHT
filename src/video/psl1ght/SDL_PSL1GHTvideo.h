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

#ifndef _SDL_PSL1GHTvideo_h
#define _SDL_PSL1GHTvideo_h

#include "../SDL_sysvideo.h"

#include <rsx/rsx.h>
#include <sysutil/video.h>

/* Debugging
 * 0: No debug messages
 * 1: Video debug messages
 * 2: SPE debug messages
 * 3: Memory adresses
 */
//#define VIDEO_DEBUG_LEVEL 0

#ifdef VIDEO_DEBUG_LEVEL
#define deprintf( level, fmt, args... ) \
    do \
{ \
    if ( (unsigned)(level) <= VIDEO_DEBUG_LEVEL ) \
    { \
        fprintf( stdout, fmt, ##args ); \
        fflush( stdout ); \
    } \
} while ( 0 )
#else
#define deprintf( level, fmt, args... )
#endif

/* Private RSX data */
typedef struct SDL_DeviceData
{
    // Context to keep track of the RSX buffer.
    gcmContextData *_CommandBuffer;

    bool _keyboardConnected;
    Uint32 _keyboardMapping;

    bool _mouseConnected;
    Uint8 _mouseButtons;
} SDL_DeviceData;

typedef struct SDL_DisplayModeData
{
    videoConfiguration vconfig;
} PSL1GHT_DisplayModeData;
#endif /* _SDL_PSL1GHTvideo_h */

/* vi: set ts=4 sw=4 expandtab: */
