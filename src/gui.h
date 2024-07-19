/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

int  gui_new(void);
int  gui_open(void);
void gui_close(void);
void gui_delete(void);
void gui_mainloop(void);
void gui_setimage(char *, struct BitMap *, UWORD *, int);
void gui_redraw(void);
Display *gui_getDisplay(void);
Visual *gui_getVisual(void);
int gui_getScreen(void);
