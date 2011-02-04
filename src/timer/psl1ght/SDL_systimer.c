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

/* Data to handle a single periodic alarm */
static int timer_alive = 0;
static SDL_Thread *timer = NULL;

static int
RunTimer(void *unused)
{
    while (timer_alive) {
        if (SDL_timer_running) {
            SDL_ThreadedTimerCheck();
        }
        SDL_Delay(10);
    }
    return (0);
}

/* This is only called if the event thread is not running */
int
SDL_SYS_TimerInit(void)
{
    timer_alive = 1;
    timer = SDL_CreateThread(RunTimer, NULL);
    if (timer == NULL)
        return (-1);
    return (SDL_SetTimerThreaded(1));
}

void
SDL_SYS_TimerQuit(void)
{
    timer_alive = 0;
    if (timer) {
        SDL_WaitThread(timer, NULL);
        timer = NULL;
    }
}

int
SDL_SYS_StartTimer(void)
{
    SDL_SetError("Internal logic error: psl1ght uses threaded timer");
    return (-1);
}

void
SDL_SYS_StopTimer(void)
{
    return;
}

#endif /* SDL_TIMER_PSL1GHT */
/* vi: set ts=4 sw=4 expandtab: */
