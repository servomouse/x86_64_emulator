#include "8086_io.h"
#include "8086_mda.h"
#include "8086_ppi.h"
#include "8086_timer.h"
#include "utils.h"

#define IO_LOG_FILE "logs/io_log.txt"
#define IO_DUMP_FILE "logs/io_dump.bin"
#define IO_SPACE_SIZE 0x100000

uint8_t *IO_SPACE = NULL;

void io_write(uint32_t addr, uint16_t value, uint8_t width) {
    
    if ((addr >= 0x3B0) && (addr <= 0x3DC)) {
        mda_write(addr, value, width);
    } else if ((addr >= 0x40) && (addr <= 0x43)) {
        timer_write(addr, value, width);
    } else if ((addr >= 0x60) && (addr <= 0x63)) {
        ppi_write(addr, value, width);
    } else {
        mylog(IO_LOG_FILE, "IO_WRITE addr = 0x%04X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    }
    if(width == 1) {
        IO_SPACE[addr] = value;
    } else if (width == 2) {
        *((uint16_t*)&IO_SPACE[addr]) = value;
    } else {
        printf("IO WRITE ERROR: Incorrect width: %d", width);
    }
    // FILE *f;
    // f = fopen(IO_LOG_FILE, "a");
    // if(f == NULL) {
    //     printf("ERROR: Failed to open %s", IO_LOG_FILE);
    //     return;
    // }
    // fprintf(f, "Write to  addr: 0x%06X; value: 0x%04X; width: %d\n", addr, value, width);
    // fclose(f);
}

uint8_t counter = 0xFF;

uint16_t io_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    // if(width == 1) {
    //     ret_val = 0xFF;
    // } else if (width == 2) {
    //     ret_val = 0xFFFF;
    // } else {
    //     printf("ERROR: Incorrect width while readiong from IO: %d", width);
    //     ret_val = 0xFFFF;
    // }

    if ((addr >= 0x3B0) && (addr <= 0x3DC)) {
        ret_val = mda_read(addr, width);
    } else if ((addr >= 0x40) && (addr <= 0x43)) {
        ret_val = timer_read(addr, width);
    } else if ((addr >= 0x60) && (addr <= 0x63)) {
        ret_val = ppi_read(addr, width);
    } else {
        if(width == 1) {
            ret_val = IO_SPACE[addr];
        } else if (width == 2) {
            ret_val = IO_SPACE[addr] + (IO_SPACE[addr+1] << 8);
        } else {
            printf("IO READ ERROR: Incorrect width: %d", width);
        }
        // if(addr == 0x0041) {
        //     IO_SPACE[addr] = 0xFF & (ret_val + 1);
        // }
    }
    mylog(IO_LOG_FILE, "IO_READ addr = 0x%04X, width = %d bytes value: 0x%04X\n", addr, width, ret_val);
    
    // FILE *f;
    // f = fopen(IO_LOG_FILE, "a");
    // if(f == NULL) {
    //     printf("ERROR: Failed to open %s", IO_LOG_FILE);
    //     return ret_val;
    // }
    // fprintf(f, "Read from addr: 0x%06X; value: 0x%04X; width: %d\n", addr, ret_val, width);
    // fclose(f);
    return ret_val;
}

int store_io(void) {
    FILE *file = fopen(IO_DUMP_FILE, "wb");
    if (file == NULL) {
        printf("Failed to open file %s\n", IO_DUMP_FILE);
        return  EXIT_FAILURE;
    }
    fwrite(IO_SPACE, IO_SPACE_SIZE, 1, file);
    fclose(file);
    return EXIT_SUCCESS;
}

int restore_io(uint8_t *io_space, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Failed to open file %s\n", filename);
        return  EXIT_FAILURE;
    }
    uint8_t *ptr = io_space;
    size_t bytes_read = 0;
    while((bytes_read = fread(ptr, 1, 1, file)) > 0) {
        ptr++;
    }
    fclose(file);
    return EXIT_SUCCESS;
}

int io_init(uint8_t continue_simulation) {
    IO_SPACE = (uint8_t*)calloc(sizeof(uint8_t), IO_SPACE_SIZE);
    mda_init();
    ppi_init();
    timer_init();
    if(IO_SPACE == NULL) {
        printf("IO ERROR: Failed to allocate memory\n");
        return EXIT_FAILURE;
    }
    if(continue_simulation) {
        restore_io(IO_SPACE, IO_DUMP_FILE);
    }
    // FILE *f;
    // f = fopen(IO_LOG_FILE, "a");
    // if(f == NULL) {
    //     printf("ERROR: Failed to open %s", IO_LOG_FILE);
    //     return EXIT_FAILURE;
    // }
    // fprintf(f, "%s I/O log start\n", get_time());
    // fclose(f);
    return EXIT_SUCCESS;
}