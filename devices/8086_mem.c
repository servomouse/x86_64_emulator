#include "8086_mem.h"
#include "utils.h"
#include <string.h>

// char *BIOS_FILENAME_0XF0000 = "BIOS/BIOS_5160_09MAY86_F0000.BIN";
// char *BIOS_FILENAME_0XF8000 = "BIOS/BIOS_5160_09MAY86_F8000.BIN";
char *BIOS_FILENAME_0XF0000 = "BIOS/08NOV82_F0000.BIN";
char *BIOS_FILENAME_0XF8000 = "BIOS/08NOV82_F8000.BIN";
// char *BIOS_FILENAME_0XF0000 = "BIOS/F0000.BIN";
// char *BIOS_FILENAME_0XF8000 = "BIOS/F8000.BIN";
#define MEMORY_LOG_FILE "logs/mem_log.txt"
#define CODE_MEM_LOG_FILE "logs/code_mem_log.txt"
#define VIDEO_MEM_LOG_FILE "logs/video_mem_log.txt"
#define MEMORY_DUMP_FILE "data/memory_dump.bin"

#define MEMORY_SIZE 0x100000

static uint8_t *MEMORY = NULL;
static uint8_t error = 0;

size_t get_file_size(FILE *file) {
    size_t init_location = ftell(file);
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, init_location, SEEK_SET);
    return file_size;
}

int copy_bios_into_memory(uint8_t *memory, size_t offset, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("MEMORY ERROR: Failed to open file %s\n", filename);
        return  EXIT_FAILURE;
    }
    if (0x8000 < get_file_size(file)) {  // Check file size
        printf("MEMORY ERROR: File %s is too large!", filename);
        return  EXIT_FAILURE;
    }
    uint8_t *ptr = memory + offset; // Move the memory pointer to the specified offset
    size_t bytes_read = 0;
    while((bytes_read = fread(ptr, 1, 1, file)) > 0) {
        ptr++;
    }
    fclose(file);
    return EXIT_SUCCESS;
}

int store_memory(void) {
    FILE *file = fopen(MEMORY_DUMP_FILE, "wb");
    if (file == NULL) {
        printf("MEMORY ERROR: Failed to open file %s\n", MEMORY_DUMP_FILE);
        return  EXIT_FAILURE;
    }
    fwrite(MEMORY, MEMORY_SIZE, 1, file);
    fclose(file);
    return EXIT_SUCCESS;
}

int restore_memory(uint8_t *memory, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("MEMORY ERROR: Failed to open file %s\n", filename);
        return  EXIT_FAILURE;
    }
    uint8_t *ptr = memory;
    size_t bytes_read = 0;
    while((bytes_read = fread(ptr, 1, 1, file)) > 0) {
        ptr++;
    }
    fclose(file);
    return EXIT_SUCCESS;
}

__declspec(dllexport)
uint8_t * mem_init(uint8_t continue_simulation) {
    uint8_t *memory = (uint8_t*)calloc(sizeof(uint8_t), MEMORY_SIZE);
    if(continue_simulation) {
        restore_memory(memory, MEMORY_DUMP_FILE);
    } else {
        if ((EXIT_FAILURE == copy_bios_into_memory(memory, 0xF0000, BIOS_FILENAME_0XF0000)) ||
            (EXIT_FAILURE == copy_bios_into_memory(memory, 0xF8000, BIOS_FILENAME_0XF8000))) {
            return NULL;
        }
    }
    MEMORY = memory;
    return memory;
}

#define VIDEO_BUFFER_SIZE 20
#define VIDEO_BUFFER_OFFSET 0xB0000

__declspec(dllexport)
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    char video_buf[VIDEO_BUFFER_SIZE];
    if((addr >= 0xA0000) && (addr < 0xC0000)) {
        if((MEMORY[VIDEO_BUFFER_OFFSET] >= 0x20) && (MEMORY[VIDEO_BUFFER_OFFSET] < 0x7F)) {
            for(int i=0; i<VIDEO_BUFFER_SIZE; i++) {
                video_buf[i] = MEMORY[VIDEO_BUFFER_OFFSET+(i*2)];
            }
            mylog(1, VIDEO_MEM_LOG_FILE, "VIDEO_BUF: %s", video_buf);
        }
        mylog(0, VIDEO_MEM_LOG_FILE, "MEM_WRITE addr = 0x%06X, value = 0x%04X, width = %d byte(s)\n", addr, value, width);
    }else {
        mylog(0, MEMORY_LOG_FILE, "MEM_WRITE addr = 0x%06X, value = 0x%04X, width = %d byte(s)\n", addr, value, width);
    }
    if(width == 1) {
        MEMORY[addr] = value;
    } else if (width == 2) {
        *((uint16_t*)&(MEMORY[addr])) = value;
    } else {
        printf("MEM WRITE ERROR: Incorrect width: %d\n", width);
    }
}

__declspec(dllexport)
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    if(width == 1) {
        ret_val = MEMORY[addr];
    } else if (width == 2) {
        ret_val = MEMORY[addr] + (MEMORY[addr+1] << 8);
    } else {
        printf("MEM READ ERROR: Incorrect width: %d", width);
    }
    if((addr >= 0xA0000) && (addr < 0xC0000)) {
        mylog(0, VIDEO_MEM_LOG_FILE, "MEM_READ  addr = 0x%06X, value = 0x%04X, width = %d byte(s)\n", addr, width, ret_val);
    }else {
        mylog(0, MEMORY_LOG_FILE, "MEM_READ  addr = 0x%06X, value = 0x%04X, width = %d byte(s)\n", addr, width, ret_val);
    }
    return ret_val;
}

__declspec(dllexport)
uint16_t code_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    if(addr < 0xE0000) {
        printf("CODE READ ERROR: Read outside of code sector: 0x%08X\n", addr);
        error = 1;
    }
    if(width == 1) {
        ret_val = MEMORY[addr];
    } else if (width == 2) {
        ret_val = MEMORY[addr] + (MEMORY[addr+1] << 8);
    } else {
        printf("CODE READ ERROR: Incorrect width: %d", width);
    }
    mylog(0, CODE_MEM_LOG_FILE, "CODE_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
    return ret_val;
}

__declspec(dllexport)
void module_save(void) {
    store_data(MEMORY, MEMORY_SIZE, MEMORY_DUMP_FILE);
}

__declspec(dllexport)
void module_restore(void) {
    uint8_t temp[MEMORY_SIZE] = {0};
    if(EXIT_SUCCESS == restore_data(temp, MEMORY_SIZE, MEMORY_DUMP_FILE)) {
        memcpy(MEMORY, temp, MEMORY_SIZE);
    }
}

__declspec(dllexport)
void module_reset(void) {
    uint8_t *memory = (uint8_t*)calloc(sizeof(uint8_t), MEMORY_SIZE);
    // if(continue_simulation) {
    //     restore_memory(memory, MEMORY_DUMP_FILE);
    // } else {
    if (EXIT_FAILURE == copy_bios_into_memory(memory, 0xF0000, BIOS_FILENAME_0XF0000)) {
        printf("ERROR: Failed reading file %s\n", BIOS_FILENAME_0XF0000);
        // return NULL;
    }
    if (EXIT_FAILURE == copy_bios_into_memory(memory, 0xF8000, BIOS_FILENAME_0XF8000)) {
        printf("ERROR: Failed reading file %s\n", BIOS_FILENAME_0XF8000);
        // return NULL;
    }
    // }
    MEMORY = memory;
    // return memory;
}

/* Retunrs an ID which can be used to unmap the device. In case of error returns 0 */
__declspec(dllexport)
uint32_t map_device(uint32_t start_addr, uint32_t end_addr, WRITE_FUNC_PTR(write_func), READ_FUNC_PTR(read_func)) {
    return 0;
}

__declspec(dllexport)
int module_tick(void) {
    return error;
}
