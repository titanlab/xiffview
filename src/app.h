/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#define APP_NAME          "xiffview"
//#define APP_VERSION	       0.1
//Sun Apr 25 05:22:46 PM CEST 2021
//#define APP_VERSION	       0.2
//Mon Jun 14 07:26:46 PM CEST 2021
//#define APP_VERSION	       0.3
//Mon Jun 21 06:41:07 PM CEST 2021
//#define APP_VERSION	       0.4
//Thu Oct  7 07:08:24 PM CEST 2021
#define APP_VERSION	       0.5
#define APP_STRING        "xiffview v0.6 by r. eberle 2021 / APL, neoman 2024 / TITAN"
#define APP_STRING_SHORT  "xiffview"

#define APP_FILENAME_MAXLEN     256
#define APP_CMD_STRING_MAXLEN   512

struct appdata {
	char filename[APP_FILENAME_MAXLEN];
	char filename_output[APP_FILENAME_MAXLEN];
	char cmdstring[APP_CMD_STRING_MAXLEN];

	// true/false for each bitplane (just a big array, must be at least IFF_ILBM_BITPLANES_MAX, see there)
	char gui_image_bitplane_enable[16];
	// pixel scaling
	char gui_image_scale;
    // set 12bpp for OCS/ECS mode
    char gui_image_12bpp_mode;
    // fix flickering on interlaced modes
    char gui_image_fix_flicker;
};
