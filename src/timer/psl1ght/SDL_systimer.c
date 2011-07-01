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

#ifdef SDL_TIMER_PSL1GHT
#include <sys/time.h>
#include <sys/unistd.h>

#include "SDL_thread.h"
#include "SDL_timer.h"
#include "../SDL_timer_c.h"

static struct timeval start;

void
SDL_StartTicks(void)
{
    /* Set first ticks value */
    gettimeofday(&start, NULL);
}

Uint32
SDL_GetTicks(void)
{
    Uint32 ticks;
    struct timeval now;
    gettimeofday(&now, NULL);
    ticks =
        (now.tv_sec - start.tv_sec) * 1000 + (now.tv_usec -
                                              start.tv_usec) / 1000;
    return (ticks);
}

void
SDL_Delay(Uint32 ms)
{
    usleep(ms * 1000);
}

Uint64
SDL_GetPerformanceCounter(void)
{
    Uint64 ticks;
    struct timeval now;

    gettimeofday(&now, NULL);
    ticks = now.tv_sec;
    ticks *= 1000000;
    ticks += now.tv_usec;
    return (ticks);
}

Uint64
SDL_GetPerformanceFrequency(void)
{
    return 1000000;
}

#endif /* SDL_TIMER_PSL1GHT */
/* vi: set ts=4 sw=4 expandtab: */
