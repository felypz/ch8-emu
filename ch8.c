#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __OpenBSD__
#include <stdlib.h>
#else
#include <stdlib.h>
#include <time.h>
#endif

#include "ch8.h"

static unsigned char fontset[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,
	0x20, 0x60, 0x20, 0x20, 0x70,
	0xF0, 0x10, 0xF0, 0x80, 0xF0,
	0xF0, 0x10, 0xF0, 0x10, 0xF0,
	0x90, 0x90, 0xF0, 0x10, 0x10,
	0xF0, 0x80, 0xF0, 0x10, 0xF0,
	0xF0, 0x80, 0xF0, 0x90, 0xF0,
	0xF0, 0x10, 0x20, 0x40, 0x40,
	0xF0, 0x90, 0xF0, 0x90, 0xF0,
	0xF0, 0x90, 0xF0, 0x10, 0xF0,
	0xF0, 0x90, 0xF0, 0x90, 0x90,
	0xE0, 0x90, 0xE0, 0x90, 0xE0,
	0xF0, 0x80, 0x80, 0x80, 0xF0,
	0xE0, 0x90, 0x90, 0x90, 0xE0,
	0xF0, 0x80, 0xF0, 0x80, 0xF0,
	0xF0, 0x80, 0xF0, 0x80, 0x80
};

static uint32_t chip8_rand(void) {
#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__APPLE__)
	return arc4random();
#else
	return (uint32_t)rand();
#endif
}

void chip8_init(Chip8State *cpu) {
	memset(cpu, 0, sizeof(Chip8State));

	cpu->program_counter = 0x200;
	cpu->running         = true;
	cpu->paused          = false;

	for (int i = 0; i < 80; i++) cpu->memory[0x50 + i] = fontset[i];

#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__APPLE__)
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	srand((unsigned)(ts.tv_sec ^ ts.tv_nsec));
#endif
}

bool chip8_load_rom(Chip8State *cpu, const char *filename) {
	int fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror(filename);
		return false;
	}

	struct stat st;
	if (fstat(fd, &st) < 0) {
		perror(filename);
		close(fd);
		return false;
	}

	off_t size = st.st_size;
	if (size > MEMORY_SIZE - 0x200) {
		fprintf(stderr, "error: ROM too large (%lld bytes, max %d bytes)\n",
		        (long long)size, MEMORY_SIZE - 0x200);
		close(fd);
		return false;
	}

	ssize_t bytes_read = read(fd, &cpu->memory[0x200], (size_t)size);
	close(fd);

	if (bytes_read != (ssize_t)size) {
		perror(filename);
		return false;
	}

	return true;
}

bool chip8_save_state(const Chip8State *cpu, const char *filename) {
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		perror(filename);
		return false;
	}

	ssize_t written = write(fd, cpu, sizeof(Chip8State));
	close(fd);

	if (written != (ssize_t)sizeof(Chip8State)) {
		perror(filename);
		return false;
	}

	return true;
}

bool chip8_load_state(Chip8State *cpu, const char *filename) {
	int fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror(filename);
		return false;
	}

	ssize_t bytes_read = read(fd, cpu, sizeof(Chip8State));
	close(fd);

	if (bytes_read != (ssize_t)sizeof(Chip8State)) {
		perror(filename);
		return false;
	}

	return true;
}

void chip8_update_timers(Chip8State *cpu) {
	if (cpu->delay_timer > 0) cpu->delay_timer--;
	if (cpu->sound_timer > 0) cpu->sound_timer--;
}

void chip8_cycle(Chip8State *cpu) {
	if (cpu->program_counter >= MEMORY_SIZE - 1) {
		fprintf(stderr, "error: PC out of bounds (0x%04X)\n", cpu->program_counter);
		cpu->running = false;
		return;
	}

	uint16_t opcode = (cpu->memory[cpu->program_counter] << 8) | cpu->memory[cpu->program_counter + 1];
	cpu->program_counter += 2;

	uint8_t  vx     = (opcode & 0x0F00) >> 8;
	uint8_t  vy     = (opcode & 0x00F0) >> 4;
	uint8_t  nibble = opcode & 0x000F;
	uint8_t  byte   = opcode & 0x00FF;
	uint16_t addr   = opcode & 0x0FFF;

	switch (opcode & 0xF000) {
	case 0x0000:
		if (opcode == 0x00E0) {
			memset(cpu->framebuffer, 0, sizeof(cpu->framebuffer));
			cpu->needs_draw = true;
		}

		else if (opcode == 0x00EE) {
			if (cpu->stack_pointer == 0) {
				fprintf(stderr, "error: stack underflow at PC=0x%04X opcode=0x%04X\n",
				        cpu->program_counter - 2, opcode);
				cpu->running = false;
				break;
			}
			
			cpu->program_counter = cpu->stack[--cpu->stack_pointer];
		}
		
		break;

	case 0x1000:
		cpu->program_counter = addr;
		break;

	case 0x2000:
		if (cpu->stack_pointer >= 16) {
			fprintf(stderr, "error: stack overflow at PC=0x%04X opcode=0x%04X (SP=%d)\n",
			        cpu->program_counter - 2, opcode, cpu->stack_pointer);
			cpu->running = false;
			break;
		}

		cpu->stack[cpu->stack_pointer++] = cpu->program_counter;
		cpu->program_counter = addr;
		break;

	case 0x3000:
		if (cpu->reg[vx] == byte) cpu->program_counter += 2;
		break;

	case 0x4000:
		if (cpu->reg[vx] != byte) cpu->program_counter += 2;
		break;

	case 0x5000:
		if (cpu->reg[vx] == cpu->reg[vy]) cpu->program_counter += 2;
		break;

	case 0x6000:
		cpu->reg[vx] = byte;
		break;

	case 0x7000:
		cpu->reg[vx] += byte;
		break;

	case 0x8000:
		switch (nibble) {
		case 0x0:
			cpu->reg[vx] = cpu->reg[vy];
			break;

		case 0x1:
			cpu->reg[vx] |= cpu->reg[vy];
			cpu->reg[0xF] = 0;
			break;

		case 0x2:
			cpu->reg[vx] &= cpu->reg[vy];
			cpu->reg[0xF] = 0;
			break;

		case 0x3:
			cpu->reg[vx] ^= cpu->reg[vy];
			cpu->reg[0xF] = 0;
			break;

		case 0x4: {
			uint16_t result = cpu->reg[vx] + cpu->reg[vy];
			cpu->reg[vx]    = result & 0xFF;

			if (result > 0xFF) {
				cpu->reg[0xF] = 1;
			}

			else {
				cpu->reg[0xF] = 0;
			}

			break;
		}

		case 0x5: {
			uint8_t flag;

			if (cpu->reg[vx] > cpu->reg[vy]) {
				flag = 1;
			}

			else {
				flag = 0;
			}

			cpu->reg[vx] -= cpu->reg[vy];
			cpu->reg[0xF] = flag;
			break;
		}

		case 0x6: {
			if (!cpu->quirk_shift) cpu->reg[vx] = cpu->reg[vy];

			uint8_t flag  = cpu->reg[vx] & 0x01;

			cpu->reg[vx] >>= 1;
			cpu->reg[0xF]  = flag;
			break;
		}

		case 0x7: {
			uint8_t flag;

			if (cpu->reg[vy] > cpu->reg[vx]) {
				flag = 1;
			}

			else {
				flag = 0;
			}

			cpu->reg[vx]  = cpu->reg[vy] - cpu->reg[vx];
			cpu->reg[0xF] = flag;
			break;
		}

		case 0xE: {
			if (!cpu->quirk_shift) cpu->reg[vx] = cpu->reg[vy];
			uint8_t flag  = (cpu->reg[vx] & 0x80) >> 7;
			cpu->reg[vx] <<= 1;
			cpu->reg[0xF]  = flag;
			break;
		}

		default:
			fprintf(stderr, "error: unknown opcode 0x%04X at PC=0x%04X\n",
			        opcode, cpu->program_counter - 2);
			break;
		}
		
		break;

	case 0x9000:
		if (cpu->reg[vx] != cpu->reg[vy]) cpu->program_counter += 2;
		break;

	case 0xA000:
		cpu->index_reg = addr;
		break;

	case 0xB000:
		if (cpu->quirk_jump) {
			cpu->program_counter = addr + cpu->reg[vx];
		}

		else {
			cpu->program_counter = addr + cpu->reg[0];
		}

		break;

	case 0xC000:
		cpu->reg[vx] = (chip8_rand() % 256) & byte;
		break;

	case 0xD000: {
		uint8_t x_pos  = cpu->reg[vx] % 64;
		uint8_t y_pos  = cpu->reg[vy] % 32;
		uint8_t height = nibble;

		cpu->reg[0xF]  = 0;

		for (uint8_t row = 0; row < height; row++) {
			uint8_t sprite = cpu->memory[cpu->index_reg + row];
			for (uint8_t col = 0; col < 8; col++) {
				if ((sprite & (0x80 >> col)) != 0) {
					int wx    = (x_pos + col) % 64;
					int wy    = (y_pos + row) % 32;
					int index = wx + (wy * 64);

					if (cpu->framebuffer[index] == 1) cpu->reg[0xF] = 1;

					cpu->framebuffer[index] ^= 1;
				}
			}
		}
	
		cpu->needs_draw = true;
		break;
	}

	case 0xE000:
		if (byte == 0x9E) {
			if (cpu->keypad[cpu->reg[vx]]) cpu->program_counter += 2;
		}

		else if (byte == 0xA1) {
			if (!cpu->keypad[cpu->reg[vx]]) cpu->program_counter += 2;
		}

		else {
			fprintf(stderr, "error: unknown opcode 0x%04X at PC=0x%04X\n",
			        opcode, cpu->program_counter - 2);
		}
		
		break;

	case 0xF000:
		switch (byte) {
		case 0x07:
			cpu->reg[vx] = cpu->delay_timer;
			break;

		case 0x0A:
			if (!cpu->waiting_key) {
				for (int i = 0; i < 16; i++) {
					if (cpu->keypad[i]) {
						cpu->waiting_key     = true;
						cpu->waiting_key_reg = vx;
						cpu->waiting_key_id  = i;
						break;
					}
				}
				
				cpu->program_counter -= 2;
			}

			else {
				if (!cpu->keypad[cpu->waiting_key_id]) {
					cpu->reg[cpu->waiting_key_reg] = cpu->waiting_key_id;
					cpu->waiting_key = false;
				}

				else {
					cpu->program_counter -= 2;
				}
			}

			break;

		case 0x15:
			cpu->delay_timer = cpu->reg[vx];
			break;

		case 0x18:
			cpu->sound_timer = cpu->reg[vx];
			break;

		case 0x1E:
			cpu->index_reg += cpu->reg[vx];
			break;

		case 0x29:
			cpu->index_reg = 0x50 + ((cpu->reg[vx] & 0x0F) * 5);
			break;

		case 0x33:
			if (cpu->index_reg + 2 >= MEMORY_SIZE) {
				fprintf(stderr, "error: Fx33 out of bounds at PC=0x%04X\n",
				        cpu->program_counter - 2);
				cpu->running = false;
				break;
			}

			cpu->memory[cpu->index_reg]     = cpu->reg[vx] / 100;
			cpu->memory[cpu->index_reg + 1] = (cpu->reg[vx] / 10) % 10;
			cpu->memory[cpu->index_reg + 2] = cpu->reg[vx] % 10;
			break;

		case 0x55:
			if (cpu->index_reg + vx >= MEMORY_SIZE) {
				fprintf(stderr, "error: Fx55 out of bounds at PC=0x%04X\n",
				        cpu->program_counter - 2);
				cpu->running = false;
				break;
			}

			for (int i = 0; i <= vx; i++) cpu->memory[cpu->index_reg + i] = cpu->reg[i];
			if (!cpu->quirk_load_store) cpu->index_reg += vx + 1;
			break;

		case 0x65:
			if (cpu->index_reg + vx >= MEMORY_SIZE) {
				fprintf(stderr, "error: Fx65 out of bounds at PC=0x%04X\n",
				        cpu->program_counter - 2);
				cpu->running = false;
				break;
			}

			for (int i = 0; i <= vx; i++) cpu->reg[i] = cpu->memory[cpu->index_reg + i];
			if (!cpu->quirk_load_store) cpu->index_reg += vx + 1;
			break;

		default:
			fprintf(stderr, "error: unknown opcode 0x%04X at PC=0x%04X\n",
			        opcode, cpu->program_counter - 2);
			break;
		}

		break;
	}
}
