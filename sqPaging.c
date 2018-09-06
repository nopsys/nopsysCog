
#include "sqPaging.h"

void* get_scheduler()
{
/* ////////////	extern sqInt specialObjectsOop;
	usqInt association;
	association  = longAt((specialObjectsOop + (BASE_HEADER_SIZE)) + (SchedulerAssociation << (SHIFT_FOR_WORD)));
	return longAt((association + (BASE_HEADER_SIZE)) + (ValueIndex << (SHIFT_FOR_WORD))); */
	return NULL; //////////
} 

void save_special_pages()
{
/* //////	extern irq_semaphores_t irq_semaphores;
	extern usqInt activeContext, youngStart;
	usqInt activeProcess, scheduler;
	scheduler = getScheduler();
	activeProcess = longAt((scheduler + (BASE_HEADER_SIZE)) + (ActiveProcessIndex << (SHIFT_FOR_WORD)));
	saveExternalSemaphorePages(irq_semaphores[1]); 	//keyboard
	saveExternalSemaphorePages(irq_semaphores[3]);   //serial port
	saveExternalSemaphorePages(irq_semaphores[4]);	//serial port
	saveExternalSemaphorePages(irq_semaphores[12]);	//mouse
	saveExternalSemaphorePages(irq_semaphores[15]);	//page Fault
	saveProcessListPagesWithPriority(40);
	saveProcessListPagesWithPriority(70);
	saveProcessListPagesWithPriority(71);
	saveSnapshotPage(activeProcess);
	saveSnapshotPage(activeContext);
	saveSnapshotPage(scheduler);
	if ((youngStart && 0xFFFFF000) != youngStart) saveSnapshotPage(youngStart);	
*/
	//saveExternalSemaphorePages(irq_semaphores[8]);	//cmos
}	

void save_process_list_pages_with_priority(uint32_t priority)
{
/* //////////	extern sqInt specialObjectsOop,nilObj;
	usqInt association,scheduler,processLists,processList,firstProcess;
	processLists = longAt((getScheduler() + (BASE_HEADER_SIZE)) + (ProcessListsIndex << (SHIFT_FOR_WORD)));
	processList = longAt((processLists + (BASE_HEADER_SIZE)) + ((priority - 1) << (SHIFT_FOR_WORD)));
	saveSnapshotPage(processList);
	firstProcess = longAt((processList + (BASE_HEADER_SIZE)) + (FirstLinkIndex << (SHIFT_FOR_WORD)));
	if (firstProcess != nilObj) saveProcessList(firstProcess); */
}	

void save_external_semaphore_pages(uint32_t index)
{
/* //////////	extern sqInt specialObjectsOop,nilObj;
	sqInt array, semaphore,firstProcess;
	printf_tab("Entre saveExternalSemaphorePages %d\n", index);	
	array = longAt((specialObjectsOop + (BASE_HEADER_SIZE)) + (ExternalObjectsArray << (SHIFT_FOR_WORD)));
	semaphore = longAt((array + (BASE_HEADER_SIZE)) + ((index - 1) << (SHIFT_FOR_WORD)));	
	saveSnapshotPage(semaphore);
	firstProcess = longAt((semaphore + (BASE_HEADER_SIZE)) + (FirstLinkIndex << (SHIFT_FOR_WORD)));
	if ((firstProcess == nilObj) || (index == 0)) return;
	saveProcessList(firstProcess);
	printf_tab("Sali saveExternalSemaphorePages\n"); */
}

void save_process_list(void *aProcess)
{
/* ////////////	extern sqInt nilObj;
	sqInt actual;
	actual = aProcess;
	while(actual != nilObj){
		saveSnapshotPage(actual);
		actual = longAt((actual + (BASE_HEADER_SIZE)) + (NextLinkIndex << (SHIFT_FOR_WORD)));
	} */
}

void save_snapshot_page(ulong virtual_address_failure)
{
/* //////////	extern Computer computer;
	printf_tab("Entre a saveSnapshotPage en la direccion:%d\n",virtual_address_failure);
	ulong pageStart = virtual_address_failure & 0xFFFFF000;
	if (alreadySaved(pageStart)) {printf_tab("Sali de saveSnapshotPage por el alreadyStart\n");return;}
	ulong saved = computer.snapshot.pagesSaved;
	computer.snapshot.pages[saved].virtualAddress = pageStart;
	computer.snapshot.pages[saved].physicalAddress = pageStart;
	//printf("estructura: %x, posicion actual: %x \n", computer->snapshot.pages, computer->snapshot.pages[saved].contents);
	memcpy(computer.snapshot.pages[saved].contents, pageStart, 4096); 	
	changeDirectoryToReadWrite(virtual_address_failure);
	computer.snapshot.pagesSaved = saved + 1;
	printf_tab("Sali de saveSnapshotPage\n"); */
}

void change_directory_to_read_write(ulong virtual_address_failure)
{
/* /////////////	ulong directoryIndex, pageTableIndex, pageDirectoryEntry, *directory, *pageTable, *pageTableEntry;
	__asm("movl %%cr3, %0" : "=a" (directory) );
	directoryIndex = virtual_address_failure >> 22;
	directoryIndex &= 0x000003FF;
	pageDirectoryEntry = directory[directoryIndex];
	pageTable = pageDirectoryEntry & 0xfffff000;
	pageTableIndex = virtual_address_failure >> 12;
	pageTableIndex &= 0x000003FF;
	pageTableEntry = &pageTable[pageTableIndex];
	*pageTableEntry |= 0x00000002; */
}

int already_saved(void *page_start)
{
/* /////////////	extern Computer computer;	
	ulong i;
	for (i=0; i<computer.snapshot.pagesSaved; i++)
		if (computer.snapshot.pages[i].virtualAddress == page_start) return 1;
	 */
	return 0;
}

bool is_inside_root_table(ulong virtual_address_failure)
{
/* ////////////////	extern ulong rootTableCount,extraRootCount;
	extern ulong* rootTable;
	extern ulong* extraRoots;
	for (int i = 1; i <= rootTableCount; i += 1) {
		ulong oop = rootTable[i];
		if ((virtual_address_failure >= oop) && (virtual_address_failure <= oop + 100))
		{
			printf_tab("IsInsideRootTable: RootTable: %d",oop);
			return true;
		}
	}
	
	for (int i = 1; i <= extraRootCount; i += 1) {
		ulong oop = ((ulong*)(extraRoots[i]))[0];
		if (!((oop & 1))) {
			if ((virtual_address_failure >= oop) && (virtual_address_failure <= oop + 100))
			{
				printf_tab("IsInsideRootTable: ExtraRootTable: %d",oop);
				return true;
			}
		}
	}
*/ /////////////////////
	return false;
}



