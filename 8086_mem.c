#include "8086_mem.h"
#include "utils.h"

// char *BIOS_FILENAME_0XF0000 = "BIOS/BIOS_5160_09MAY86_F0000.BIN";
// char *BIOS_FILENAME_0XF8000 = "BIOS/BIOS_5160_09MAY86_F8000.BIN";
char *BIOS_FILENAME_0XF0000 = "BIOS/F0000.BIN";
char *BIOS_FILENAME_0XF8000 = "BIOS/F8000.BIN";
#define MEMORY_LOG_FILE "logs/mem_log.txt"
#define MEMORY_DUMP_FILE "logs/mem_dump.bin"

#define MEMORY_SIZE 0x100000

static uint8_t *MEMORY = NULL;

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
        perror("Failed to open file");
        return  EXIT_FAILURE;
    }
    if (0x8000 < get_file_size(file)) {  // Check file size
        perror("File is too large!");
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
        perror("Failed to open file");
        return  EXIT_FAILURE;
    }
    fwrite(MEMORY, MEMORY_SIZE, 1, file);
    fclose(file);
    return EXIT_SUCCESS;
}

int restore_memory(uint8_t *memory, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Failed to open file");
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
    FILE *f;
    f = fopen(MEMORY_LOG_FILE, "a");
    if(f == NULL) {
        printf("ERROR: Failed to open %s", MEMORY_LOG_FILE);
        return NULL;
    }
    fprintf(f,"%s Memory log start\n", get_time());
    fclose(f);
    return memory;
}

void mem_write(uint32_t addr, uint16_t value, uint8_t width) {
    printf("MEM_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    if(width == 1) {
        MEMORY[addr] = value;
    } else if (width == 2) {
        *((uint16_t*)&MEMORY[addr]) = value;
    } else {
        printf("ERROR: Incorrect width: %d", width);
    }
    // FILE *f;
    // f = fopen(MEMORY_LOG_FILE, "a");
    // if(f == NULL) {
    //     printf("ERROR: Failed to open  %s", MEMORY_LOG_FILE);
    //     return;
    // }
    // fprintf(f,"MEM_WRITE Addr: 0x%06X; value: 0x%04X; width: %d\n", addr, value, width);
    // fclose(f);
}

uint16_t mem_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    if(width == 1) {
        ret_val = MEMORY[addr];
    } else if (width == 2) {
        ret_val = MEMORY[addr] + (MEMORY[addr+1] << 8);
    } else {
        mylog(1, "ERROR: Incorrect width: %d", width);
    }
    // FILE *f;
    // f = fopen(MEMORY_LOG_FILE, "a");
    // if(f == NULL) {
    //     printf("ERROR: Failed to open %s", MEMORY_LOG_FILE);
    //     return ret_val;
    // }
    // fprintf(f,"Read from addr: 0x%06X; value: 0x%04X; width: %d\n", addr, ret_val, width);
    // fclose(f);
    mylog(4, "MEM_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
    return ret_val;
}