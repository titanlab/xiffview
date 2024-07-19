/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

#include <stdio.h>

int userconfirm(char *fmtstr, char *str) {
	int ret = 0;
	if (fmtstr) {
		if (str) {
			printf(fmtstr, str);
		} else {
			printf(fmtstr);
		}
		printf(" (enter y to confirm) ");
		char c = getchar();
		if (c == 'y') {
			ret = 1;
			//printf("ok");
		} else {
			//printf("cancelled");
		}
	}
	return ret;
}

void usermsg(char *fmtstr, char *str) {
	if (fmtstr) {
		if (str) {
			printf(fmtstr, str);
		} else {
			printf(fmtstr);
		}
	}
	printf("\n");
}

void usererr(char *fmtstr, char *str) {
	printf("ERROR: ");
	usermsg(fmtstr, str);
}
