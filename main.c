#include <stdio.h>

#include <SDL/SDL.h>
#include <GL/gl.h>

#define INIT_W 840
#define INIT_V 640

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

	/* GL 2d setup */
	glViewport(0, 0, INIT_W, INIT_V);
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, INIT_W, INIT_V, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);

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
	SDL_WM_SetCaption( "Event test", NULL );

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
