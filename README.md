# ch8-emu

ch8-emu is a side/hobby project — a CHIP-8 interpreter/emulator for UNIX systems written in C.

CHIP-8 is a virtual machine and interpreted programming language developed by Joseph Weisbecker in 1977, originally designed to simplify game development on early microcomputers.

## Requirements

- A C compiler (GCC, or Clang on Apple systems)
- make
- SDL3

## Installation

**GNU/Linux**

First, install the dependencies with your distribution's package manager. Here are some examples for commonly used distros:

Ubuntu/Debian
```bash
$ sudo apt install gcc make libsdl3-dev
```

Fedora
```bash
$ sudo dnf install gcc make sdl3-devel
```

Arch
```bash
$ sudo pacman -S gcc make sdl3
```

After installing the dependencies, clone the repository and install the binary:
```bash
$ sudo make install
```

Run the emulator:
```bash
$ ch8 [options] <rom.ch8>
```

To uninstall:
```bash
$ sudo make uninstall
```

---

**FreeBSD**

Install the dependencies with root privileges:
```bash
# pkg install gcc make sdl3
```

After installing the dependencies, clone the repository and install the binary:
```bash
# make install
```

Run the emulator:
```bash
$ ch8 [options] <rom.ch8>
```

To uninstall:
```bash
# make uninstall
```

---

**OpenBSD**

Install the dependencies with root privileges:
```bash
# pkg_add gcc make sdl3
```

After installing the dependencies, clone the repository and install the binary:
```bash
# make install
```

Run the emulator:
```bash
$ ch8 [options] <rom.ch8>
```

To uninstall:
```bash
# make uninstall
```

---

**macOS (Apple Silicon / Intel)**

Make sure Homebrew is installed, then install the dependencies:
```bash
$ brew install gcc make sdl3
```

After installing the dependencies, clone the repository and install the binary:
```bash
$ sudo make install
```

Run the emulator:
```bash
$ ch8 [options] <rom.ch8>
```

To uninstall:
```bash
$ sudo make uninstall
```

## Usage

```
usage: ch8 [options] <rom.ch8>

options:
  -q, --quirks <mode>   original (default) or modern
  -c, --color  <name>   default, blue, matrix, warm
  -h, --help
```

### Quirks

CHIP-8 has several behavioral differences between implementations. Two modes are available:

- `original` — matches the behavior of the original COSMAC VIP interpreter (default)
- `modern` — matches the behavior expected by most modern ROMs

### Color schemes

| Name      | Description                     |
|-----------|---------------------------------|
| `default` | White on black                  |
| `blue`    | White on dark blue (CRT-style)  |
| `matrix`  | Green on black                  |
| `warm`    | Orange on dark brown            |

### Keyboard layout

The original CHIP-8 16 keypad is mapped to the following keys:

```
CHIP-8     Keyboard
1 2 3 C    1 2 3 4
4 5 6 D    Q W E R
7 8 9 E    A S D F
A 0 B F    Z X C V
```

## Features

- 4KB RAM
- 16 general-purpose 8-bit registers (V0–VF)
- 16-level call stack
- 64×32 pixel display
- Delay timer and sound timer running at 60Hz
- 16-key keypad input
- Beeper audio output via SDL3
- Configurable CPU clock (default: 500Hz)
- Quirks mode support (`original` and `modern`)
- Multiple color schemes (`default`, `blue`, `matrix`, `warm`)
- Sprite wrapping

## License

See [LICENSE](LICENSE).
