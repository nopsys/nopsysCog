
#include "nopsys.h"
#include "ints.h"
#include "utils.h"
//#include "math.h"
#include "sys/time.h"

#include <sq.h>
#include <sqAtomicOps.h>
#include <sqAssert.h>

// taken from nopsys computer, is for squeaknos for now
snapshot_info_t snapshot;
void *snapshot_start, *snapshot_end;
void (*page_fault_handler)(void*);
bool in_gc;
int  in_page_fault;
void *page_fault_address;
uint32_t total_page_faults;

// to be called when we fix more important stuff
void init_vars()
{
	snapshot_start = NULL;
	snapshot_end = NULL;
	in_gc = 0;
	in_page_fault = 0;
}




void *os_exports[][3] = { {NULL, NULL, NULL} };

#define	VM_BUILD_STRING "built on "__DATE__ " "__TIME__" Compiler: "__VERSION__


/*** Variables -- image and path names ***/
#define IMAGE_NAME_SIZE 300
char imageName[IMAGE_NAME_SIZE + 1];  /* full path to image */

#define SHORTIMAGE_NAME_SIZE 100
char shortImageName[] = "SqueakNOS.image";  /* just the image file name */

#define VMPATH_SIZE 300
char vmPath[] = "";  /* full path to interpreter's directory */


void printCallStack();


void semaphore_signal_with_index(int index)
{
	signalSemaphoreWithIndex(index);
}

int max(int a, int b)
{
	return a > b? a : b;
}


void perror(const char *s)
{
	// breakpoint();
	// FIXME: Should print also errno and a corresponding message
	if (s)
		printf("%s\n", s);
	printCallStack();
	breakpoint();

	while (true) {}
}

int osCogStackPageHeadroom()
{
	static int stackPageHeadroom = 0;
	if (!stackPageHeadroom)
		stackPageHeadroom = 1024;
	return stackPageHeadroom;
}

sqInt amInVMThread()
{ 
	return ioOSThreadsEqual(ioCurrentOSThread(),getVMOSThread());
}

void
ioInitThreads()
{
	extern void ioInitExternalSemaphores(void);
#if !COGMTVM
	/* Set the current VM thread.  If the main thread isn't the VM thread then
	 * when that thread is spawned it can reassign ioVMThread.
	 */
	ioVMThread = ioCurrentOSThread();
#endif
#if COGMTVM
	//initThreadLocalThreadIndices();
#endif
	ioInitExternalSemaphores();
}


// from unix/vm/sqUnixMain.c
/*
 * Support code for Cog.
 * a) Answer whether the C frame pointer is in use, for capture of the C stack
 *    pointers.
 * b) answer the amount of stack room to ensure in a Cog stack page, including
 *    the size of the redzone, if any.
 */

/*
 * Cog has already captured CStackPointer  before calling this routine.  Record
 * the original value, capture the pointers again and determine if CFramePointer
 * lies between the two stack pointers and hence is likely in use.  This is
 * necessary since optimizing C compilers for x86 may use %ebp as a general-
 * purpose register, in which case it must not be captured.
 */

int
isCFramePointerInUse()
{
#if NOPSYS_COGVM
	extern unsigned long CStackPointer, CFramePointer;
	extern void (*ceCaptureCStackPointers)(void);
	unsigned long currentCSP = CStackPointer;

	currentCSP = CStackPointer;
	ceCaptureCStackPointers();
	assert(CStackPointer < currentCSP);
	return CFramePointer >= CStackPointer && CFramePointer <= currentCSP;
#endif
}

// from unix/vm/sqUnixThreads.c

/* bit 0 = thread to crash in; 1 => this thread
 * bit 1 = crash method; 0 => indirect through null pointer; 1 => call exit
 */
sqInt crashInThisOrAnotherThread(sqInt flags)
{
	if ((flags & 1)) {
		//if (!(flags & 2))
		//	return indirect(flags & ~1);
		perror("crashInThisOrAnotherThread");
		return 0;
	}
	else {
		perror("crashInThisOrAnotherThread what?");
		/*pthread_t newThread;

		(void)pthread_create(
				&newThread,					 // pthread_t *new_thread_ID 
				0,							 // const pthread_attr_t *attr 
				(void *(*)(void *))indirect, // void * (*thread_execp)(void *) 
				(void *)300					 // void *arg 
				);
		sleep(1);*/
	}
	return 0;
}

// from the old nos/sqMinimal.c

char * GetAttributeString(sqInt id)
{
	/* This is a hook for getting various status strings back from
	   the OS. In particular, it allows Squeak to be passed arguments
	   such as the name of a file to be processed. Command line options
	   are reported this way as well, on platforms that support them.
	*/

	// id #0 should return the full name of VM; for now it just returns its path
	if (id == 0) return vmPath;
	// id #1 should return imageName, but returns empty string in this release to
	// ease the transition (1.3x images otherwise try to read image as a document)
	if (id == 1) return shortImageName;  /* will be imageName */
	if (id == 2) return "";

	/* the following attributes describe the underlying platform: */
	if (id == 1001) return "SqueakNOS";
	if (id == 1002) return "v1";
	if (id == 1003) return "Intel x86";

	if (id == 1006) return VM_BUILD_STRING;
	if (id == 1007) return interpreterVersion;
	if (id == 1008) return "$COGIT CLASS VERSION";
	/* It is not good to include sqsccsversion.h. 
	Find another way of defining version*/
	// if (id == 1009) return sourceVersionString(' '); 
	if (id == 1009) return 1; 
	
	/* attribute undefined by this platform */
	success(false);
	return "";
}

sqInt attributeSize(sqInt id) {
	/* return the length of the given attribute string. */
	return strlen(GetAttributeString(id));
}

sqInt getAttributeIntoLength(sqInt id, sqInt byteArrayIndex, sqInt length) {
	/* copy the attribute with the given id into a Squeak string. */
	char *srcPtr, *dstPtr, *end;
	int charsToMove;

	srcPtr = GetAttributeString(id);
	charsToMove = strlen(srcPtr);
	if (charsToMove > length) {
		charsToMove = length;
	}

	dstPtr = (char *) byteArrayIndex;
	end = srcPtr + charsToMove;
	while (srcPtr < end) {
		*dstPtr++ = *srcPtr++;
	}
	return charsToMove;
}

static long          pageSize = 0x1000;
static unsigned long pageMask = ~0xFFF;

# define roundUpToPage(v) (((v)+pageSize-1)&pageMask)

// this looks like something to convert to smalltalk

void *
sqAllocateMemorySegmentOfSizeAboveAllocatedSizeInto(sqInt size, void *minAddress, sqInt *allocatedSizePointer)
{
	// FIXME: should somehow check if address is available
	*allocatedSizePointer = roundUpToPage(size);
	return (char *)roundUpToPage((uintptr_t)minAddress);
}

/* Deallocate a region of memory previously allocated by
 * sqAllocateMemorySegmentOfSizeAboveAllocatedSizeInto.  Cannot fail.
 */
void sqDeallocateMemorySegmentAtOfSize(void *addr, sqInt sz)
{
}


int getpagesize() { return 4096; }


// move the image file to a big space after used memory 
uintptr_t aligned_end_of_memory()
{
	computer_t *computer = current_computer();
	uintptr_t buffer = computer_first_free_address(computer); 

	uintptr_t alignment = max(getpagesize(),1024*1024); // if using 2 mb pages (?)
	uintptr_t mask = ~(alignment - 1);

	return (buffer + alignment - 1) & mask;
}


// take into account that this function should be called only once (to allocate heap memory)
void* sqAllocateMemory(int minHeapSize, int desiredHeapSize)
{
	static void *image_heap = NULL;

	if (image_heap != NULL)
		perror("should be called only once");
	
	image_heap = (void*)aligned_end_of_memory();
	memset(image_heap, 0xCC, desiredHeapSize);
	printf("heap will start at %x\n", image_heap); 
	return image_heap;
}

/*** VM Home Directory Path ***/

sqInt vmPathSize(void)
{
	/* return the length of the path string for the directory containing the VM. */
	return sizeof(vmPath)-1;
}

sqInt vmPathGetLength(sqInt sqVMPathIndex, sqInt length)
{
	/* copy the path string for the directory containing the VM into the given Squeak string. */
	char *stVMPath = (char *) sqVMPathIndex;
	sqInt count, i;

	count = sizeof(vmPath)-1;
	count = (length < count) ? length : count;

	/* copy the file name into the Squeak string */
	for (i = 0; i < count; i++) {
		stVMPath[i] = vmPath[i];
	}
	return count;
}

/*** Image File Name ***/

sqInt imageNameSize(void) {
	/* return the length of the Squeak image name. */
	return sizeof(shortImageName)-1;
}

sqInt imageNameGetLength(sqInt sqImageNameIndex, sqInt length) {
	char *stVMPath = (char *) sqImageNameIndex;
	sqInt count, i;

	count = sizeof(shortImageName)-1;
	count = (length < count) ? length : count;

	/* copy the file name into the Squeak string */
	for (i = 0; i < count; i++) {
		stVMPath[i] = shortImageName[i];
	}
	return count;
}

sqInt imageNamePutLength(sqInt sqImageNameIndex, sqInt length) 
{
	char *stVMPath = (char *) sqImageNameIndex;
	printf("NEW path %s, size = %d\n", stVMPath, length);
	return 0; // do not fail on "save as..."
}


/*** Clipboard Support ***/

sqInt clipboardReadIntoAt(sqInt count, sqInt byteArrayIndex, sqInt startIndex) {
	/* return number of bytes read from clipboard; stubbed out. */
	return 0;
}

sqInt clipboardSize(void) {
	/* return the number of bytes of data the clipboard; stubbed out. */
	return 0;
}

sqInt clipboardWriteFromAt(sqInt count, sqInt byteArrayIndex, sqInt startIndex) {
	/* write count bytes to the clipboard; stubbed out. */
	return 0;
}


/* External modules functions */

typedef struct ModuleEntry {
	struct ModuleEntry *next;
	void *handle;
	sqInt ffiLoaded;
	char name[1];
} ModuleEntry;

sqOSThread ioCurrentOSThread(void)
{
    return 1;
}

int ioOSThreadsEqual(sqOSThread a, sqOSThread b)
{
    return a == b;
}

/*static void *
findExternalFunctionIn(char *functionName, ModuleEntry *module, sqInt fnameLength, sqInt *accessorDepthPtr){
    error("CogNOS should not call external functions!");
    return 0;
}*/

void fillInto(char *dst, size_t startOffset, size_t size, char *src)
{
	memcpy(dst+startOffset, src, size);
}

#define NOPSYSC_HANDLE (void*)0x9090

void * ioFindExternalFunctionInAccessorDepthInto(char *lookupName, void *moduleHandle, sqInt *accessorDepthPtr){
    if (moduleHandle != NOPSYSC_HANDLE)
	    perror("CogNOS only supports ffi calls to 'nopsysC' right now!");
    
	if (strcmp(lookupName, "fillInto") == 0)
		return fillInto;
    
    perror("function not found!");
    
    return 0;
}

void *ioLoadModule(char *moduleName)
{
	if (strcmp(moduleName, "nopsysC") != 0)
	{
		//printf("Looking for external module %s. Module not available\n", moduleName);
		return 0;
	}
	
	printf("Looking for external module %s. Module not available\n", moduleName);
	return NOPSYSC_HANDLE;
}  

sqInt ioFreeModule(void *moduleHandle)
{
  perror("CogNOS should not load/free external modules (plugins)");
  return 0;
}

/* Timing functions */
#if !defined(DEFAULT_BEAT_MS)
# define DEFAULT_BEAT_MS 2
#endif
#define MicrosecondsPerSecond 1000000LL
static long long vmGMTOffset = 0;
static unsigned volatile long long localMicrosecondClock;
static unsigned long heartbeats;
static unsigned long long frequencyMeasureStart = 0;
static unsigned long long utcStartMicroseconds; /* for the ioMSecs clock. */
static int beatMilliseconds = DEFAULT_BEAT_MS;

/* Check the following struct */
struct timespec {
    time_t   tv_sec;        /* seconds */
    long     tv_nsec;       /* nanoseconds */
};

static struct timespec beatperiod = { 0, DEFAULT_BEAT_MS * 1000 * 1000 };

// return amount of milliseconds 
long ioMSecs(void) {
	sqInt milliseconds = nopsys_ticks*1000.0/TIMER_FREQ_HZ;
	return milliseconds;
}

long ioMicroMSecs(void) {
  /* return the highest available resolution of the millisecond clock */
  return ioMSecs();	/* this already to the nearest millisecond */
}


sqLong ioHighResClock(void)
{
  return get_tsc();
}

unsigned volatile long long  ioUTCMicrosecondsNow()
{
  return ioHighResClock() * 1000000.0 / nopsys_tsc_freq;
}

unsigned volatile long long  ioUTCMicroseconds()
{
  return ioUTCMicrosecondsNow();
}

unsigned volatile long long ioLocalMicroseconds()
{
	//FIXME: returning garbage
	return localMicrosecondClock;
}


sqInt ioSecondsNow(void)
{
	return ioLocalMicrosecondsNow() / MicrosecondsPerSecond;
}

sqInt ioLocalSecondsOffset() {
	return vmGMTOffset / MicrosecondsPerSecond;
}

// from unix/vm/sqUnixMain.c
unsigned long long currentUTCMicroseconds()
{
	perror("currentUTCMicroseconds needs gettimeofday");
	return 0;
	//struct timeval utcNow;
	//gettimeofday(&utcNow,0);
	//return ((utcNow.tv_sec * MicrosecondsPerSecond) + utcNow.tv_usec)
	//		+ MicrosecondsFrom1901To1970;
}

unsigned volatile long long ioLocalMicrosecondsNow()
{
	return currentUTCMicroseconds() + vmGMTOffset;
}

unsigned long long ioUTCStartMicroseconds()
{
	return utcStartMicroseconds;
}

void ioSetHeartbeatMilliseconds(int ms)
{
	beatMilliseconds = ms;
	beatperiod.tv_sec = beatMilliseconds / 1000;
	beatperiod.tv_nsec = (beatMilliseconds % 1000) * 1000 * 1000;
}




// CAREFUL: We use hlt instruction, which pauses the processor
// until next interrupt. In default nopsys configuration, that'd be 0.5 ms.
// Now, documentation for ProcessorScheduler>>relinquishProcessorForMicroseconds:
// says the prim can return _before_ the arg passed, but not after.
// relinquish is sent from the ProcessorScheduler>>idleProcess
// with 1ms (and also with 10ms from #sweepHandleProcess). 

sqInt ioRelinquishProcessorForMicroseconds(sqInt mSecs)
	{
	float hlt_pause_in_ms = 1000.0 / TIMER_FREQ_HZ;
	int times = 1; //mSecs / hlt_pause_in_ms;

	for (int i = 0; i < times; i++)
		asm("hlt");
		
	return 0;
}

// Implement properly to support timezones
void ioUpdateVMTimezone() {}




uint bcd_to_bin(uint bcd)
{
  return (bcd / 16) * 10 + (bcd & 0xf);
}

unsigned char get_cmos_byte_at(uint i)
{
	outb(0x70, i);
	return inb(0x71);
}

uint16_t get_cmos_month()
{
	uint month = get_cmos_byte_at(8);
	return bcd_to_bin(month);
}

uint16_t get_cmos_day()
{
	uint day = get_cmos_byte_at(7);
	return bcd_to_bin(day);
}


/* See SqUnixHertbeat.c line 73*/
void
ioGetClockLogSizeUsecsIdxMsecsIdx(sqInt *np, void **usecsp, sqInt *uip, void **msecsp, sqInt *mip)
{
	*np = *uip = *mip = 0;
	*usecsp = *msecsp = 0;
}


void ioInitHeartbeat()
{
	printf("ioInitHeartbeat unimplemented, we should create a second thread\n");
}

/* Answer the average heartbeats per second since the stats were last reset.
 */
unsigned long ioHeartbeatFrequency(int resetStats)
{
	unsigned long duration = (ioUTCMicroseconds() - get64(frequencyMeasureStart))
						/ MicrosecondsPerSecond;
	unsigned long frequency = duration ? heartbeats / duration : 0;

	if (resetStats) {
		unsigned long long zero = 0;
		set64(frequencyMeasureStart,zero);
	}
	return frequency;
}

int ioHeartbeatMilliseconds() { return beatMilliseconds; }


/* Profiling & Logging */

long  ioControlNewProfile(int on, unsigned long buffer_size) {
  perror("ioControlNewProfile is needed for profiling. Are we using profiling in CogNOS?");
  return 0;
}

void  ioClearProfile(void){}

sqInt ioSetLogDirectoryOfSize(void* lblIndex, sqInt sz) { return 1; }
void ioNewProfileStatus(sqInt *running, long *buffersize) {}
long ioNewProfileSamplesInto(void *sampleBuffer) { return 0;}
char* ioGetLogDirectory(void) { return ""; }

/* Display */

extern computer_t computer;

//From SqUnixMain.c and SqUnixDisplayNull
sqInt ioForceDisplayUpdate(void)
{
	//printf("forcing display update\n");
	return 0;
}

sqInt ioScreenDepth(void)
{
	return computer.video_info.depth;
} 

sqInt ioScreenSize(void)
{
  return computer.video_info.width << 16 | computer.video_info.height;
}

sqInt ioSetFullScreen(sqInt fullScreen)
{
	return 1;
}

sqInt ioSetDisplayMode(sqInt width, sqInt height, sqInt depth, sqInt fullscreenFlag)
{
	return 1;
}

sqInt ioHasDisplayDepth(sqInt depth)
{
  return abs((int)depth) == ioScreenDepth();
}

sqInt ioIsWindowObscured(void) { return false; }

char* ioGetWindowLabel(void) {return "";}

sqInt ioSetCursor(sqInt cursorBitsIndex, sqInt offsetX, sqInt offsetY) { return 0; }
sqInt ioSetCursorARGB(sqInt cursorBitsIndex, sqInt extentX, sqInt extentY, sqInt offsetX, sqInt offsetY) { return 0; }
sqInt ioSetCursorWithMask(sqInt cursorBitsIndex, sqInt cursorMaskIndex, sqInt offsetX, sqInt offsetY) { return 1; }

/* Right now this funciton isn't responded to so we simply provide a dummy
 * definition here.  If any of the display subsystems do need it then it will
 * have to be reimplemented as per the functions above.
 */
void  ioNoteDisplayChangedwidthheightdepth(void *b, int w, int h, int d) {
	//printf("note display changed b: %x, w: %d, h: %d, d: %d\n", b, w, h, d);

}

sqInt ioFormPrint(sqInt bitsAddr, sqInt width, sqInt height, sqInt depth, double hScale, double vScale, sqInt landscapeFlag)
{ 
	//printf("ioFormPrint stub\n");
	return false;
}

/* Take care with the following implementations!!!*/
sqInt ioGetWindowWidth()
{
	return computer.video_info.width;
} 

sqInt ioGetWindowHeight()
{
	return computer.video_info.height;
}
 
double ioScreenScaleFactor(void)
{
	return 1.0;
}

sqInt ioProcessEvents(void)
{
	return 0;
}

/* Sound Functions */
sqInt ioBeep(void)
{
	return 0;
}

/* Error Handling */
sqInt ioExitWithErrorCode(int errorCode)
{ 
	console_set_debugging(1);
	printf("ioExit()\n\n");
	printCallStack();
	breakpoint();
	serial_enter_debug_mode();
  return errorCode;
}

//Should we support these? 
sqInt ioSetWindowWidthHeight(sqInt w, sqInt h) { return 0; }
sqInt ioSetWindowLabelOfSize(void* lbl, sqInt size) { return 0; }

sqInt ioShowDisplay(sqInt fromImageData, sqInt width, sqInt height, sqInt depth,
                    sqInt affectedL, sqInt affectedR, sqInt affectedT, sqInt affectedB)
{
  int lastWord;
  int firstWord;
  int line, countPerLine;

  uintptr_t toImageData = (uintptr_t)computer.video_info.address;
  uint scanLine= computer.video_info.bytes_per_scanline;

  // fromImage = pointerForOop(fromImageData);
  firstWord= scanLine*affectedT + BYTES_PER_LINE_FLOOR(affectedL, depth);
  lastWord= scanLine*affectedT + BYTES_PER_LINE_PADDED(affectedR, depth);
  countPerLine = lastWord - firstWord;



  switch (computer.video_info.depth) {
    case 16:
      for (line= affectedT; line < affectedB; line++, firstWord += scanLine) {
        uint16_t *from = (uint16_t*)(fromImageData+firstWord);
        uint16_t *to   = (uint16_t*)(toImageData+firstWord);
        for (;to < (uint16_t*)(toImageData+firstWord+countPerLine); to+=2, from+=2) {
          to[0] = repack(from[1]);
          to[1] = repack(from[0]);
        }
      }
      break;
    case 8:
      for (line= affectedT; line < affectedB; line++, firstWord += scanLine) {
        uint32_t *from = (uint32_t*)(fromImageData+firstWord);
        uint32_t *to   = (uint32_t*)(toImageData+firstWord);
        for (;to < (uint32_t*)(toImageData+firstWord+countPerLine); to+=1, from+=1) 
		  // to[0] = from[0];
          to[0] = swap32(from[0]);
      }
      break;
    default:
      for (line= affectedT; line < affectedB; line++, firstWord += scanLine)
        memcpy((char*)toImageData+firstWord, (char*)fromImageData+firstWord, countPerLine);
      break;
  }

  return 0;
}

void sqMakeMemoryExecutableFromTo(unsigned long startAddr, unsigned long endAddr)
{ 
}

void sqMakeMemoryNotExecutableFromTo(unsigned long startAddr, unsigned long endAddr)
{ 
}


/* Mouse & Keyboard */
sqInt ioPeekKeystroke(void) { return -1; }
sqInt ioGetKeystroke(void) { return -1; }
sqInt ioGetButtonState(void) { return 0;  }
sqInt ioMousePoint(void) { return 320 << 16 | 240; }
sqInt ioGetNextEvent(sqInputEvent *evt) { return 0; }

/* Semaphores */

sqInt ioSetInputSemaphore(sqInt semaIndex)		{ return true; }

/* Power Management */

sqInt ioDisablePowerManager(sqInt disableIfNonZero)	{ return true; }


