#include <stdio.h>
#include <SDL/SDL.h>


#define INIT_W 840
#define INIT_V 640
#define INIT_BPP 32

static SDL_Surface *screen;

static int init(void)
{
	SDL_Init(SDL_INIT_EVERYTHING);

	const SDL_VideoInfo *info = SDL_GetVideoInfo();

	int vid_flags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER;
	if (info->hw_available)
		vid_flags |= SDL_HWSURFACE;
	else
		vid_flags |= SDL_SWSURFACE;

	screen = SDL_SetVideoMode(INIT_W, INIT_V, info->vfmt->BitsPerPixel, vid_flags);
	SDL_WM_SetCaption( "Event test", NULL );


	return 0;
}

static void fini(void)
{

	SDL_FreeSurface(screen);

	SDL_Quit();
}

#define __unused __attribute__((__unused__))

int main(__unused int argc, __unused char **argv)
{
	SDL_Event event;

	init();

	for(;;) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				goto done;
			default:
				printf("unkown event type %d\n", event.type);

			}
		}
	}

done:
	fini();

	return 0;
}
