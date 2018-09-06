#include "nopsys.h"

#include "sq.h"

int nopsys_vm_main(void *image, unsigned int image_length)
{
	memory_file_t file;
	sqImageFile image_file = &file;
	
	sqImageFileCreate(image_file, image);
	sqImageFileSeek(image_file, 0);

	readImageFromFileHeapSizeStartingAt(image_file, image_length + 1024*1024, 0);

	mark(COLOR_GREEN);
	ioInitThreads();

	interpret();

	return 0;
}

void error(char* msg)
{
	perror(msg);
}

//int sqFileInit(void)	{ return true; }
//sqInt sqShrinkMemoryBy(sqInt oldLimit, sqInt delta)	{ return oldLimit; }
//sqInt sqMemoryExtraBytesLeft(sqInt includingSwap)	{ return 0; }


