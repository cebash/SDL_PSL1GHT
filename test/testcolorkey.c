/*
   METATOWN PS3 GAME
    
*/

#include <psl1ght/lv2.h>

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

#include <io/pad.h>

#include <SDL/SDL.h>

int currentColor;
int main(int argc, const char* argv[])
{
        SDL_Surface *screen;
        SDL_Surface *sprite;

	PadInfo padinfo;
	PadData paddata;
	
        int i;

        int width, height;
        Uint8 video_bpp;
        Uint32 videoflags;
        SDL_Rect playpos;
         
        playpos.x = 500;
        playpos.y = 500;

        SDL_setenv("SDL_VIDEODRIVER", "psl1ght", 1);
        SDL_Init(SDL_INIT_VIDEO);
            width = 1920;
            height = 1080;
            video_bpp = 16;
            videoflags = 0;
       
        ioPadInit(7);        


        screen = SDL_SetVideoMode(1920, 1080, video_bpp, 0);
                                currentColor = 0xFFFFFFFF;

        sprite = SDL_LoadBMP("player.bmp");
        SDL_SetColorKey(sprite, SDL_RLEACCEL|SDL_SRCCOLORKEY, * (Uint8 *)(sprite->pixels));

        while(1) {
		ioPadGetInfo(&padinfo);
		for(i=0; i<MAX_PADS; i++){
			if(padinfo.status[i]){
				ioPadGetData(i, &paddata);
				
				if(paddata.BTN_CROSS){
				return 0;
				}
                                if(paddata.BTN_UP) {
                                playpos.y -= 20;
                                }
                                if(paddata.BTN_DOWN) {
                                playpos.y += 20;
                                }
			}
			
		}



        SDL_FillRect(screen, NULL, currentColor);
        SDL_BlitSurface(sprite, NULL, screen, &playpos);
        SDL_Flip(screen);


        }

        sleep(2);
        SDL_Quit();
	return 0;
}
