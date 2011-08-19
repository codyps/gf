#include <stdio.h>
#include <stdint.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include <GL/gl.h>

#define INIT_W 840
#define INIT_V 640

#define RECT_EDGE 25

#define GRID_W 100
#define GRID_H 100

#define eprint(fmt, ...) do {			\
	fprintf(stderr, fmt, ## __VA_ARGS__);	\
	putc('\n', stderr);			\
} while (0)

struct tile {
	SDL_Rect rect;

	uint32_t color;
};

static struct tile grid[GRID_H * GRID_W];

static SDL_Surface *screen;

static void grid_init(struct tile *grid, int width, int vert)
{
	int v, v_off;
	for (v = 0, v_off = 0; v < vert; v++, v_off += width) {
		int w;
		for (w = 0; w < width; w++) {
			struct tile *tile = grid + v * width + w;
			SDL_Rect *r = &tile->rect;
			r->w = RECT_EDGE;
			r->h = RECT_EDGE;
			r->x = w * RECT_EDGE;
			r->y = v * RECT_EDGE;

			tile->color = (v << 18) | ((w & 0xffff) << 2) | rand();
		}
	}
}

static void grid_draw(struct tile *grid, int width, int vert)
{
	int v;
	for (v = 0; v < vert; v++) {
		int w;
		for (w = 0; w < width; w++) {
			struct tile *tile = grid + v * width + w;
			if (SDL_FillRect(screen, &tile->rect, tile->color) == -1)
				eprint("FillRect\n");
		}
	}
}

static int init(void)
{
	SDL_Init(SDL_INIT_EVERYTHING);

	const SDL_VideoInfo *info = SDL_GetVideoInfo();

	int vid_flags = 0;
	if (info->hw_available) {
		eprint("Video: using hardware.");
		vid_flags |= SDL_HWSURFACE | SDL_OPENGL | SDL_GL_DOUBLEBUFFER;
	} else {
		eprint("Video: using software.");
		vid_flags |= SDL_SWSURFACE;
	}

	screen = SDL_SetVideoMode(INIT_W, INIT_V, info->vfmt->BitsPerPixel, vid_flags);

	/* GL 2d setup */
#if 0
	glViewport(0, 0, INIT_W, INIT_V);
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, INIT_W, INIT_V, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
#endif

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
	SDL_WM_SetCaption( "gf testing", NULL );

	grid_init(grid, GRID_W, GRID_H);
	grid_draw(grid, GRID_W, GRID_H);

	for(;;) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				goto done;
			default:
				printf("unhandled event type %d\n", event.type);

			}
		}
	}

done:
	fini();

	return 0;
}
