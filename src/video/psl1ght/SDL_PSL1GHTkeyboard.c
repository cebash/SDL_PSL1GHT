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
#include "SDL_events.h"
#include "../../events/SDL_keyboard_c.h"

#include <io/kb.h>

#include "SDL_PSL1GHTkeyboard_c.h"

static Uint8 * newkeystate = 0;

void
PSL1GHT_PumpKeyboard(_THIS)
{
    KbData Keys;

    s32 ret;
    int x = 0;
    int numKeys = 0;

    SDL_Keymod modstate = SDL_GetModState();
    Uint8 * keystate = SDL_GetKeyboardState(&numKeys);
    memcpy(newkeystate, keystate, numKeys);

    if (ioKbRead(0, &Keys) == 0) {
        //read Keys.
        /* Left Shift */
        if (Keys.mkey._KbMkeyU._KbMkeyS.l_shift == 1 && !(modstate & KMOD_LSHIFT)) {
            ret = SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_LSHIFT);
        } else if (Keys.mkey._KbMkeyU._KbMkeyS.l_shift == 0 && (modstate & KMOD_LSHIFT)) {
            ret = SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_LSHIFT);
        }

        /* Right Shift */
        if (Keys.mkey._KbMkeyU._KbMkeyS.r_shift == 1 && !(modstate & KMOD_RSHIFT)) {
            ret = SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_RSHIFT);
        } else if (Keys.mkey._KbMkeyU._KbMkeyS.r_shift == 0  && (modstate & KMOD_RSHIFT)) {
            ret = SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_RSHIFT);
        }

        /* Left Control */
        if (Keys.mkey._KbMkeyU._KbMkeyS.l_ctrl == 1 && !(modstate & KMOD_LCTRL)) {
            ret = SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_LCTRL);
        } else if (Keys.mkey._KbMkeyU._KbMkeyS.l_ctrl == 0 && (modstate & KMOD_LCTRL)) {
            ret = SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_LCTRL);
        }

        /* Right Control */
        if (Keys.mkey._KbMkeyU._KbMkeyS.r_ctrl == 1 && !(modstate & KMOD_RCTRL)) {
            ret = SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_RCTRL);
        } else if (Keys.mkey._KbMkeyU._KbMkeyS.r_ctrl == 0  && (modstate & KMOD_RCTRL)) {
            ret = SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_RCTRL);
        }

        SDL_Scancode scancode;

        for (scancode = 0; scancode < SDL_NUM_SCANCODES; ++scancode) {
            newkeystate[scancode] = SDL_RELEASED;
        }

        for (x = 0; x < Keys.nb_keycode; x++) {
            if (Keys.keycode[x] != 0)
                newkeystate[scancode] = SDL_PRESSED;
        }

        for (scancode = 0; scancode < SDL_NUM_SCANCODES; ++scancode) {
            if (newkeystate[scancode] != keystate[scancode]) {
                ret = SDL_SendKeyboardKey(newkeystate[scancode], scancode);
            }
        }
    }
}

void
PSL1GHT_InitKeyboard(_THIS)
{
    int numKeys = 0;
    SDL_GetKeyboardState(&numKeys);
    newkeystate = malloc(numKeys * sizeof(Uint8));

    /*Init the PS3 Keyboard*/
    ioKbInit(1);

    //set raw keyboard code types to get scan codes
    ioKbSetCodeType(0, KB_CODETYPE_RAW);
}

void
PSL1GHT_QuitKeyboard(_THIS)
{
    free(newkeystate);

    ioKbEnd();
}

/* vi: set ts=4 sw=4 expandtab: */
