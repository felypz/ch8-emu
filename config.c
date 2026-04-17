#include <string.h>

#include "config.h"

Colorscheme get_colorscheme(const char *name) {
	if (name != NULL) {
		if (strcmp(name, "blue")   == 0) return (Colorscheme){ COLOR_FG_BLUE,   COLOR_BG_BLUE   };
		if (strcmp(name, "matrix") == 0) return (Colorscheme){ COLOR_FG_MATRIX, COLOR_BG_MATRIX };
		if (strcmp(name, "warm")   == 0) return (Colorscheme){ COLOR_FG_WARM,   COLOR_BG_WARM   };
	}

	return (Colorscheme){ COLOR_FG_DEFAULT, COLOR_BG_DEFAULT };
}
