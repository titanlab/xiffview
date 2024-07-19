/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#include <string.h>

void bytereverse(char *mem, int size) {
	char tmpmem[16];
	int tmpsize = size;
	char *c = tmpmem;
	while(tmpsize) {
		tmpsize--;
		*c = mem[tmpsize];
		c++;
	}
	memcpy(mem, tmpmem, size);
}
