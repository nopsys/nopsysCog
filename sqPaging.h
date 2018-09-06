
#include "types.h"

void* get_scheduler();
void save_special_pages();
void save_process_list_pages_with_priority(uint32_t priority);
void save_external_semaphore_pages(uint32_t index);
void save_process_list(void *aProcess);
void save_snapshot_page(ulong virtual_address_failure);
void change_directory_to_read_write(ulong virtual_address_failure);
int  already_saved(void *page_start);
bool is_inside_root_table(ulong virtual_address_failure);



