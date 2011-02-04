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

#include <sysutil/events.h>

static void eventHandle(u64 status, u64 param, void * userdata) {
    _THIS = userdata;
    if(status == EVENT_REQUEST_EXITAPP){
	printf("Quit game requested\n");
	SDL_SendQuit();
    }else if(status == EVENT_MENU_OPEN){
	//xmb opened, should prob pause game or something :P
    }else if(status == EVENT_MENU_CLOSE){
	//xmb closed, and then resume
    }else if(status == EVENT_DRAWING_BEGIN){
    }else if(status == EVENT_DRAWING_END){
    }else{
	printf("Unhandled event: %08llX\n", (unsigned long long int)status);
    }
}

void
PSL1GHT_PumpEvents(_THIS)
{
    sysCheckCallback();
}

void
PSL1GHT_InitSysEvent(_THIS)
{
    sysRegisterCallback(EVENT_SLOT0, eventHandle, _this);
}

void
PSL1GHT_QuitSysEvent(_THIS)
{
    sysUnregisterCallback(EVENT_SLOT0);
}

/* vi: set ts=4 sw=4 expandtab: */
