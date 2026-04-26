#include "sd_loader.h"
#include "xil_printf.h"

static FATFS fatfs;

int init_sd_card() {

    FRESULT res = f_mount(&fatfs, "0:/", 0);

    if (res != FR_OK) {
        xil_printf("SD Mount Failed early: %d\r\n", res);
        return XST_FAILURE;
    }

    DIR dir;
    res = f_opendir(&dir, "0:/");
    if (res != FR_OK) {
        xil_printf("FileSystem Validation Failed: %d\r\n", res);
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

int load_file_to_memory(const char* filename, u32 destination_addr, u32 size) {
    FIL fil;
    UINT br;
    FRESULT res;

    res = f_open(&fil, filename, FA_READ);
    if (res != FR_OK) {
        xil_printf("Error: Could not open %s\r\n", filename);
        return XST_FAILURE;
    }

    xil_printf("Loading %s to 0x%08X...\r\n", filename, destination_addr);
    res = f_read(&fil, (void*)destination_addr, size, &br);

    if (res != FR_OK || br != size) {
        xil_printf("Error: Failed to read %s completely\r\n", filename);
        f_close(&fil);
        return XST_FAILURE;
    }

    f_close(&fil);
    return XST_SUCCESS;
}
