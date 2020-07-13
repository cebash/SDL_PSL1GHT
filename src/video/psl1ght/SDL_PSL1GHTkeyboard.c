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

static void unicodeToUtf8(Uint16 w, char *utf8buf)
{
    unsigned char *utf8s = (unsigned char *) utf8buf;

    if ( w < 0x0080 ) {
        utf8s[0] = ( unsigned char ) w;
        utf8s[1] = 0;
    }
    else if ( w < 0x0800 ) {
        utf8s[0] = 0xc0 | (( w ) >> 6 );
        utf8s[1] = 0x80 | (( w ) & 0x3f );
        utf8s[2] = 0;
    }
    else {
        utf8s[0] = 0xe0 | (( w ) >> 12 );
        utf8s[1] = 0x80 | (( ( w ) >> 6 ) & 0x3f );
        utf8s[2] = 0x80 | (( w ) & 0x3f );
        utf8s[3] = 0;
    }
}

static void updateKeymap(_THIS)
{
    SDL_DeviceData *data =
        (SDL_DeviceData *) _this->driverdata;

    SDL_Scancode scancode;
    SDL_Keycode keymap[SDL_NUM_SCANCODES];
    KbConfig kbConfig;
    KbMkey kbMkey;
    KbLed kbLed;
    Uint16 unicode;

    SDL_GetDefaultKeymap(keymap);

    ioKbGetConfiguration(0, &kbConfig);

    data->_keyboardMapping = kbConfig.mapping;

    kbMkey._KbMkeyU.mkeys = 0;
    kbLed._KbLedU.leds = 1; // Num lock

    // Update SDL keycodes according to the keymap
    for (scancode = 0; scancode < SDL_NUM_SCANCODES; ++scancode) {

        // Make sure this scancode is a valid character scancode
        if (scancode == SDL_SCANCODE_UNKNOWN ||
            scancode == SDL_SCANCODE_ESCAPE ||
            scancode == SDL_SCANCODE_RETURN ||
            (keymap[scancode] & SDLK_SCANCODE_MASK)) {
            continue;
        }

        unicode = ioKbCnvRawCode(data->_keyboardMapping, kbMkey, kbLed, scancode);

        // Ignore Keypad flag
        unicode &= ~KB_KEYPAD;

        // Exclude raw keys
        if (unicode != 0 && unicode < KB_RAWDAT) {
            keymap[scancode] = unicode;
        }
    }
    SDL_SetKeymap(0, keymap, SDL_NUM_SCANCODES);
}

static void checkKeyboardConnected(_THIS)
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

        // Set raw keyboard code types to get scan codes
        ioKbSetCodeType(0, KB_CODETYPE_RAW);
        ioKbSetReadMode(0, KB_RMODE_INPUTCHAR);

        updateKeymap(_this);
    }
    else if (kbInfo.status[0] != 1 && data->_keyboardConnected) // Disconnected
    {
        data->_keyboardConnected = false;

        SDL_ResetKeyboard();
    }
}

static void updateModifierKey(bool oldState, bool newState, SDL_Scancode scancode)
{
    if (!oldState ^ !newState) {
        SDL_SendKeyboardKey(newState ? SDL_PRESSED : SDL_RELEASED, scancode);
    }
}

static void updateModifiers(_THIS, const KbData *Keys)
{
    SDL_Keymod modstate = SDL_GetModState();

    updateModifierKey(modstate & KMOD_LSHIFT, Keys->mkey._KbMkeyU._KbMkeyS.l_shift, SDL_SCANCODE_LSHIFT);
    updateModifierKey(modstate & KMOD_RSHIFT, Keys->mkey._KbMkeyU._KbMkeyS.r_shift, SDL_SCANCODE_RSHIFT);
    updateModifierKey(modstate & KMOD_LCTRL, Keys->mkey._KbMkeyU._KbMkeyS.l_ctrl, SDL_SCANCODE_LCTRL);
    updateModifierKey(modstate & KMOD_RCTRL, Keys->mkey._KbMkeyU._KbMkeyS.r_ctrl, SDL_SCANCODE_RCTRL);
    updateModifierKey(modstate & KMOD_LALT, Keys->mkey._KbMkeyU._KbMkeyS.l_alt, SDL_SCANCODE_LALT);
    updateModifierKey(modstate & KMOD_RALT, Keys->mkey._KbMkeyU._KbMkeyS.r_alt, SDL_SCANCODE_RALT);
    updateModifierKey(modstate & KMOD_LGUI, Keys->mkey._KbMkeyU._KbMkeyS.l_win, SDL_SCANCODE_LGUI);
    updateModifierKey(modstate & KMOD_RGUI, Keys->mkey._KbMkeyU._KbMkeyS.r_win, SDL_SCANCODE_RGUI);
}

static void updateKeys(_THIS, const KbData *Keys)
{
    SDL_DeviceData *data =
        (SDL_DeviceData *) _this->driverdata;

    int x = 0;
    int numKeys = 0;
    Uint8 newkeystate[SDL_NUM_SCANCODES];
    Uint8 * keystate = SDL_GetKeyboardState(&numKeys);
    Uint16 unicode;
    SDL_Scancode scancode;

    for (scancode = 0; scancode < SDL_NUM_SCANCODES; ++scancode) {
        newkeystate[scancode] = SDL_RELEASED;
    }

    for (x = 0; x < Keys->nb_keycode; x++) {
        if (Keys->keycode[0] != 0)
            newkeystate[Keys->keycode[x]] = SDL_PRESSED;
    }

    for (scancode = 0; scancode < SDL_NUM_SCANCODES; ++scancode) {
        if ((newkeystate[scancode] != keystate[scancode])
                && (scancode < SDL_SCANCODE_LCTRL || scancode > SDL_SCANCODE_RGUI)) {

            // Send new key state
            SDL_SendKeyboardKey(newkeystate[scancode], scancode);

            // Send the text corresponding to the keypress
            if (newkeystate[scancode] == SDL_PRESSED) {
                // Convert scancode
                unicode = ioKbCnvRawCode(data->_keyboardMapping, Keys->mkey, Keys->led, scancode);

                // Ignore Keypad flag
                unicode &= ~KB_KEYPAD;

                // Exclude raw keys
                if (unicode != 0 && unicode < KB_RAWDAT) {
                    char utf8[SDL_TEXTINPUTEVENT_TEXT_SIZE];

                    // Convert from Unicode to UTF-8
                    unicodeToUtf8(unicode, utf8);
                    SDL_SendKeyboardText(utf8);
                }
            }
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

        // Read data from the keyboard buffer
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

    // Init the PS3 Keyboard
    ioKbInit(1);

    data->_keyboardConnected = false;
}

void
PSL1GHT_QuitKeyboard(_THIS)
{
    ioKbEnd();
}

/* vi: set ts=4 sw=4 expandtab: */
