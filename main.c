#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <SDL/SDL.h>
//#include <SDL/SDL_image.h>
//#include <SDL/SDL_ttf.h>

#include <GL/gl.h>

#define VIEW_W 840
#define VIEW_H 640

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
struct world {
	size_t w;
	size_t h;
	struct tile *ts;
};

struct location {
	int64_t x;
	int64_t y;
	struct world *world;
};

/* something which exsists in the world */
struct actor {
	struct location loc;
};

/* a view of the world with a particular centering "goal" */
struct view {
	/* the world this is a view of (worlds may have multiple views, views
	 * are only of a single world) */
	struct location loc;

	unsigned w, h; /* size of the view (pixels) */
	unsigned edge; /* size of a tile edge (pixels) */
};

typedef int (tile_fn)(struct tile *t, int x, int y, void *data);

static SDL_Surface *screen;

static int world_for_each_tile(struct world *world, tile_fn *op, void *data)
{
	size_t y, y_off;
	for (y = 0, y_off = 0; y < world->h; y++, y_off += world->w) {
		size_t x;
		for (x = 0; x < world->w; x++) {
			struct tile *tile = world->ts + y * world->w + x;

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

static int world_init(struct world *world, int width, int height)
{
	world->w = width;
	world->h = height;
	world->ts = malloc(sizeof(*world->ts) * world->w * world->h);

	if (!world->ts)
		return -1;

	world_for_each_tile(world, tile_setup, NULL);

	return 0;
}

static int tile_rand_color(struct tile *tile, __unused int x, __unused int y, __unused void *data)
{
	tile->color = rand();
	return 0;
}

static void world_recolor(struct world *world)
{
	world_for_each_tile(world, tile_rand_color, NULL);
}

static int tile_draw(struct tile *tile, __unused int x, __unused int y, void *v_screen)
{
	SDL_Surface *screen = v_screen;
	if (SDL_FillRect(screen, &tile->rect, tile->color) == -1)
		eprint("FillRect");
	return 0;
}

#define div_ceil(x,y) (((x) > 0) ? 1 + ((x) - 1)/(y) : ((x) / (y)))

static bool world_contains(struct world *world, int64_t x, int64_t y)
{
	int64_t xs[2] = { world->w / 2, -div_ceil(world->w, 2) };
	int64_t ys[2] = { world->h / 2, -div_ceil(world->h, 2) };

	return x < xs[0] && x > xs[1] && y < ys[0] && y > ys[1];
}

static int view_draw(struct view *view)
{

	/* number of tiles left and right of the target */
	unsigned w_tiles = div_ceil(div_ceil(view->w, 2) - view->edge, view->edge);

	/* number of tiles above and below the target */
	unsigned h_tiles = div_ceil(div_ceil(view->h, 2) - view->edge, view->edge);

	/* starting and ending points */
	int w[2] = { view->loc.x - w_tiles, view->loc.x + w_tiles };
	int h[2] = { view->loc.y - h_tiles, view->loc.y + h_tiles };

	/* check for cliping */
	if     (!world_contains(view->loc.world, w[0], h[0])) {
	} else if (!world_contains(view->loc.world, w[1], h[1])) {
	}

	/* (view->loc.x, view->loc.y) is the tile to center. */


	return world_for_each_tile(view->loc.world, tile_draw, screen);
}

static int view_init_attach(struct view *view, unsigned w, unsigned h, unsigned edge, int x, int y, struct world *world)
{
	view->w = w;
	view->h = h;
	view->edge = edge;

	view->loc.x = x;
	view->loc.y = y;
	view->loc.world = world;


	/* possibly inform the world about the view */

	return 0;
}

static int init(void)
{
	SDL_Init(SDL_INIT_EVERYTHING);

	const SDL_VideoInfo *info = SDL_GetVideoInfo();

#ifdef USE_OPENGL
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif

	int vid_flags = SDL_DOUBLEBUF
		/* | SDL_FULLSCREEN */
		/* | SDL_OPENGL */
		;
	if (info->hw_available) {
		eprint("Video: using hardware.");
		vid_flags |= SDL_HWSURFACE;
	} else {
		eprint("Video: using software.");
		vid_flags |= SDL_SWSURFACE;
	}

	screen = SDL_SetVideoMode(VIEW_W, VIEW_H, info->vfmt->BitsPerPixel, vid_flags);

#if USE_OPENGL
	/* GL 2d setup */
	glViewport(0, 0, VIEW_W, VIEW_H);
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, VIEW_W, VIEW_H, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
#endif

	return 0;
}

static void fini(void)
{
	/* "screen" is freed by SDL_Quit() */
	SDL_Quit();
}

static void handle_keypress(SDL_KeyboardEvent *key, struct view *view)
{
	struct world *world = view->loc.world;
	SDL_keysym *ks = &key->keysym;

	switch(ks->sym) {
	case SDLK_F4:
		world_recolor(world);
		view_draw(view);
		break;

	case SDLK_LEFT:
		break;
	case SDLK_RIGHT:
		break;
	case SDLK_UP:
		break;
	case SDLK_DOWN:
		break;

	default:
		eprint("unhandled keypress %d %d", ks->scancode, ks->unicode);
	}
}

int main(__unused int argc, __unused char **argv)
{
	SDL_Event event;
	struct view view;
	struct world world;

	init();
	SDL_WM_SetCaption("gf testing", NULL);

	world_init(&world, GRID_W, GRID_H);
	view_init_attach(&view, VIEW_W, VIEW_H, RECT_EDGE, 0, 0, &world);

	view_draw(&view);

	for(;;) {
		while(SDL_WaitEvent(&event)) {
			switch(event.type) {
			case SDL_KEYDOWN:
				handle_keypress(&event.key, &view);
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
