/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "usermsg.h"
#include "app.h"
#include "types.h"
#include "iff_load_ilbm.h"
#include "cmd.h"

char cmd_string[APP_CMD_STRING_MAXLEN];  // the entire command string (from CLI args)
char cmd_command[APP_CMD_STRING_MAXLEN]; // the command portion of command string
char cmd_delimiters[16];
int cmd_args_int[16];

#define CMD_DELIMITERS_STRING                  ":,"
#define CMD_COMMAND_TO_SPRITE_FILENAME_DEFAULT "sprite.c"
#define CMD_COMMAND_TO_SPRITE_WORDS_PER_LINE   4

struct {
	struct appdata      *appdata;
	struct BitMap       *img_bm;
	ULONG               *img_colormap;
	BitMapHeader        *iff_bmh;
} cmd_data;

int cmd_exec_to_sprite(int args[]) {
	int ret = 1;
	char *bitplane_mem_a;
	char *bitplane_mem_b;
	unsigned long offset;
	int i, j, x, y, height, bitplane_a, bitplane_b, pen1, pen2, pen3, penmax, per_line;
	int pen[3];
	char str_pen[2];
	x = args[0];
	y = args[1];
	height = args[2];
	bitplane_a = args[3];
	bitplane_b = args[4];
	pen[0] = args[5];
	pen[1] = args[6];
	pen[2] = args[7];
	penmax = (1 << cmd_data.img_bm->Depth) - 1;

	debugmsg("x,y: %d,%d - height: %d - pens: %d, %d, %d", x, y, height, pen[0], pen[1], pen[2]);
	//debugmsg("img_bm->Depth: %d, colors: %d", cmd_data.img_bm->Depth, 1 << cmd_data.img_bm->Depth);
	//debugmsg("penmax: %d", penmax);

	if (!strlen(cmd_data.appdata->filename_output)) {
		debugmsg("using to-sprite default filename: %s", CMD_COMMAND_TO_SPRITE_FILENAME_DEFAULT);
		strcpy(cmd_data.appdata->filename_output, CMD_COMMAND_TO_SPRITE_FILENAME_DEFAULT);
	}

	if (height < 1 || height > 256) {
		ret = 0; usererr("height (number of lines) out of bounds", NULL);
	}
	if (x % 8) {
		ret = 0; usererr("x coordinate must be multiple of 8", NULL);
	}
	if (x < 0 || y < 0) {
		ret = 0; usererr("x and y coordinates must be non-negative", NULL);
	}
	if (x > ((cmd_data.img_bm->BytesPerRow * 8) - 16)) {
		ret = 0; usererr("x coordinate out of bounds, or doesn't leave 16 pixels for a sprite", NULL);
	}
	if (y > ((cmd_data.img_bm->Rows - 1) - height)) {
		ret = 0; usererr("y coordinate out of bounds, or doesn't leave enough lines for sprite height", NULL);
	}
	if (bitplane_a < 0 || bitplane_b < 0) {
		ret = 0; usererr("bitplane numbers a and b must be non-negative", NULL);
	}
	if (bitplane_a > cmd_data.img_bm->Depth - 1) {
		ret = 0; usererr("bitplane number a out of bounds", NULL);
	}
	if (bitplane_b > cmd_data.img_bm->Depth - 1) {
		ret = 0; usererr("bitplane number b out of bounds", NULL);
	}
	for (i = 0; i < 3; i++) {
		if (pen[i] < 0 || pen[i] > penmax) {
			sprintf(str_pen, "%d", i+1);
			ret = 0; usererr("pen index %s out of bounds", str_pen);
		}
	}

	if (ret) {
		offset = y * cmd_data.img_bm->BytesPerRow + x/8;

		FILE *f = fopen(cmd_data.appdata->filename_output, "r");
		if (f) {
			if (!userconfirm("output file exists - overwrite?", NULL)) {
				usermsg("cancelled, exiting.", NULL);
				ret = 0;
			}
			fclose(f);
		}

		if (ret) {
			f = fopen(cmd_data.appdata->filename_output, "w");
			if (f) {

				// put our two (a/b) bitplane numbers into array for loop below:
				//int bitplane[2]; bitplane[0] = bitplane_a; bitplane[1] = bitplane_b;

				// size of WORD (not BYTE) array is:
				// (number of lines (1 line = 1 WORD = 16 pixels) * number of bitplanes) + 2 for termination
				fprintf(f, "WORD __chip sprite_xSPRITENAMEx_bitplanes[%d+2] = {\n", height * 2);

				bitplane_mem_a = (char *) cmd_data.img_bm->Planes[bitplane_a] + offset;
				bitplane_mem_b = (char *) cmd_data.img_bm->Planes[bitplane_b] + offset;
				for (i = 0; i < height; i++) {
					fprintf(f, "\t");

					// this does not give the desired results (byte alignment wrong)
					//fprintf(f, "0x%04x, ", (UWORD) *bitplane_mem_a);
					//fprintf(f, "0x%04x, ", (UWORD) *bitplane_mem_b);

					// this does:
					fprintf(f, "0x%02x", (unsigned char) *bitplane_mem_a);
					fprintf(f, "%02x, ", (unsigned char) *(bitplane_mem_a+1));
					fprintf(f, "0x%02x", (unsigned char) *bitplane_mem_b);
					fprintf(f, "%02x, ", (unsigned char) *(bitplane_mem_b+1));

					fprintf(f, "\n");
					bitplane_mem_a += cmd_data.img_bm->BytesPerRow;
					bitplane_mem_b += cmd_data.img_bm->BytesPerRow;
				}
				fprintf(f, "\t0, 0\n");
				fprintf(f, "};\n\n");

				debugmsg("...writing sprite colors...");
				fprintf(f, "WORD sprite_xSPRITENAMEx_colors[3] = {\n");
				fprintf(f, "\t");
				i = 0;
				for (i = 0; i < 3; i++) {
                    ULONG rgb;
                    UWORD rgb4;

					if (i)
						fprintf(f, ", ");

                    rgb = cmd_data.img_colormap[pen[i]];
                    rgb4 = ((rgb >> 12) & 0xf00) | ((rgb >> 8) & 0xf0) | ((rgb >> 4) & 0xf);

					fprintf(f, "0x%04x", rgb4);
				}
				fprintf(f, "\n};\n\n");

				debugmsg("...writing sprite struct...");
				fprintf(f, "NEWVSPRITE sprite_xSPRITENAMEx_newvsprite = {\n");
				fprintf(f, "\t// bitplane data (height * 2 WORDs), sprite_colors (3 WORDs), 1 = WORD width (for true VSprite)\n");
				fprintf(f, "\tsprite_xSPRITENAMEx_bitplanes, sprite_xSPRITENAMEx_colors, 1,\n");
				fprintf(f, "\t// height (lines), depth = 2 (for true VSprite), x pos, y pos\n");
				fprintf(f, "\t%d, 2, 0, 0,\n", height);
				fprintf(f, "\t// flags, hitmask, me-mask\n");
				fprintf(f, "\tVSPRITE, 0, 0\n", height);
				fprintf(f, "};\n");

				fclose(f);
			} else {
				usererr("could not open output file for writing", NULL);
				ret = 0;
			}
		}
	}

	return ret;
}

// appdata->cmdstring will be copied to (global) cmd_string - must fit memory!
int cmd_exec(struct appdata *ad, struct BitMap *img_bm, ULONG *img_colormap, BitMapHeader *iff_bmh) {
	int ret = 0;
	char *tok;
	int i;

	cmd_data.appdata      = ad;
	cmd_data.img_bm       = img_bm;
	cmd_data.img_colormap = img_colormap;
	cmd_data.iff_bmh      = iff_bmh;

	sprintf(cmd_delimiters, CMD_DELIMITERS_STRING);

	strcpy(cmd_string, ad->cmdstring);
	debugmsg("cmd_string: %s", cmd_string);

	debugmsg("strtok...");
	tok = strtok(cmd_string, cmd_delimiters);
	if (tok) {
		strcpy(cmd_command, tok);
		debugmsg("cmd_command: %s", cmd_command);
	}
	if (!strcmp(cmd_command, "to-sprite")) {
		debugmsg("cmd: to-sprite");
		i = 0;
		while (i < 8 && (tok = strtok(NULL, cmd_delimiters))) {
			cmd_args_int[i] = atoi(tok);
			i++;
		}
		if (i < 8) {
			usererr("too few arguments for command \"%s\"", cmd_command);
		} else if (strtok(NULL, cmd_delimiters)) {
			usererr("too many arguments for command \"%s\"", cmd_command);
		} else {
			// looks good -> execute!
			ret = cmd_exec_to_sprite(cmd_args_int);
		}
	} else {
		usererr("command syntax error", NULL);
	}
	return ret;
}
