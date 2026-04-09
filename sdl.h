#ifndef SDLIO_H
#define SDLIO_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL3/SDL.h>

#include "config.h"

#define SCALE              12

typedef struct {
	SDL_Window      *window;
	SDL_Renderer    *renderer;
	SDL_Texture     *texture;
	SDL_AudioStream *audio_stream;
	uint32_t         pixel_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];
	int              audio_sample_pos;
	bool             running;
} SdlContext;

bool sdl_init(SdlContext *sdl);
void sdl_handle_events(SdlContext *sdl, uint8_t keypad[16]);
void sdl_update_display(SdlContext *sdl, const uint8_t *framebuffer,uint32_t color_foreground,uint32_t color_background);
void sdl_update_audio(SdlContext *sdl, uint8_t sound_timer);
void sdl_cleanup(SdlContext *sdl);

#endif
