#ifndef __SQ_PLATFORM_SPECIFIC_H__
#define __SQ_PLATFORM_SPECIFIC_H__

#include "nopsys.h"

#define initialHeapSize 100*1024*1024

typedef struct readonly_page_t
{
	unsigned long virtualAddress;
	unsigned long physicalAddress;
	unsigned char contents[4096];
} readonly_page_t;

typedef struct snapshot_info_t
{
	unsigned long pagesSaved;
	unsigned long pagesToSave;
	readonly_page_t *pages;
} snapshot_info_t;



sqInt sqMain(void *image, unsigned int image_length);

void enable_paging();

void mark(int col);

#define warnPrintf printf

/* undefine clock macros (these are implemented as functions) */

//#undef ioMSecs
//#undef ioMicroMSecs
#undef ioLowResMSecs

#undef sqAllocateMemory
#undef sqGrowMemoryBy
#undef sqShrinkMemoryBy
#undef sqMemoryExtraBytesLeft

void* sqAllocateMemory(int minHeapSize, int desiredHeapSize);

#include "sqMemoryAccess.h"
#include "sqFile.h"

#define allocateMemoryMinimumImageFileHeaderSize(heapSize, minimumMemory, fileStream, headerSize) \
			sqAllocateMemory(minimumMemory, heapSize)


#undef	sqFilenameFromString
#undef	sqFilenameFromStringOpen
#define sqFilenameFromStringOpen sqFilenameFromString

#undef dispatchFunctionPointer
#undef dispatchFunctionPointerOnin

#undef	sqFTruncate

#define sqOSThread void *

#if defined(__GNUC__)
# if !defined(VM_LABEL)
#	define VM_LABEL(foo) asm("\n.globl L" #foo "\nL" #foo ":")
# endif
#endif

void sqMakeMemoryExecutableFromTo(unsigned long, unsigned long);
void sqMakeMemoryNotExecutableFromTo(unsigned long, unsigned long);

#endif  /* __SQ_PLATFORM_SPECIFIC_H__ */


