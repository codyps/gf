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

#define __unused __attribute__((__unused__))

#define eprint(fmt, ...) do {			\
	fprintf(stderr, fmt, ## __VA_ARGS__);	\
	putc('\n', stderr);			\
} while (0)

struct tile {
	SDL_Rect rect;
	uint32_t color;
};

/* a set of tiles. */
struct grid {
	size_t w;
	size_t h;
	struct tile *ts;
};

/* a view of the grid with a particular centering "goal" */
struct view {
	/* centering target */
	int64_t x;
	int64_t y;
};

typedef int (tile_fn)(struct tile *t, int x, int y, void *data);

static SDL_Surface *screen;

static int grid_for_each_tile(struct grid *grid, tile_fn *op, void *data)
{
	size_t y, y_off;
	for (y = 0, y_off = 0; y < grid->h; y++, y_off += grid->w) {
		size_t x;
		for (x = 0; x < grid->w; x++) {
			struct tile *tile = grid->ts + y * grid->w + x;

			int ret = op(tile, x, y, data);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int tile_setup(struct tile *tile, int x, int y, __unused void *data)
{
	SDL_Rect *r = &tile->rect;
	r->w = RECT_EDGE;
	r->h = RECT_EDGE;
	r->x = x * RECT_EDGE;
	r->y = y * RECT_EDGE;

	tile->color = (y << 18) | ((x & 0xffff) << 2) | rand();

	return 0;
}

static int grid_init(struct grid *grid, int width, int height)
{
	grid->w = width;
	grid->h = height;
	grid->ts = malloc(sizeof(*grid->ts) * grid->w * grid->h);

	if (!grid->ts)
		return -1;

	grid_for_each_tile(grid, tile_setup, NULL);

	return 0;
}

static int tile_rand_color(struct tile *tile, __unused int x, __unused int y, __unused void *data)
{
	tile->color = rand();
	return 0;
}

static void grid_recolor(struct grid *grid)
{
	grid_for_each_tile(grid, tile_rand_color, NULL);
}

static int tile_draw(struct tile *tile, __unused int x, __unused int y, void *v_screen)
{
	SDL_Surface *screen = v_screen;
	if (SDL_FillRect(screen, &tile->rect, tile->color) == -1)
		eprint("FillRect");
	return 0;
}

static void grid_draw(struct grid *grid)
{
	grid_for_each_tile(grid, tile_draw, screen);
}

static int init(void)
{
	SDL_Init(SDL_INIT_EVERYTHING);

	const SDL_VideoInfo *info = SDL_GetVideoInfo();

	int vid_flags = 0
		/* | SDL_FULLSCREEN */
		/* | SDL_OPENGL */
		/* | SDL_GL_DOUBLEBUFFER */
		;
	if (info->hw_available) {
		eprint("Video: using hardware.");
		vid_flags |= SDL_HWSURFACE;
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

void handle_keypress(SDL_KeyboardEvent *key, struct grid *grid)
{
	SDL_keysym *ks = &key->keysym;

	switch(ks->sym) {
	case SDLK_LEFT:
		grid_recolor(grid);
		grid_draw(grid);
		break;
	default:
		eprint("unhandled keypress %d %d", ks->scancode, ks->unicode);
	}
}

int main(__unused int argc, __unused char **argv)
{
	SDL_Event event;
	struct grid grid;

	init();
	SDL_WM_SetCaption( "gf testing", NULL );

	if (grid_init(&grid, GRID_W, GRID_H) < 0) {
		eprint("grid_init failure");
		exit(-1);
	}

	grid_draw(&grid);

	for(;;) {
		while(SDL_WaitEvent(&event)) {
			switch(event.type) {
			case SDL_KEYDOWN:
				handle_keypress(&event.key, &grid);
				break;
			case SDL_QUIT:
				goto done;
			default:
				printf("unhandled event type %d\n", event.type);

			}
			SDL_Flip(screen);
		}
	}

done:
	fini();

	return 0;
}
