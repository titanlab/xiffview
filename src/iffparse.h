/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
*/

/*
Sun Apr 11 07:45:38 PM CEST 2021
ok we have to make a decision - this will break AmigaOS code compatibility a bit.
but our first goal is to make this work on Linux - compatibility is second.
example: Open() returns pointer, which is 32 bit on Amiga, and gets casted to
         and from ULONG, but is 64 bit on this linux machine 
         (Linux 5.8.18-300.fc33.x86_64). if we change ULONG to 64 bit we may break 
		 other functions (think e.g. writing Amiga 32 bit ULONG file contents).
		 so we use real pointers where pointers are actually used, trying to avoid
		 casting, in our Linux code, and adjust Amiga-style API if required.
*/

struct ContextNode {
    struct MinNode cn_Node;
    LONG	   cn_ID;
    LONG	   cn_Type;
    LONG	   cn_Size;
    LONG	   cn_Scan;	
};

struct StoredProperty {
    LONG sp_Size;
    APTR sp_Data;
};

struct IFFHandle {
	//for Linux:
	// must use real pointer for proper Linux 64 bit pointer storage
	// (this break Amiga code compatibility) 
    //ULONG iff_Stream;
    BPTR iff_Stream;

    ULONG iff_Flags;
    LONG  iff_Depth;	

	/*
 	 * for Linux:
	 */

	// type is either 'c' for clipboard, or 'd' for DOS
	char type;

	// the chunk last read by ParseIFF()
	struct ContextNode currentchunk;

	// the chunk set with StopChunk()
	struct ContextNode stopchunk;

	// the prop chunks set with PropChunk(),
	// and their stored properties.
	struct ContextNode    propchunk[64];
	struct StoredProperty storedprop[64];
	int propchunk_count;
};

#define IFFF_READ	0L			 
#define IFFF_WRITE	1L			
#define IFFF_RWBITS	(IFFF_READ | IFFF_WRITE)
#define IFFF_FSEEK	(1L<<1)		 
#define IFFF_RSEEK	(1L<<2)		
#define IFFF_RESERVED	0xFFFF0000L

struct IFFStreamCmd {
    LONG sc_Command;	/* Operation to be performed (IFFCMD_) */
    APTR sc_Buf;	/* Pointer to data buffer	       */
    LONG sc_NBytes;	/* Number of bytes to be affected      */
};

struct LocalContextItem {
    struct MinNode lci_Node;
    ULONG	   lci_ID;
    ULONG	   lci_Type;
    ULONG	   lci_Ident;
};

struct CollectionItem {
    struct CollectionItem *ci_Next;
    LONG		   ci_Size;
    APTR		   ci_Data;
};

#define IFFERR_EOF	  -1L	
#define IFFERR_EOC	  -2L
#define IFFERR_NOSCOPE	  -3L	
#define IFFERR_NOMEM	  -4L
#define IFFERR_READ	  -5L	
#define IFFERR_WRITE	  -6L	
#define IFFERR_SEEK	  -7L	
#define IFFERR_MANGLED	  -8L	
#define IFFERR_SYNTAX	  -9L
#define IFFERR_NOTIFF	  -10L
#define IFFERR_NOHOOK	  -11L
#define IFF_RETURN2CLIENT -12L

// the original Amiga MAKE_ID()
#define MAKE_ID_MSB(a,b,c,d)	\
	((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))

// for Linux: 
// we're probably not MC68000 (MSB first, big endian) but x86_64 (LSB first, little endian)
#define MAKE_ID_LSB(a,b,c,d)	\
	((ULONG) (d)<<24 | (ULONG) (c)<<16 | (ULONG) (b)<<8 | (ULONG) (a))

#define MAKE_ID(a,b,c,d)    MAKE_ID_LSB(a,b,c,d)

#define ID_FORM		MAKE_ID('F','O','R','M')
#define ID_LIST		MAKE_ID('L','I','S','T')
#define ID_CAT		MAKE_ID('C','A','T',' ')
#define ID_PROP		MAKE_ID('P','R','O','P')
#define ID_NULL		MAKE_ID(' ',' ',' ',' ')

#define IFFLCI_PROP		MAKE_ID('p','r','o','p')
#define IFFLCI_COLLECTION	MAKE_ID('c','o','l','l')
#define IFFLCI_ENTRYHANDLER	MAKE_ID('e','n','h','d')
#define IFFLCI_EXITHANDLER	MAKE_ID('e','x','h','d')

#define IFFPARSE_SCAN	 0L
#define IFFPARSE_STEP	 1L
#define IFFPARSE_RAWSTEP 2L

#define IFFSLI_ROOT  1L  
#define IFFSLI_TOP   2L 
#define IFFSLI_PROP  3L

#define IFFSIZE_UNKNOWN -1L

#define IFFCMD_INIT	    0
#define IFFCMD_CLEANUP	1
#define IFFCMD_READ	    2	
#define IFFCMD_WRITE	3	
#define IFFCMD_SEEK	    4	
#define IFFCMD_ENTRY	5
#define IFFCMD_EXIT	    6	
#define IFFCMD_PURGELCI 7

#define IFFSCC_INIT	IFFCMD_INIT
#define IFFSCC_CLEANUP	IFFCMD_CLEANUP
#define IFFSCC_READ	IFFCMD_READ
#define IFFSCC_WRITE	IFFCMD_WRITE
#define IFFSCC_SEEK	IFFCMD_SEEK

/*
 * for Linux:
 * missing defines
 * iffparse prototypes
 */

// from amiga dos/dos.h:

/* Mode parameter to Open() */
#define MODE_OLDFILE	     1005   
#define MODE_NEWFILE	     1006  
#define MODE_READWRITE	     1004 

struct IFFHandle *AllocIFF(void);
BPTR Open(STRPTR, LONG);
void InitIFFasDOS(struct IFFHandle *);
LONG OpenIFF(struct IFFHandle *, LONG);
LONG StopChunk(struct IFFHandle *, LONG, LONG);
LONG PropChunk(struct IFFHandle *, LONG, LONG);
LONG ParseIFF(struct IFFHandle *, LONG);
struct StoredProperty *FindProp(struct IFFHandle *, LONG, LONG);
struct ContextNode *CurrentChunk(struct IFFHandle *);
LONG ReadChunkBytes(struct IFFHandle *, UBYTE *, LONG);
void CloseIFF(struct IFFHandle *);
BOOL Close(BPTR);
void FreeIFF(struct IFFHandle *);
