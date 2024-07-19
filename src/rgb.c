/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#include "types.h"

void rgb4torgb8(UWORD rgb4, char *r, char *g, char *b) {
	char *crgb4 = (char *) &rgb4;
	*r = crgb4[1] << 4;
	*g = crgb4[0];
	*b = crgb4[0] << 4;
}
