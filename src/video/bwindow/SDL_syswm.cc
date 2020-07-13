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

#include "SDL_BWin.h"

extern "C"
{

#include "SDL_syswm_c.h"
#include "SDL_error.h"

    void BE_SetWMCaption(_THIS, const char *title, const char *icon)
    {
        SDL_Win->SetTitle(title);
    }

    int BE_IconifyWindow(_THIS)
    {
        SDL_Win->Minimize(true);
    }

    int BE_GetWMInfo(_THIS, SDL_SysWMinfo * info)
    {
        if (info->version.major <= SDL_MAJOR_VERSION) {
            return 1;
        } else {
            SDL_SetError("Application not compiled with SDL %d.%d\n",
                         SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
            return -1;
        }
    }

};                              /* Extern C */

/* vi: set ts=4 sw=4 expandtab: */
