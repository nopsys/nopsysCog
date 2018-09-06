
#include "string.h"
#include "stdio.h"

#include "sqFile.h"
#include "sqPaging.h"

memory_file_t block;

extern void *snapshot_start, *snapshot_end;

// FIXME: these are present only on stack VM, not in spur, so we have to change the code that uses them
//extern usqInt memory;
//extern usqInt memoryLimit;
usqInt memory;      
usqInt memoryLimit;


sqImageFile sqImageFileCreate(memory_file_t *file, char *buffer)
{
	file->start = (ulong)buffer;
	return file;
}


sqInt sqImageFilePosition(sqImageFile f)
{
	return f->offset;
}

sqInt sqImageFileSeek(sqImageFile f, sqInt pos)
{
	return f->offset = pos;
}


memory_file_t* copy_memory_block()
{
		uint memorySize;
		memorySize = memoryLimit - memory + 1;
		//printf_pocho("Snapshot end : %u\n",computer.snapshotEndAddress);
		block.start = (uint)snapshot_end - memorySize;
		snapshot_start = (uchar*)block.start;
		//printf_pocho("Snapshot start : %u\n",block.start);
		block.length = memorySize;
		block.offset = 0;
		return &block;
}

// called only when saving the image
sqImageFile sqImageFileOpen(char *fileName, int mode)
{
	return copy_memory_block();
}


sqInt sqGetFilenameFromString(char *aCharBuffer, char *aFilenameString, sqInt filenameLength, sqInt aBoolean)
{
  memcpy(aCharBuffer, aFilenameString, filenameLength);
  aCharBuffer[filenameLength]= 0;
  return 0;
}

sqInt sqImageFileRead(char *ptr, sqInt size, sqInt count, sqImageFile f)
{
	size_t total_bytes = count * size;

	memcpy(ptr, (char*)(f->start + f->offset), total_bytes);

	f->offset += total_bytes;

	return total_bytes;
}
	 

sqInt sqImageFileWrite(char *ptr, sqInt size, sqInt count, sqImageFile f)
{
	size_t total_bytes = count * size;
	//printf_pocho("Position to write: %u\n, count to write: %u\n", block.start+block.offset, count);
	if (count >= 2)
		perror("to much bytes to write\n"); // why?

	memcpy(block.start+block.offset, ptr, total_bytes);
	block.offset += total_bytes;

	return count;
}

sqInt sqImageFileClose(sqImageFile f)
{
	extern usqInt memory;

	snapshot_end = (char*)block.offset;
	make_read_only(memory, memory + snapshot_end);
	save_special_pages();	
	return 1;
}




