#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

typedef struct {
	uint16_t index_reg;
	uint16_t program_counter;
	uint16_t stack[16];

	uint8_t  framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
	uint8_t  memory[MEMORY_SIZE];
	uint8_t  reg[16];
	uint8_t  stack_pointer;
	uint8_t  delay_timer;
	uint8_t  sound_timer;
	uint8_t  keypad[16];
	uint8_t  waiting_key_reg;
	uint8_t  waiting_key_id;

	bool     needs_draw;
	bool     running;
	bool     paused;
	bool     quirk_shift;
	bool     quirk_load_store;
	bool     quirk_jump;
	bool     waiting_key;
} Chip8State;

void chip8_init(Chip8State *cpu);
bool chip8_load_rom(Chip8State *cpu, const char *filename);
void chip8_cycle(Chip8State *cpu);
void chip8_update_timers(Chip8State *cpu);
bool chip8_save_state(const Chip8State *cpu, const char *filename);
bool chip8_load_state(Chip8State *cpu, const char *filename);

#endif
