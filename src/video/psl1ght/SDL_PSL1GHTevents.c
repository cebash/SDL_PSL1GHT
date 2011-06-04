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

/* Being a null driver, there's no event stream. We just define stubs for
   most of the API. */

#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"

#include "SDL_PSL1GHTvideo.h"
#include "SDL_PSL1GHTevents_c.h"

#include <sysutil/sysutil.h>

static void eventHandle(u64 status, u64 param, void * userdata) {
    _THIS = userdata;
    SDL_Window *window = NULL;

    // There should only be one window
    if (_this->num_displays == 1) {
        SDL_VideoDisplay *display = &_this->displays[0];
        if (display->windows != NULL) {
            window = display->windows;
        }
    }

    // Process event
    if (status == SYSUTIL_EXIT_GAME) {
	    deprintf(1, "Quit game requested\n");
	    SDL_SendQuit();
    } else if(status == SYSUTIL_MENU_OPEN) {
	    // XMB opened
	    if (window) {
	        SDL_SendWindowEvent(window, SDL_WINDOWEVENT_LEAVE, 0, 0);
	    }
    } else if(status == SYSUTIL_MENU_CLOSE) {
		// XMB closed
	    if (window) {
	        SDL_SendWindowEvent(window, SDL_WINDOWEVENT_ENTER, 0, 0);
	    }
    } else if(status == SYSUTIL_DRAW_BEGIN) {
    } else if(status == SYSUTIL_DRAW_END) {
    } else {
	    deprintf(1, "Unhandled event: %08llX\n", (unsigned long long int)status);
    }
}

void
PSL1GHT_PumpEvents(_THIS)
{
    sysUtilCheckCallback();
}

void
PSL1GHT_InitSysEvent(_THIS)
{
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, eventHandle, _this);
}

void
PSL1GHT_QuitSysEvent(_THIS)
{
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
}

/* vi: set ts=4 sw=4 expandtab: */
