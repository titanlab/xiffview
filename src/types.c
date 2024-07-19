/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#include <stdio.h>

#include "types.h"

void types_check() {
	printf("size of UBYTE    (Amiga: 1 byte) : %d\n", sizeof(UBYTE));
	printf("size of WORD     (Amiga: 2 bytes): %d\n", sizeof(WORD));
	printf("size of UWORD    (Amiga: 2 bytes): %d\n", sizeof(UWORD));
	printf("size of LONG     (Amiga: 4 bytes): %d\n", sizeof(LONG));
	printf("size of ULONG    (Amiga: 4 bytes): %d\n", sizeof(ULONG));
	printf("size of BPTR     (Amiga: 4 bytes): %d\n", sizeof(BPTR));
	printf("size of PLANEPTR (Amiga: 4 bytes): %d\n", sizeof(PLANEPTR));
	printf("size of STRPTR   (Amiga: 4 bytes): %d\n", sizeof(STRPTR));
	printf("size of BOOL     (Amiga: 2 bytes): %d\n", sizeof(BOOL));
}
