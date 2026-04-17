#ifndef CHIP8_CONFIG_H
#define CHIP8_CONFIG_H

#include <stdint.h>
#include <string.h>

#define SCREEN_WIDTH  64
#define SCREEN_HEIGHT 32

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BEEP_HZ     440
#define AUDIO_VOLUME      3000

#define CPU_CLOCK_HZ        500
#define TIMER_CLOCK_HZ      60
#define MAX_CYCLES_PER_TICK 10

#define MEMORY_SIZE 4096

#define COLOR_FG_DEFAULT 0x00FFFFFF
#define COLOR_BG_DEFAULT 0x00000000
#define COLOR_FG_BLUE    0x00FFFFFF
#define COLOR_BG_BLUE    0x000000AA
#define COLOR_FG_MATRIX  0x0000FF00
#define COLOR_BG_MATRIX  0x00000000
#define COLOR_FG_WARM    0x00FF8000
#define COLOR_BG_WARM    0x00402000

typedef struct {
	uint32_t fg;
	uint32_t bg;
} Colorscheme;

Colorscheme get_colorscheme(const char *name);

#endif
