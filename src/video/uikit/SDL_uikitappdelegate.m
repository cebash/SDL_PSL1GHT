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

#import "../SDL_sysvideo.h"

#import "SDL_uikitappdelegate.h"
#import "SDL_uikitopenglview.h"
#import "SDL_events_c.h"
#import "jumphack.h"

#ifdef main
#undef main
#endif

extern int SDL_main(int argc, char *argv[]);
static int forward_argc;
static char **forward_argv;

int main(int argc, char **argv) {

    int i;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    /* store arguments */
    forward_argc = argc;
    forward_argv = (char **)malloc((argc+1) * sizeof(char *));
    for (i=0; i<argc; i++) {
        forward_argv[i] = malloc( (strlen(argv[i])+1) * sizeof(char));
        strcpy(forward_argv[i], argv[i]);
    }
    forward_argv[i] = NULL;

    /* Give over control to run loop, SDLUIKitDelegate will handle most things from here */
    UIApplicationMain(argc, argv, NULL, [SDLUIKitDelegate getAppDelegateClassName]);
    
    [pool release];
    return 0;
}

@implementation SDLUIKitDelegate

/* convenience method */
+(SDLUIKitDelegate *)sharedAppDelegate {
    /* the delegate is set in UIApplicationMain(), which is garaunteed to be called before this method */
    return (SDLUIKitDelegate *)[[UIApplication sharedApplication] delegate];
}

+(NSString *)getAppDelegateClassName {
    /* subclassing notice: when you subclass this appdelegate, make sure to add a category to override
       this method and return the actual name of the delegate */
    return @"SDLUIKitDelegate";
}

- (id)init {
    self = [super init];
    return self;
}

- (void)postFinishLaunch {

    /* run the user's application, passing argc and argv */
    int exit_status = SDL_main(forward_argc, forward_argv);
    
    /* free the memory we used to hold copies of argc and argv */
    int i;
    for (i=0; i<forward_argc; i++) {
        free(forward_argv[i]);
    }
    free(forward_argv);    
        
    /* exit, passing the return status from the user's application */
    exit(exit_status);
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
            
    /* Set working directory to resource path */
    [[NSFileManager defaultManager] changeCurrentDirectoryPath: [[NSBundle mainBundle] resourcePath]];
    
    [self performSelector:@selector(postFinishLaunch) withObject:nil afterDelay:0.0];

    return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application {
    
    SDL_SendQuit();
     /* hack to prevent automatic termination.  See SDL_uikitevents.m for details */
    longjmp(*(jump_env()), 1);
}

- (void) applicationWillResignActive:(UIApplication*)application
{
    //NSLog(@"%@", NSStringFromSelector(_cmd));

    // Send every window on every screen a MINIMIZED event.
    SDL_VideoDevice *_this = SDL_GetVideoDevice();
    if (!_this) {
        return;
    }
	
    SDL_Window *window;
    for (window = _this->windows; window != nil; window = window->next) {
        SDL_SendWindowEvent(window, SDL_WINDOWEVENT_MINIMIZED, 0, 0);
    }
}

- (void) applicationDidBecomeActive:(UIApplication*)application
{
    //NSLog(@"%@", NSStringFromSelector(_cmd));

    // Send every window on every screen a RESTORED event.
    SDL_VideoDevice *_this = SDL_GetVideoDevice();
    if (!_this) {
        return;
    }
	
	SDL_Window *window;
    for (window = _this->windows; window != nil; window = window->next) {
		SDL_SendWindowEvent(window, SDL_WINDOWEVENT_RESTORED, 0, 0);
    }
}

@end

/* vi: set ts=4 sw=4 expandtab: */
