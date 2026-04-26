#ifndef SD_LOADER_H
#define SD_LOADER_H

#include "ff.h"
#include "xstatus.h"
#include "xil_types.h"

int init_sd_card();

int load_file_to_memory(const char* filename, u32 destination_addr, u32 size);

#endif
