
#ifndef _SQ_FILE_H_
#define _SQ_FILE_H_

/** File things **/
#include "types.h"
#include "sqMemoryAccess.h"

// undefine things defaulting to libc (platforms/Cross/vm/sq.h)

#undef sqImageFile
#undef sqImageFileOpen
#undef sqImageFileClose
#undef sqImageFilePosition
#undef sqImageFileRead
#undef sqImageFileSeek
#undef sqImageFileWrite
#undef sqImageFileStartLocation

typedef struct SQFile
{
	char  *file;
	ulong  offset;
	int    sessionID;
	int    writable;
	int    fileSize;
	int    lastOp;  /* 0 = uncommitted, 1 = read, 2 = write */
} SQFile;

typedef struct memory_file_t
{
	ulong start;
	ulong offset;
	ulong length;
} memory_file_t;


typedef memory_file_t* sqImageFile;
typedef uint squeakFileOffsetType;


sqImageFile sqImageFileCreate(memory_file_t *file, char *buffer);
sqImageFile sqImageFileOpen(char *fileName, int mode);
sqInt sqImageFilePosition(sqImageFile f);
sqInt sqImageFileSeek(sqImageFile f, sqInt pos);
sqInt sqImageFileWrite(char *ptr, sqInt size, sqInt count, sqImageFile f);
sqInt sqImageFileRead(char *ptr, sqInt size, sqInt count, sqImageFile f);
#define sqImageFileStartLocation(fileRef, fileName, size)  0


#endif   // _SQ_FILE_H_


