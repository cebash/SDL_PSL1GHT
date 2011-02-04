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

#ifdef SDL_JOYSTICK_PSL1GHT

/* This is the system specific header for the SDL joystick API */

#include "SDL_events.h"
#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#include <io/pad.h>

#define pdprintf(x) printf(x)

#define NAMESIZE 10

typedef struct SDL_PSL1GHT_JoyData
{
	char name[NAMESIZE];
} SDL_PSL1GHT_JoyData;

struct joystick_hwdata
{
	PadData old_pad_data;
};

static SDL_PSL1GHT_JoyData joy_data[MAX_PADS];



/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int
SDL_SYS_JoystickInit(void)
{
	int iReturn = 0;
    SDL_numjoysticks = MAX_PADS;
	PadInfo padinfo;

	pdprintf("SDL_SYS_JoystickInit\n");

	SDL_zero( joy_data);

	if( iReturn == 0)
	{
		iReturn =  ioPadInit( MAX_PADS) ;
		pdprintf("\tPad initialized\n");
		if( iReturn != 0)
		{
			SDL_SetError("SDL_SYS_JoystickInit() : Couldn't initialize PS3 pads");
		}
	}


	if( iReturn == 0)
	{
		iReturn = ioPadGetInfo(&padinfo);
		pdprintf("\tGot info\n");
		if( iReturn != 0)
		{
			SDL_SetError("SDL_SYS_JoystickInit() : Couldn't get PS3 pads information ");
		}
	}

	if( iReturn == 0)
	{
		unsigned int i;
		SDL_numjoysticks = padinfo.connected;

		for(i = 0; i < padinfo.connected; i++)
		{
			if( padinfo.status[i])
			{
				sprintf( joy_data[i].name, "PAD%02X", i);
			}
		} 
	}
	return SDL_numjoysticks;	
}

/* Function to get the device-dependent name of a joystick */
const char *
SDL_SYS_JoystickName(int index)
{
	char * name = NULL;
	if( index <  SDL_numjoysticks)
		name = joy_data[index].name;
	else
		SDL_SetError("No joystick available with that index");
    return name;
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int
SDL_SYS_JoystickOpen(SDL_Joystick * joystick)
{
    if (!(joystick->hwdata = SDL_malloc(sizeof(struct joystick_hwdata))))
        return -1;

	joystick->naxes = 4;
	joystick->nhats = 0;
	joystick->nballs = 0;
	joystick->nbuttons = 16;

    return 0;
}

#define CheckPSL1GHTAxis( btn, bnum) \
	if( new_pad_data.btn != joystick->hwdata->old_pad_data.btn) {\
		SDL_PrivateJoystickAxis( joystick, (bnum), ((new_pad_data.btn-0x80)<<8)|new_pad_data.btn); \
	} \
	joystick->hwdata->old_pad_data.btn = new_pad_data.btn;

#define CheckPSL1GHTButton( btn, bnum) \
	if( new_pad_data.btn != joystick->hwdata->old_pad_data.btn) {\
		if( new_pad_data.btn == 0) \
		SDL_PrivateJoystickButton( joystick, (bnum), SDL_RELEASED); \
		else \
		SDL_PrivateJoystickButton( joystick, (bnum), SDL_PRESSED); \
	} \
	joystick->hwdata->old_pad_data.btn = new_pad_data.btn;


/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void
SDL_SYS_JoystickUpdate(SDL_Joystick * joystick)
{
	PadData new_pad_data;
	if( ioPadGetData(joystick->index, &new_pad_data) == 0)
	{
		// Update axes
		CheckPSL1GHTAxis( ANA_L_H, 0);
		CheckPSL1GHTAxis( ANA_L_V, 1);
		CheckPSL1GHTAxis( ANA_R_H, 2);
		CheckPSL1GHTAxis( ANA_R_V, 3);
		
		// Update buttons
		CheckPSL1GHTButton( BTN_LEFT, 0);
		CheckPSL1GHTButton( BTN_DOWN, 1);
		CheckPSL1GHTButton( BTN_RIGHT, 2);
		CheckPSL1GHTButton( BTN_UP, 3);

		CheckPSL1GHTButton( BTN_START, 4);
		CheckPSL1GHTButton( BTN_R3, 5);
		CheckPSL1GHTButton( BTN_L3, 6);
		CheckPSL1GHTButton( BTN_SELECT, 7);

		CheckPSL1GHTButton( BTN_SQUARE, 8);
		CheckPSL1GHTButton( BTN_CROSS, 9);
		CheckPSL1GHTButton( BTN_CIRCLE, 10);
		CheckPSL1GHTButton( BTN_TRIANGLE, 11);

		CheckPSL1GHTButton( BTN_R1, 12);
		CheckPSL1GHTButton( BTN_L1, 13);
		CheckPSL1GHTButton( BTN_R2, 14);
		CheckPSL1GHTButton( BTN_L2, 15);
	}
	else
		SDL_SetError("No joystick available with that index");

    return;
}

/* Function to close a joystick after use */
void
SDL_SYS_JoystickClose(SDL_Joystick * joystick)
{
    if (joystick->hwdata)
        SDL_free(joystick->hwdata);
    return;
}

/* Function to perform any system-specific joystick related cleanup */
void
SDL_SYS_JoystickQuit(void)
{
    SDL_numjoysticks = 0;
    return;
}

#endif /* SDL_JOYSTICK_PSL1GHT */

/* vi: set ts=4 sw=4 expandtab: */
