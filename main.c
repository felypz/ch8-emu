#include <err.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#ifdef __OpenBSD__
#include <unistd.h>
#endif

#include "ch8.h"
#include "config.h"
#include "sdl.h"

#define SAVESTATE_FILE "ch8.sav"
#define SPEED_STEP     50
#define SPEED_MIN      100
#define SPEED_MAX      2000

static volatile sig_atomic_t quit_requested = 0;

static void handle_sigint(int sig) {
	(void)sig;
	quit_requested = 1;
}

static void usage(const char *prog) {
	fprintf(stderr,
		"usage: %s [options] <rom.ch8>\n"
		"\n"
		"options:\n"
		"  -c, --color  <n>    default, blue, matrix, warm\n"
		"  -h, --help\n"
		"  -q, --quirks <mode> original (default) or modern\n"
		"  -v, --version\n"
		"\n"
		"keys:\n"
		"  F1        reset\n"
		"  F5        save state\n"
		"  F9        load state\n"
		"  P         pause / resume\n"
		"  +/-       increase / decrease speed\n",
		prog);
}

static void apply_quirks(Chip8State *cpu, const char *mode) {
	if (strcmp(mode, "modern") == 0) {
		cpu->quirk_shift      = true;
		cpu->quirk_load_store = true;
		cpu->quirk_jump       = false;
	}

	else if (strcmp(mode, "original") == 0) {
		cpu->quirk_shift      = false;
		cpu->quirk_load_store = false;
		cpu->quirk_jump       = false;
	}

	else {
		errx(EX_USAGE, "unknown quirks mode '%s' (use: original, modern)", mode);
	}
}

int main(int argc, char *argv[]) {
	struct sigaction sa = { .sa_handler = handle_sigint };
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	Chip8State cpu;
	SdlContext sdl;

	const char *color_name = NULL;
	const char *quirks     = NULL;
	const char *rom        = NULL;

	static struct option long_opts[] = {
		{ "color",  required_argument, NULL, 'c' },
		{ "help",   no_argument,       NULL, 'h' },
		{ "quirks", required_argument, NULL, 'q' },
		{ "version", no_argument,       NULL, 'v' },
		{ NULL, 0, NULL, 0 }
	};

	int opt;
	while ((opt = getopt_long(argc, argv, "q:c:h", long_opts, NULL)) != -1) {
		switch (opt) {
		case 'c': color_name = optarg; break;
		case 'h': usage(argv[0]); return EX_OK;
		case 'q': quirks     = optarg; break;
		case 'v': fprintf(stdout, "ch8 %s\n", VERSION); return EX_OK;
		default:  usage(argv[0]); return EX_USAGE;
		}
	}

	if (optind >= argc) {
		warnx("no ROM file specified");
		usage(argv[0]);
		return EX_USAGE;
	}

	rom = argv[optind];

	chip8_init(&cpu);

	if (quirks) apply_quirks(&cpu, quirks);

	Colorscheme colors = get_colorscheme(color_name);

	if (!chip8_load_rom(&cpu, rom)) return EX_NOINPUT;
	if (!sdl_init(&sdl)) return EX_UNAVAILABLE;

#ifdef __OpenBSD__
	if (pledge("stdio audio", NULL) == -1)
		err(EX_OSERR, "pledge");
#endif

	int      cpu_clock_hz      = CPU_CLOCK_HZ;
	uint64_t ns_per_cycle      = 1000000000ULL / (uint64_t)cpu_clock_hz;
	uint64_t ns_per_timer      = 1000000000ULL / TIMER_CLOCK_HZ;
	uint64_t cycle_accumulator = 0;
	uint64_t timer_accumulator = 0;
	uint64_t last_time_ns      = SDL_GetTicksNS();

	const int max_cycles_per_tick = 64;

	Chip8State rom_snapshot = cpu;
	chip8_save_state(&cpu, SAVESTATE_FILE);

	while (sdl.running && cpu.running && !quit_requested) {
		sdl_handle_events(&sdl, cpu.keypad);

		if (sdl.reset_requested) {
			cpu               = rom_snapshot;
			cycle_accumulator = 0;
			timer_accumulator = 0;
			last_time_ns      = SDL_GetTicksNS();
		}

		if (sdl.pause_requested) cpu.paused = !cpu.paused;

		if (sdl.save_requested) {
			if (chip8_save_state(&cpu, SAVESTATE_FILE))
				fprintf(stderr, "state saved\n");
		}

		if (sdl.load_requested) {
			if (chip8_load_state(&cpu, SAVESTATE_FILE))
				fprintf(stderr, "state loaded\n");
		}

		if (sdl.speed_delta != 0) {
			cpu_clock_hz += sdl.speed_delta * SPEED_STEP;
			if (cpu_clock_hz < SPEED_MIN) cpu_clock_hz = SPEED_MIN;
			if (cpu_clock_hz > SPEED_MAX) cpu_clock_hz = SPEED_MAX;
			ns_per_cycle = 1000000000ULL / (uint64_t)cpu_clock_hz;
			fprintf(stderr, "speed: %d Hz\n", cpu_clock_hz);
		}

		uint64_t current_ns = SDL_GetTicksNS();
		uint64_t delta_ns   = current_ns - last_time_ns;
		last_time_ns        = current_ns;

		if (!cpu.paused) {
			cycle_accumulator += delta_ns;
			timer_accumulator += delta_ns;

			int cycles_this_frame = 0;
			while (cycle_accumulator >= ns_per_cycle && cycles_this_frame < max_cycles_per_tick) {
				chip8_cycle(&cpu);
				cycle_accumulator -= ns_per_cycle;
				cycles_this_frame++;
			}

			if (cycles_this_frame == max_cycles_per_tick) cycle_accumulator = 0;

			while (timer_accumulator >= ns_per_timer) {
				chip8_update_timers(&cpu);
				sdl_update_audio(&sdl, cpu.sound_timer);
				timer_accumulator -= ns_per_timer;
			}
		}

		if (cpu.needs_draw) {
			sdl_update_display(&sdl, cpu.framebuffer, colors);
			cpu.needs_draw = false;
		}

		SDL_DelayNS(500000);
	}

	sdl_cleanup(&sdl);
	return EX_OK;
}
