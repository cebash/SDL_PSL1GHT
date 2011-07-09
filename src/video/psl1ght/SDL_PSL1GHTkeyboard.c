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
#include "../../events/scancodes_linux.h"

#include <io/kb.h>

#include "SDL_PSL1GHTkeyboard_c.h"

static void updateKeymap()
{
    int i;
    SDL_Scancode scancode;
    SDL_Keycode keymap[SDL_NUM_SCANCODES];
    KbConfig kbConfig;
    KbMkey kbMkey;
    KbLed kbLed;

    SDL_GetDefaultKeymap(keymap);

    ioKbGetConfiguration(0, &kbConfig);

    kbMkey._KbMkeyU.mkeys = 0;
    kbLed._KbLedU.leds = 0;

    for (i = 0; i < SDL_arraysize(linux_scancode_table); i++) {
        /* Make sure this scancode is a valid character scancode */
        scancode = linux_scancode_table[i];
        if (scancode == SDL_SCANCODE_UNKNOWN ||
            (keymap[scancode] & SDLK_SCANCODE_MASK)) {
            continue;
        }

        keymap[scancode] = ioKbCnvRawCode(kbConfig.mapping, kbMkey, kbLed, scancode);
    }
    SDL_SetKeymap(0, keymap, SDL_NUM_SCANCODES);
}

void checkKeyboardConnected(_THIS)
{
    SDL_DeviceData *data =
        (SDL_DeviceData *) _this->driverdata;

    KbInfo kbInfo;
    ioKbGetInfo(&kbInfo);

    if (kbInfo.status[0] == 1 && !data->_keyboardConnected) // Connected
    {
        data->_keyboardConnected = true;

        // Old events in the queue are discarded
        ioKbClearBuf(0);

        //set raw keyboard code types to get scan codes
        ioKbSetCodeType(0, KB_CODETYPE_RAW);
        ioKbSetReadMode(0, KB_RMODE_PACKET);

        updateKeymap();
    }
    else if (kbInfo.status[0] != 1 && data->_keyboardConnected) // Disconnected
    {
        data->_keyboardConnected = false;
    }
}

void updateModifiers(_THIS, const KbData *Keys)
{
    SDL_Keymod modstate = SDL_GetModState();

    /* Left Shift */
    if (Keys->mkey._KbMkeyU._KbMkeyS.l_shift == 1 && !(modstate & KMOD_LSHIFT)) {
        SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_LSHIFT);
    } else if (Keys->mkey._KbMkeyU._KbMkeyS.l_shift == 0 && (modstate & KMOD_LSHIFT)) {
        SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_LSHIFT);
    }

    /* Right Shift */
    if (Keys->mkey._KbMkeyU._KbMkeyS.r_shift == 1 && !(modstate & KMOD_RSHIFT)) {
        SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_RSHIFT);
    } else if (Keys->mkey._KbMkeyU._KbMkeyS.r_shift == 0  && (modstate & KMOD_RSHIFT)) {
        SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_RSHIFT);
    }

    /* Left Control */
    if (Keys->mkey._KbMkeyU._KbMkeyS.l_ctrl == 1 && !(modstate & KMOD_LCTRL)) {
        SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_LCTRL);
    } else if (Keys->mkey._KbMkeyU._KbMkeyS.l_ctrl == 0 && (modstate & KMOD_LCTRL)) {
        SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_LCTRL);
    }

    /* Right Control */
    if (Keys->mkey._KbMkeyU._KbMkeyS.r_ctrl == 1 && !(modstate & KMOD_RCTRL)) {
        SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_RCTRL);
    } else if (Keys->mkey._KbMkeyU._KbMkeyS.r_ctrl == 0  && (modstate & KMOD_RCTRL)) {
        SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_RCTRL);
    }
}

void updateKeys(_THIS, const KbData *Keys)
{
    int x = 0;
    int numKeys = 0;
    Uint8 newkeystate[SDL_NUM_SCANCODES];
    Uint8 * keystate = SDL_GetKeyboardState(&numKeys);

    SDL_Scancode scancode;

    for (scancode = 0; scancode < SDL_NUM_SCANCODES; ++scancode) {
        newkeystate[scancode] = SDL_RELEASED;
    }

    for (x = 0; x < Keys->nb_keycode; x++) {
        if (Keys->keycode[0] != 0)
            newkeystate[Keys->keycode[x]] = SDL_PRESSED;
    }

    for (scancode = 0; scancode < SDL_NUM_SCANCODES; ++scancode) {
        if (newkeystate[scancode] != keystate[scancode]
                && (scancode < SDL_SCANCODE_LCTRL || scancode > SDL_SCANCODE_RGUI)) {
            SDL_SendKeyboardKey(newkeystate[scancode], scancode);
        }
    }
}

void
PSL1GHT_PumpKeyboard(_THIS)
{
    SDL_DeviceData *data =
        (SDL_DeviceData *) _this->driverdata;

    checkKeyboardConnected(_this);

    if (data->_keyboardConnected) {
        KbData Keys;

        if (ioKbRead(0, &Keys) == 0 && Keys.nb_keycode > 0) {
            updateModifiers(_this, &Keys);
            updateKeys(_this, &Keys);
        }
    }
}

void
PSL1GHT_InitKeyboard(_THIS)
{
    SDL_DeviceData *data =
        (SDL_DeviceData *) _this->driverdata;

    /*Init the PS3 Keyboard*/
    ioKbInit(1);

    data->_keyboardConnected = false;
}

void
PSL1GHT_QuitKeyboard(_THIS)
{
    ioKbEnd();
}

/* vi: set ts=4 sw=4 expandtab: */
