#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ch8.h"
#include "config.h"
#include "sdl.h"

int main(int argc, char *argv[]) {
	Chip8State cpu;
	SdlContext sdl;
  uint32_t color_foreground = 0xFFFFFFFF;
  uint32_t color_background = 0x00000000;
  uint32_t colors[] = {
    0xFF0000FF,  // red
    0xFFFF0000,  // blue
    0xFFFFFFFF,  // white
    0x00000000  // black
};

	if (argc < 2) {
		printf("usage: %s <rom.ch8> [modern|original] --color=red|blue|white|black\n", argv[0]);
		return 1;
	}

	chip8_init(&cpu);

	if (!chip8_load_rom(&cpu, argv[1])) {
		printf("error: failed to open ROM file '%s'\n", argv[1]);
		return 1;
	}

	if (argc >= 3 && strcmp(argv[2], "modern") == 0) {
		cpu.quirk_shift      = true;
		cpu.quirk_load_store = true;
		cpu.quirk_jump       = false;
	}

	else {
		cpu.quirk_shift      = false;
		cpu.quirk_load_store = false;
		cpu.quirk_jump       = false;
	}
if (argc >= 4) {
  if (strcmp(argv[3], "--color=red") == 0) {
    color_foreground = colors[0]; 
  }
  else if (strcmp(argv[3], "--color=blue") == 0) {
    color_foreground = colors[1]; 
  }
  else if (strcmp(argv[3], "--color=white") == 0) {
    color_foreground = colors[2]; 
  }
  else if (strcmp(argv[3], "--color=black") == 0) {
    color_foreground = colors[3];  
  }
} else {
}

	if (!sdl_init(&sdl)) {
		printf("error: failed to initialize SDL\n");
		return 1;
	}

	const double ms_per_cycle = 1000.0 / CPU_CLOCK_HZ;
	const double ms_per_timer = 1000.0 / TIMER_CLOCK_HZ;

	double cycle_accumulator = 0.0;
	double timer_accumulator = 0.0;

	uint64_t last_time_ms = SDL_GetTicks();

	while (sdl.running && cpu.running) {
		sdl_handle_events(&sdl, cpu.keypad);

		uint64_t current_time = SDL_GetTicks();

		double delta_ms = (double)(current_time - last_time_ms);
		last_time_ms = current_time;

		cycle_accumulator += delta_ms;
		timer_accumulator += delta_ms;

		int cycles_this_frame = 0;
		while (cycle_accumulator >= ms_per_cycle && cycles_this_frame < MAX_CYCLES_PER_TICK) {
			chip8_cycle(&cpu);
			cycle_accumulator -= ms_per_cycle;
			cycles_this_frame++;
		}

		while (timer_accumulator >= ms_per_timer) {
			chip8_update_timers(&cpu);
			sdl_update_audio(&sdl, cpu.sound_timer);
			timer_accumulator -= ms_per_timer;
		}

		if (cpu.needs_draw) {
			sdl_update_display(&sdl, cpu.framebuffer,color_foreground,color_background);
			cpu.needs_draw = false;
		}

		SDL_Delay(1);
	}

	sdl_cleanup(&sdl);
	return 0;
}
