/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

/*
 *
 * from AmigaOS docs:
 * "If ParseIFF() is called again
 * after returning, it will continue to parse the file where it
 * left off."
 * THIS IS CURRENTLY NOT IMPLEMENTED.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "usermsg.h"
#include "bytereverse.h"
#include "types.h"
#include "iffparse.h"

int iffparse_readbytes(FILE *f, char *mem, int size) {
	//debugmsg("iffparse_readbytes() - f: 0x%016x - mem: 0x%016x - size: %d", mem, size);
	int bytes_read = fread(mem, 1, size, f);
	debugmsg("iffparse_readbytes() - bytes_read: %d", bytes_read);
	/*
	char *c = mem;
	int i;
	for (i = 0; i < bytes_read; i++) {
		debugmsg("%c", *c);
		c++;
	}
	*/
	return bytes_read;
}

// index is index of propchunk that matched
int iffparse_storeproperty(struct IFFHandle *ih, int index, int size) {
	debugmsg("iffparse_storeproperty() - index: %d - size: %d", index, size);
	int ok = 0;
	char *mem = malloc(size);
	if (mem) {
		int bytes_read = iffparse_readbytes((FILE *) ih->iff_Stream, mem, size);
		if (bytes_read == size) {
			ih->storedprop[index].sp_Size = size;
			ih->storedprop[index].sp_Data = mem;
			ok = 1;
		} else {
			free(mem);
		}
	}
	return ok;
}

struct ContextNode iffparse_contextnode_stack[128];

LONG StopChunk(struct IFFHandle *ih, LONG type, LONG id) {
	debugmsg("StopChunk()");
	LONG err = 0;
	ih->stopchunk.cn_ID = id;
	ih->stopchunk.cn_Type = type;
	ih->stopchunk.cn_Size = 0;
	ih->stopchunk.cn_Scan = 0;
	return err;
}

LONG PropChunk(struct IFFHandle *ih, LONG type, LONG id) {
	debugmsg("PropChunk()");
	LONG err = 0;
	ih->propchunk[ih->propchunk_count].cn_ID = id;
	ih->propchunk[ih->propchunk_count].cn_Type = type;
	ih->propchunk[ih->propchunk_count].cn_Size = 0;
	ih->propchunk[ih->propchunk_count].cn_Scan = 0;
	ih->propchunk_count++;
	return err;
}

LONG ParseIFF(struct IFFHandle *ih, LONG control) {
	debugmsg("ParseIFF()");
	LONG err = IFFERR_READ;
	int i;
	ULONG chkhead[2];
	ULONG chk_id, chk_size, form_type;
	int bytes_read;

	FILE *f = (FILE *) ih->iff_Stream;
	int found_form = 0;
	int stopped = 0;
	int stored = 0;
	char *ca, *cb;

	bytes_read = iffparse_readbytes(f, (char *) chkhead, 8);
	if (bytes_read == 8) {
		chk_id = chkhead[0];
		chk_size = chkhead[1];
		if (chk_id == ID_FORM) {
			found_form = 1;
		} else
		if (chk_id == ID_LIST) {
		} else
		if (chk_id == ID_CAT) {
		} else {
			err = IFFERR_NOTIFF;
		}
	} else {
		if (feof(f)) err = IFFERR_EOF;
	}

	if (found_form) {
		if (control == IFFPARSE_SCAN) {

			bytes_read = iffparse_readbytes(f, (char *) chkhead, 4);
			if (bytes_read == 4) {
				form_type = chkhead[0];

				// we now have a form_type (e.g. ILBM) and read bytes until StopChunk found,
				// or end of form (e.g. ILBM) reached, storing property chunks along the way.
				// note that this currently really isn't a sophisticated loop - we're ignoring
				// nested chunks, have only one stop-chunk, and so on...
				while (1) {
					bytes_read = iffparse_readbytes(f, (char *) chkhead, 8);
					if (bytes_read == 8) {
						chk_id = chkhead[0];
						// reverse bytes, the endianness thing...
						chk_size = chkhead[1];
						bytereverse((char *) &chk_size, 4);


						/*
						ca = (char *) &chk_size;
						for (i = 0; i < 4; i++) { printf("%d ", *ca); ca++; } printf("\n");
						ca = (char *) chkhead;
						for (i = 0; i < 8; i++) { printf("%02x ", *ca); ca++; } printf("\n");
						ca = (char *) chkhead;
						for (i = 0; i < 8; i++) { printf("%c ", *ca); ca++; } printf("\n");
						ca = (char *) chkhead;
						for (i = 0; i < 8; i++) { printf("%d ", *ca); ca++; } printf("\n");

						ca = (char *) &chk_id;
						debugmsg("currentchunk id: %c%c%c%c", ca[0], ca[1], ca[2], ca[3]);
						debugmsg("currentchunk size: %d", chk_size);
						*/

						ih->currentchunk.cn_ID = chk_id;
						ih->currentchunk.cn_Type = form_type;
						ih->currentchunk.cn_Size = chk_size;
						ih->currentchunk.cn_Scan = 0;

						stored = 0;
						for (i = 0; i < ih->propchunk_count; i++) {
							if (ih->propchunk[i].cn_Type == form_type && ih->propchunk[i].cn_ID == chk_id) {
								ca = (char *) &ih->propchunk[i].cn_Type;
								cb = (char *) &ih->propchunk[i].cn_ID;
								debugmsg("found propchunk: type: %c%c%c%c - id: %c%c%c%c", ca[0], ca[1], ca[2], ca[3], cb[0], cb[1], cb[2], cb[3]);
								// read chunk bytes & store the chunk
								if (iffparse_storeproperty(ih, i, chk_size)) {
                                    ih->propchunk[i].cn_Size = chk_size;
                                    stored = 1;
								} else {
									if (feof(f)) err = IFFERR_EOF;
									break;
								}
							}
						}

						if (ih->stopchunk.cn_Type == form_type && ih->stopchunk.cn_ID == chk_id) {
							ca = (char *) &ih->stopchunk.cn_Type;
							cb = (char *) &ih->stopchunk.cn_ID;
							debugmsg("found stopchunk: type: %c%c%c%c - id: %c%c%c%c", ca[0], ca[1], ca[2], ca[3], cb[0], cb[1], cb[2], cb[3]);
							// stopchunk found - stop reading
							stopped = 1;
							if (stored) {
								// restore file seek position from before storing (=after chunk header)
								fseek((FILE *) ih->iff_Stream, -chk_size, SEEK_CUR);
							}
							break;
						}

						if (!stored) {
                            fseek(f, chk_size, SEEK_CUR);
						}

						if (chk_size & 1) {
							// skip a pad byte
							fseek(f, 1, SEEK_CUR);
						}

					} else {
						if (feof(f)) err = IFFERR_EOF;
						break;
					}
				} // end while(1) ...

				if (!stopped) {
					// this means "no current chunk"
					ih->currentchunk.cn_ID = 0;
				}

				// okeedokee. :-)

			} else {
				if (feof(f)) err = IFFERR_EOF;
			}
		} else {
			err = IFFERR_EOF;
		}
	}
	return err;
}

struct StoredProperty *FindProp(struct IFFHandle *ih, LONG type, LONG id) {
	char *ct = (char *) &type;
	char *ci= (char *) &id;
	debugmsg("FindProp() - type: %c%c%c%c - id: %c%c%c%c", ct[0], ct[1], ct[2], ct[3], ci[0], ci[1], ci[2], ci[3]);
	int i;
	for (i = 0; i < ih->propchunk_count; i++) {
		if (ih->propchunk[i].cn_Type == type && ih->propchunk[i].cn_ID == id) {
			return &ih->storedprop[i];
		}
	}
	return NULL;
}

struct ContextNode *CurrentChunk(struct IFFHandle *ih) {
	// cn_ID == 0 indicates "no current chunk"
	if (ih->currentchunk.cn_ID == 0) return NULL;
	return &ih->currentchunk;
}

LONG ReadChunkBytes(struct IFFHandle *ih, UBYTE *buf, LONG size) {
	LONG actual = IFFERR_READ;
	FILE *f = (FILE *) ih->iff_Stream;
	int bytes_read = iffparse_readbytes(f, buf, size);
	if (bytes_read == size) {
		actual = (LONG) bytes_read;
	} else {
		if (feof(f)) actual = IFFERR_EOF;
	}
	return actual;
}

void InitIFFasDOS(struct IFFHandle *ih) {
	ih->type = 'd';
}

LONG OpenIFF(struct IFFHandle *ih, LONG iffmode) {
	LONG err = 0;
	// ih->iff_Stream = unchanged, set before calling this
	ih->iff_Flags  = 0 | iffmode;
	ih->iff_Depth  = 0;
	memset(&ih->stopchunk,    0, sizeof(struct ContextNode));
	memset(&ih->currentchunk, 0, sizeof(struct ContextNode));
	ih->propchunk_count = 0;

	if (iffmode == IFFF_READ) {
		// okeedokee. :-)
	} else {
		usererr("only IFFF_READ supported by OpenIFF()", NULL);
		err = 1;
	}
	return err;
}

void CloseIFF(struct IFFHandle *ih) {
	int i = ih->propchunk_count;
	while (i) {
		i--;
        if(ih->propchunk[i].cn_Size > 0) {
		    free(ih->storedprop[i].sp_Data);
        }
	}
}

BPTR Open(STRPTR filepath, LONG mode) {
	int err = 0;
	BPTR stream = NULL;
	char mode_str[3];
	if (filepath) {
		if (strlen(filepath)) {
			mode_str[0] = 'r';
			mode_str[1] = '+';
			mode_str[2] = 0;
			if (mode == MODE_OLDFILE) {
				// open existing file
				stream = (BPTR) fopen(filepath, mode_str);
				debugmsg("file opened (MODE_OLDFILE), stream: 0x%016x", stream);
			}
			if (mode == MODE_NEWFILE) {
				// create new, or overwrite existing
				// hmm, what about the exclusive-lock thing...?
				mode_str[0] = 'w';
				stream = (BPTR) fopen(filepath, mode_str);
			}
			if (mode == MODE_READWRITE) {
				// open existing file, or create new (no overwrite)
				// this should match AmigaDOS Open() behaviour.
				// (except for the shared-lock thing...?)
				if (!(stream = (BPTR) fopen(filepath, mode_str))) {
					mode_str[0] = 'w';
				    stream = (BPTR) fopen(filepath, mode_str);
				}
			}
		}
	}
	return stream;
}

BOOL Close(BPTR stream) {
	if (stream) fclose((FILE *) stream);
}

struct IFFHandle *AllocIFF() {
	debugmsg("AllocIFF()");
	struct IFFHandle *ih = malloc(sizeof(struct IFFHandle));
    memset(ih, 0, sizeof(struct IFFHandle));
	return ih;
}

void FreeIFF(struct IFFHandle *ih) {
	if (ih) free(ih);
}
