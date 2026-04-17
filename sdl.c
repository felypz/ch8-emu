#include <stdio.h>
#include <stdint.h>
#include "sdl.h"

bool sdl_init(SdlContext *sdl) {
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		fprintf(stderr, "error: SDL_Init failed: %s\n", SDL_GetError());
		return false;
	}

	if (!SDL_CreateWindowAndRenderer("Chip-8 Emulator",
	                                 SCREEN_WIDTH * SCALE,
	                                 SCREEN_HEIGHT * SCALE,
	                                 0, &sdl->window, &sdl->renderer)) {

		fprintf(stderr, "error: SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
		return false;
	}

	SDL_SetRenderLogicalPresentation(sdl->renderer,
	                                 SCREEN_WIDTH,
	                                 SCREEN_HEIGHT,
	                                 SDL_LOGICAL_PRESENTATION_LETTERBOX);

	sdl->texture = SDL_CreateTexture(sdl->renderer,
	                                 SDL_PIXELFORMAT_XRGB8888,
	                                 SDL_TEXTUREACCESS_STREAMING,
	                                 SCREEN_WIDTH, SCREEN_HEIGHT);

	if (!sdl->texture) {
		fprintf(stderr, "error: SDL_CreateTexture failed: %s\n", SDL_GetError());
		return false;
	}

	SDL_SetTextureScaleMode(sdl->texture, SDL_SCALEMODE_NEAREST);

	SDL_AudioSpec spec = {
		.format   = SDL_AUDIO_S16,
		.channels = 1,
		.freq     = AUDIO_SAMPLE_RATE
	};

	sdl->audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
	                                              &spec, NULL, NULL);

	if (!sdl->audio_stream) {
		fprintf(stderr, "error: SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
		return false;
	}

	SDL_ResumeAudioStreamDevice(sdl->audio_stream);

	sdl->audio_sample_pos = 0;
	sdl->running          = true;
	sdl->save_requested   = false;
	sdl->load_requested   = false;
	sdl->pause_requested  = false;
	sdl->reset_requested  = false;
	sdl->speed_delta      = 0;

	return true;
}

void sdl_update_display(SdlContext *sdl, const uint8_t *framebuffer, Colorscheme colors) {
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) sdl->pixel_buffer[i] = framebuffer[i] ? colors.fg : colors.bg;

	SDL_UpdateTexture(sdl->texture, NULL, sdl->pixel_buffer, SCREEN_WIDTH * sizeof(uint32_t));
	SDL_RenderClear(sdl->renderer);
	SDL_RenderTexture(sdl->renderer, sdl->texture, NULL, NULL);
	SDL_RenderPresent(sdl->renderer);
}

void sdl_update_audio(SdlContext *sdl, uint8_t sound_timer) {
	const int samples_per_frame = AUDIO_SAMPLE_RATE / TIMER_CLOCK_HZ;
	const int period            = AUDIO_SAMPLE_RATE / AUDIO_BEEP_HZ;
	int16_t   buffer[samples_per_frame];

	if (sound_timer > 0) {
		int queued_samples = SDL_GetAudioStreamAvailable(sdl->audio_stream) / (int)sizeof(int16_t);

		if (queued_samples < samples_per_frame * 3) {
			for (int i = 0; i < samples_per_frame; i++) {
				float phase = (float)(sdl->audio_sample_pos % period) / (float)period;
				buffer[i]   = (phase < 0.5f) ? AUDIO_VOLUME : -AUDIO_VOLUME;
				sdl->audio_sample_pos++;
			}

			SDL_PutAudioStreamData(sdl->audio_stream, buffer, samples_per_frame * (int)sizeof(int16_t));
		}

	}

	else if (sdl->audio_sample_pos > 0) {
		SDL_ClearAudioStream(sdl->audio_stream);
		sdl->audio_sample_pos = 0;
	}
}

void sdl_handle_events(SdlContext *sdl, uint8_t keypad[16]) {
	sdl->save_requested  = false;
	sdl->load_requested  = false;
	sdl->pause_requested = false;
	sdl->reset_requested = false;
	sdl->speed_delta     = 0;

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) sdl->running = false;

		if (event.type == SDL_EVENT_KEY_DOWN) {
			switch (event.key.key) {
			case SDLK_F5:     sdl->save_requested  = true; break;
			case SDLK_F9:     sdl->load_requested  = true; break;
			case SDLK_P:      sdl->pause_requested = true; break;
			case SDLK_F1:     sdl->reset_requested = true; break;
			case SDLK_EQUALS: sdl->speed_delta      = +1;  break;
			case SDLK_MINUS:  sdl->speed_delta      = -1;  break;
			default: break;
			}
		}

		if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
			int key = -1;
			switch (event.key.key) {
			case SDLK_1: key = 0x1; break;
			case SDLK_2: key = 0x2; break;
			case SDLK_3: key = 0x3; break;
			case SDLK_4: key = 0xC; break;
			case SDLK_Q: key = 0x4; break;
			case SDLK_W: key = 0x5; break;
			case SDLK_E: key = 0x6; break;
			case SDLK_R: key = 0xD; break;
			case SDLK_A: key = 0x7; break;
			case SDLK_S: key = 0x8; break;
			case SDLK_D: key = 0x9; break;
			case SDLK_F: key = 0xE; break;
			case SDLK_Z: key = 0xA; break;
			case SDLK_X: key = 0x0; break;
			case SDLK_C: key = 0xB; break;
			case SDLK_V: key = 0xF; break;
			default: break;
			}

			if (key != -1) keypad[key] = (event.type == SDL_EVENT_KEY_DOWN);
		}
	}
}

void sdl_cleanup(SdlContext *sdl) {
	if (sdl->audio_stream) SDL_DestroyAudioStream(sdl->audio_stream);
	if (sdl->texture)      SDL_DestroyTexture(sdl->texture);
	if (sdl->renderer)     SDL_DestroyRenderer(sdl->renderer);
	if (sdl->window)       SDL_DestroyWindow(sdl->window);
	SDL_Quit();
}
