#include <stdio.h>
#include <time.h>

#include <SDL.h>

#include "must.h"
#include "util.h"

#include "config.h"

#define SCR_W 29
#define SCR_H 5

#define MAX_TIME (\
		/* hours   */ 99*3600u +\
		/* minutes */ 59*60u +\
		/* seconds */ 59u)

struct context {
	SDL_Window *win;
	SDL_Renderer *ren;
	SDL_Texture *tex;
	i32 win_x, win_y;
	u32 win_w, win_h;
	u32 pixels[SCR_W * SCR_H];
	bool should_quit;
};

struct timer {
	u8 h, m, s; /* hours, minutes, seconds */
	time_t sec_left, sec_end, sec_paused;
	bool finished;
};

struct context G;

void
cleanup(void)
{
	SDL_DestroyTexture(G.tex);
	SDL_DestroyRenderer(G.ren);
	SDL_DestroyWindow(G.win);
	SDL_Quit();
}

time_t
time_sync(void)
{
	time_t start = time(NULL), now;
	while ((now = time(NULL)) <= start)
		SDL_Delay(1);
	return now;
}

#define draw_pixel(x, y, color)\
	G.pixels[(y)*SCR_W + (x)] = (color);

void
draw_digit(u8 digit, i32 x, i32 y, u32 color)
{
	static const u8 font[10][3][3] = {
		{{1, 1, 1}, {1, 0, 1}, {1, 1, 1}}, /* 0 */
		{{1, 1, 0}, {0, 1, 0}, {1, 1, 1}}, /* 1 */
		{{1, 1, 0}, {0, 1, 0}, {0, 1, 1}}, /* 2 */
		{{1, 1, 0}, {0, 1, 1}, {1, 1, 0}}, /* 3 */
		{{1, 0, 1}, {1, 1, 1}, {0, 0, 1}}, /* 4 */
		{{0, 1, 1}, {0, 1, 0}, {1, 1, 0}}, /* 5 */
		{{1, 0, 0}, {1, 1, 1}, {1, 1, 1}}, /* 6 */
		{{1, 1, 1}, {0, 0, 1}, {0, 0, 1}}, /* 7 */
		{{1, 1, 1}, {1, 1, 1}, {1, 1, 1}}, /* 8 */
		{{1, 1, 1}, {1, 1, 1}, {0, 0, 1}}  /* 9 */
	};

	for (u8 dy = 0; dy < 3; ++dy)
		for (u8 dx = 0; dx < 3; ++dx)
			if (font[digit][dy][dx])
				draw_pixel(x+dx, y+dy, color);
}

int
main(int argc, char *argv[])
{
	struct timer t = {0};

	if (argc == 2) {
		t.sec_left = atoi(argv[1]);
	} else if (argc == 3) {
		t.sec_left = atoi(argv[1]) * 60;
		t.sec_left += atoi(argv[2]);
	} else if (argc == 4) {
		t.sec_left = atoi(argv[1]) * 3600;
		t.sec_left += atoi(argv[2]) * 60;
		t.sec_left += atoi(argv[3]);
	} else {
		fprintf(stderr, "Usage:	%s <hours> <minutes> <seconds>\n"
				"	%s <minutes> <seconds>\n"
				"	%s <seconds>\n"
				"Source: https://github.com/danielsource/countdown.c\n",
				argv[0], argv[0], argv[0]);
		return 1;
	}

	if (t.sec_left > MAX_TIME) {
		fprintf(stderr, "Maximum seconds exceeded (max: %u, got: %lu)\n",
				MAX_TIME, t.sec_left);
		return 1;
	}

	G.win_w = CFG_SIZE_PX * ((float)SCR_W/SCR_H);
	G.win_h = CFG_SIZE_PX;

	SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
	must(atexit(cleanup) == 0, "atexit");
	must(SDL_Init(SDL_INIT_VIDEO) == 0,
			"SDL_Init(): %s",
			SDL_GetError());

#if !(defined(CFG_WIN_X) && defined(CFG_WIN_Y))
	SDL_Rect dpy_bounds;
	must(SDL_GetDisplayBounds(0, &dpy_bounds) == 0,
			"SDL_GetDisplayBounds(): %s",
			SDL_GetError());
	G.win_x = dpy_bounds.w - G.win_w;
	G.win_y = 0;
#else
	G.win_x = CFG_WIN_X;
	G.win_y = CFG_WIN_Y;
#endif

	must(G.win = SDL_CreateWindow(argv[0],
				G.win_x, G.win_y, G.win_w, G.win_h,
				SDL_WINDOW_HIDDEN | CFG_WIN_FLAGS),
			"SDL_CreateWindow(): %s",
			SDL_GetError());
	must(G.ren = SDL_CreateRenderer(G.win, -1, 0),
			"SDL_CreateRenderer(): %s",
			SDL_GetError());
	must(G.tex = SDL_CreateTexture(G.ren,
				SDL_PIXELFORMAT_ARGB8888,
				SDL_TEXTUREACCESS_STREAMING,
				SCR_W, SCR_H),
			"SDL_CreateTexture(): %s",
			SDL_GetError());

#define B CFG_COLOR_BG
#define F CFG_COLOR_FG_ON
	u32 icon_pixels[5*5] = {
		B, B, B, B, B,
		B, F, F, F, B,
		B, F, B, B, B,
		B, F, F, F, B,
		B, B, B, B, B,
	};
#undef B
#undef F

	SDL_Surface *icon_sfc = SDL_CreateRGBSurfaceFrom(icon_pixels,
			5, 5, 32, 5*sizeof(u32),
			0x00ff0000,
			0x0000ff00,
			0x000000ff,
			0);
	if (icon_sfc) {
		SDL_Surface *out = SDL_CreateRGBSurface(0,
				96, 96, 32,
				0xff0000,
				0x00ff00,
				0x0000ff,
				0);
		SDL_BlitScaled(icon_sfc,
				&(SDL_Rect){0, 0, 5, 5},
				out,
				&(SDL_Rect){0, 0, 96, 96});
		SDL_SetWindowIcon(G.win, out);
		SDL_FreeSurface(out);
		SDL_FreeSurface(icon_sfc);
	}

	SDL_ShowWindow(G.win);

	t.sec_end = time_sync() + t.sec_left;
	time_t last_draw = t.sec_left + 1;
	while (!G.should_quit) {
		if (t.finished && CFG_QUIT_IMMEDIATELY)
			G.should_quit = true;

		if (!t.finished && !t.sec_paused) {
			time_t now = time(NULL);
			if (now < t.sec_end) {
				t.sec_left = t.sec_end - now;
				t.h = t.sec_left / 3600;
				t.m = (t.sec_left % 3600) / 60;
				t.s = t.sec_left % 60;
			} else {
				memset(&t, 0, sizeof(t));
				t.finished = true;
			}
		}

		for (SDL_Event ev; SDL_PollEvent(&ev);) {
			switch (ev.type) {
			case SDL_QUIT:
				G.should_quit = true;
				break;
			case SDL_KEYDOWN:
				if (t.finished)
					G.should_quit = true;
				else if (ev.key.keysym.sym == SDLK_p) {
					if (t.sec_paused == 0) {
						t.sec_paused = time(NULL);
					} else {
						t.sec_end += time(NULL) - t.sec_paused;
						t.sec_paused = 0;
					}
					goto draw;
				}
				break;
			}
		}

		if (t.sec_left != last_draw) {
draw:
			last_draw = t.sec_left;

			for (int i = 0; i < SCR_W*SCR_H; ++i)
				G.pixels[i] = CFG_COLOR_BG;

			u32 fg_on = (t.sec_paused || t.finished) ?
				CFG_COLOR_FG_END : CFG_COLOR_FG_ON;

			if (t.h > 0 || t.finished) {
				draw_digit(t.h/10, 1, 1, fg_on);
				draw_digit(t.h%10, 5, 1, fg_on);
			} else {
				draw_digit(0, 1, 1, CFG_COLOR_FG_OFF);
				draw_digit(0, 5, 1, CFG_COLOR_FG_OFF);
			}

			draw_pixel(9, 1, CFG_COLOR_COLON);
			draw_pixel(9, 3, CFG_COLOR_COLON);

			if (t.m > 0 || t.h > 0 || t.finished) {
				draw_digit(t.m/10, 11, 1, fg_on);
				draw_digit(t.m%10, 15, 1, fg_on);
			} else {
				draw_digit(0, 11, 1, CFG_COLOR_FG_OFF);
				draw_digit(0, 15, 1, CFG_COLOR_FG_OFF);
			}

			draw_pixel(19, 1, CFG_COLOR_COLON);
			draw_pixel(19, 3, CFG_COLOR_COLON);

			draw_digit(t.s/10, 21, 1, fg_on);
			draw_digit(t.s%10, 25, 1, fg_on);

			SDL_UpdateTexture(G.tex, NULL, G.pixels, SCR_W * sizeof(u32));
		}

		SDL_RenderClear(G.ren);
		SDL_RenderCopy(G.ren, G.tex, NULL, NULL);
		SDL_RenderPresent(G.ren);
		SDL_Delay(250);
	}

	cleanup();
	return 0;
}
