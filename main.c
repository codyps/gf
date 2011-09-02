#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <SDL/SDL.h>
//#include <SDL/SDL_image.h>
//#include <SDL/SDL_ttf.h>

#ifdef USE_OPENGL
# include <GL/gl.h>
#endif

static SDL_Rect default_vid_mode = { .w = 860, .h = 640 };
static SDL_Rect *vid_mode = &default_vid_mode;
static SDL_Rect **vid_modes;

#define RECT_EDGE 25

#define GRID_W 100
#define GRID_H 100

#define __unused __attribute__((__unused__))

#define eprint(fmt, ...) do {			\
	fprintf(stderr, fmt, ## __VA_ARGS__);	\
	putc('\n', stderr);			\
} while (0)

struct tile {
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

	unsigned off_px, off_py;
};

typedef int (tile_fn)(struct tile *t, int x, int y, void *data);

static SDL_Surface *screen;

#define w_for_each_tile(world, tile, i) for (i = 0; i < world->h * world->w && ( tile = world->ts + i, true ) ; i++)

static int w_ix_to_x(struct world *world, int i)
{
	return i / world->w;
}

static int w_ix_to_y(struct world *world, int i)
{
	return i % world->w;
}

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

#define div_ceil(x,y) (((x) > 0) ? 1 + ((x) - 1)/(y) : ((x) / (y)))

#if 0
static bool world_contains(struct world *world, int64_t x, int64_t y)
{
	int64_t xs[2] = { world->w / 2, -div_ceil(world->w, 2) };
	int64_t ys[2] = { world->h / 2, -div_ceil(world->h, 2) };

	return x < xs[0] && x > xs[1] && y < ys[0] && y > ys[1];
}
#endif

static int tile_draw(struct tile *tile, unsigned px, unsigned py, unsigned edge_len, SDL_Surface *screen)
{
	SDL_Rect rect = {
		.x = px,
		.y = py,
		.w = edge_len,
		.h = edge_len
	};

	if (SDL_FillRect(screen, &rect, tile->color) == -1)
		eprint("failure in SDL_FillRect");
	return 0;
}

static void view_draw(struct view *view)
{

	/* number of tiles left and right of the target */
	unsigned w_tiles = div_ceil(div_ceil(view->w, 2) - view->edge, view->edge);

	/* number of tiles above and below the target */
	unsigned h_tiles = div_ceil(div_ceil(view->h, 2) - view->edge, view->edge);

	/* starting and ending points */
	int w[2] = { view->loc.x - w_tiles, view->loc.x + w_tiles };
	int h[2] = { view->loc.y - h_tiles, view->loc.y + h_tiles };

	/* check for cliping */
	int w_constrain = 0, h_constrain = 0;

	struct world *world = view->loc.world;
	if (w[0] < 0)
		w_constrain = -1;
	else if (w[1] > 0 && (unsigned) w[1] > world->w)
		w_constrain = +1;

	if (h[0] < 0)
		h_constrain = -1;
	else if (h[1] > 0 && (unsigned) h[1] > world->h)
		h_constrain = +1;

	/* (view->loc.x, view->loc.y) is the tile to center. */

	SDL_Rect back = {
		.x = 0,
		.y = 0,
		.w = view->w,
		.h = view->h
	};

	SDL_FillRect(screen, &back, 0);


	struct tile *tile;
	unsigned i;
	w_for_each_tile(world, tile, i) {
		int y = w_ix_to_y(world, i);
		int x = w_ix_to_x(world, i);
		unsigned px = x * view->edge + view->off_px;
		unsigned py = y * view->edge + view->off_py;
		tile_draw(tile, px, py, view->edge, screen);
	}

}

static int view_init_attach(struct view *view, unsigned w, unsigned h, unsigned edge, int x, int y, struct world *world)
{

	view->off_px = view->off_py = 0;

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

	if (!info) {
		eprint("Video: none found, exiting.");
		exit(EXIT_FAILURE);
	}

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

	SDL_Rect **modes = SDL_ListModes(info->vfmt, vid_flags);

	if (!modes) {
		eprint("Video: no modes avaliable.");
		exit(EXIT_FAILURE);
	} else if (modes == (SDL_Rect **)-1) {
		eprint("Video: no restriction on modes.");
	} else {
		vid_modes = modes;
		int i;
		for(i = 0; modes[i]; i++) {
			eprint("vid_mode: %d x %d", modes[i]->w, modes[i]->h);
			vid_mode = modes[i];
		}
	}

	screen = SDL_SetVideoMode(vid_mode->w, vid_mode->h, info->vfmt->BitsPerPixel, vid_flags);
	SDL_ShowCursor(SDL_DISABLE);

#if USE_OPENGL
	/* GL 2d setup */
	glViewport(0, 0, vid_mode->w, vid_mode->h);
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, vid_mode->w, vid_mode->h, 0, -1, 1);
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

static void view_move_x(struct view *view, int shift)
{
	view->off_px += shift;
}

static void view_move_y(struct view *view, int shift)
{
	view->off_py += shift;
}


#define SHIFT_PX 10

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
		view_move_x(view, -SHIFT_PX * view->edge);
		view_draw(view);
		break;

	case SDLK_RIGHT:
		view_move_x(view, SHIFT_PX * view->edge);
		view_draw(view);
		break;
	case SDLK_UP:
		view_move_y(view, -SHIFT_PX * view->edge);
		view_draw(view);
		break;
	case SDLK_DOWN:
		view_move_y(view, SHIFT_PX * view->edge);
		view_draw(view);
		break;

	case SDLK_q:
		fini();
		exit(EXIT_SUCCESS);
		break;

	case SDLK_KP_PLUS:
	case SDLK_PLUS:
	case SDLK_EQUALS:
		if ((int)view->edge < screen->h && (int)view->edge < screen->w) {
			/* TODO: make this non-linear */
			view->edge *= 2;
			view_draw(view);
		}
		break;

	case SDLK_KP_MINUS:
	case SDLK_MINUS:
		if (view->edge > 1) {
			/* TODO: make this non-linear */
			view->edge /= 2;
			view_draw(view);
		}
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
	view_init_attach(&view, vid_mode->w, vid_mode->h, RECT_EDGE, 0, 0, &world);

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
				eprint("unhandled event type %d", event.type);

			}
			SDL_Flip(screen);
		}
	}

done:
	fini();

	return 0;
}
