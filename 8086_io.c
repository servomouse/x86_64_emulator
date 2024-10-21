#include "8086_io.h"
// #include "8086_mda.h"
// #include "8086_ppi.h"
// #include "8253_timer.h"
#include "utils.h"
#include <string.h>

#define IO_LOG_FILE "logs/io_log.txt"
#define IO_DUMP_FILE "data/io_dump.bin"
#define IO_SPACE_SIZE 0x100000

typedef struct {
    uint32_t id;
    uint32_t start;
    uint32_t end;
    WRITE_FUNC_PTR(write_func);
    READ_FUNC_PTR(read_func);
} map_t;

typedef struct {
    uint32_t size;          // available size
    uint32_t num_devices;   // how many devices are mapped
    map_t *dev_table;
} dev_table_t;

uint8_t *IO_SPACE = NULL;
dev_table_t io_space;
int io_error = 0;

static uint32_t get_id(uint32_t val1, uint32_t val2) {
    uint32_t id = (val1 & 0xFFFF) | ((val2 & 0xFFFF) << 16);
    return id;
}

/* Checks if the new address range overlaps one of existing, returns range id if there is an overlap, othervwise returns 0 */
static uint8_t range_overlaps(uint32_t start, uint32_t end) {
    if(io_space.dev_table == NULL) {
        return 0;
    }
    for(uint32_t i=0; i<io_space.num_devices; i++) {
        if(io_space.dev_table[i].id == 0) {    // device was unmapped
            continue;
        }
        if(io_space.dev_table[i].start <= start && io_space.dev_table[i].end >= start) {
            return 1;
        }
        if(io_space.dev_table[i].start <= end && io_space.dev_table[i].end >= end) {
            return 1;
        }
    }
    return 0;
}

static void add_device(uint32_t index, uint32_t id, uint32_t start_addr, uint32_t end_addr, WRITE_FUNC_PTR(write_func), READ_FUNC_PTR(read_func)) {
    io_space.dev_table[index].start = start_addr;
    io_space.dev_table[index].end = end_addr;
    io_space.dev_table[index].id = id;
    io_space.dev_table[index].write_func = write_func;
    io_space.dev_table[index].read_func = read_func;
}

static uint8_t check_allocation(void) {
    if(io_space.dev_table == NULL) {
        io_space.dev_table = calloc(sizeof(map_t), 10);
        if(io_space.dev_table == NULL) {
            printf("ERROR: Cannot allocate memory: %s, %d", __FILE__, __LINE__);
            io_error = 1;
            return 0;
        }
        io_space.size = 10;
        io_space.num_devices = 0;
    }
    if(io_space.num_devices == io_space.size) {
        io_space.dev_table = realloc(io_space.dev_table, sizeof(map_t) * (10 + io_space.size));
        if(io_space.dev_table == NULL) {
            printf("ERROR: Cannot allocate memory: %s, %d", __FILE__, __LINE__);
            io_error = 1;
            return 0;
        }
    }
    return 1;
}

/* Retunrs an ID which can be used to unmap the device. In case of error returns 0 */
__declspec(dllexport)
uint32_t map_device(uint32_t start_addr, uint32_t end_addr, WRITE_FUNC_PTR(write_func), READ_FUNC_PTR(read_func)) {
    uint32_t id = get_id(start_addr, end_addr);
    uint32_t overlaps_id = range_overlaps(start_addr, end_addr);
    if(range_overlaps(start_addr, end_addr)) {
        printf("MAPPING ERROR: new device ranges 0x%08X overlaps existing 0x%08X", id, overlaps_id);
        io_error = 1;
        return 0;
    }
    if(!check_allocation()) {   // Allocation failed
        return 0;
    }
    uint8_t found_free_cell = 0;
    for(uint32_t i=0; i<io_space.num_devices; i++) {
        if(io_space.dev_table[i].id == 0) {    // device was unmapped
            add_device(i, id, start_addr, end_addr, write_func, read_func);
            found_free_cell = 1;
            break;
        }
    }
    if(!found_free_cell) {
        add_device(io_space.num_devices, id, start_addr, end_addr, write_func, read_func);
        io_space.num_devices += 1;
    }
    if(!check_allocation()) {   // Allocation failed
        return 0;
    }
    mylog(IO_LOG_FILE, "Device 0x%08X was successfully mapped at addressed 0x%04X-0x%04X\n", id, start_addr, end_addr);
    return id;
}

/* Use the ID returned by the map_device function to unmap it */
__declspec(dllexport)
void unmap_device(uint32_t id) {
    for(uint32_t i=0; i<io_space.num_devices; i++) {
        if(io_space.dev_table[i].id == id) {
            io_space.dev_table[i].id = 0;
            mylog(IO_LOG_FILE, "Device 0x%08X was successfully unmapped\n", id);
        }
    }
}

__declspec(dllexport)
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    uint8_t addr_found = 0;
    for(uint32_t i=0; i<io_space.num_devices; i++) {
        if(io_space.dev_table[i].id == 0) {
            continue;
        }
        if((io_space.dev_table[i].start) <= addr && (io_space.dev_table[i].end >= addr)) {
            io_space.dev_table[i].write_func(addr, value, width);
            addr_found = 1;
        }
    }
    if(!addr_found) {
        printf("IO_WRITE ERROR: No device at address 0x%08X!\n", addr);
        io_error = 1;
    }
    mylog(IO_LOG_FILE, "IO_WRITE addr = 0x%04X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    
    // if ((addr >= 0x3B0) && (addr <= 0x3DC)) {
    //     mda_write(addr, value, width);
    // } else if ((addr >= 0x40) && (addr <= 0x43)) {
    //     timer_write(addr, value, width);
    // } else if ((addr >= 0x60) && (addr <= 0x63)) {
    //     ppi_write(addr, value, width);
    // } else {
    //     mylog(IO_LOG_FILE, "IO_WRITE addr = 0x%04X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    // }
    // if(width == 1) {
    //     IO_SPACE[addr] = value;
    // } else if (width == 2) {
    //     *((uint16_t*)&IO_SPACE[addr]) = value;
    // } else {
    //     printf("IO WRITE ERROR: Incorrect width: %d", width);
    // }
}

// uint8_t counter = 0xFF;

__declspec(dllexport)
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0xFF;
    if(width == 2) {
        ret_val = 0xFFFF;
    }
    uint8_t addr_found = 0;
    
    for(uint32_t i=0; i<io_space.num_devices; i++) {
        if(io_space.dev_table[i].id == 0) {
            continue;
        }
        if((io_space.dev_table[i].start) <= addr && (io_space.dev_table[i].end >= addr)) {
            ret_val = io_space.dev_table[i].read_func(addr, width);
            addr_found = 1;
        }
    }
    if(!addr_found) {
        printf("IO_READ ERROR: No device at address 0x%08X!\n", addr);
        io_error = 1;
    }
    mylog(IO_LOG_FILE, "IO_READ addr = 0x%04X, width = %d bytes value: 0x%04X\n", addr, width, ret_val);
    // if(width == 1) {
    //     ret_val = 0xFF;
    // } else if (width == 2) {
    //     ret_val = 0xFFFF;
    // } else {
    //     printf("ERROR: Incorrect width while readiong from IO: %d", width);
    //     ret_val = 0xFFFF;
    // }

    // if ((addr >= 0x3B0) && (addr <= 0x3DC)) {
    //     ret_val = mda_read(addr, width);
    // } else if ((addr >= 0x40) && (addr <= 0x43)) {
    //     ret_val = timer_read(addr, width);
    // } else if ((addr >= 0x60) && (addr <= 0x63)) {
    //     ret_val = ppi_read(addr, width);
    // } else {
    //     if(width == 1) {
    //         ret_val = IO_SPACE[addr];
    //     } else if (width == 2) {
    //         ret_val = IO_SPACE[addr] + (IO_SPACE[addr+1] << 8);
    //     } else {
    //         printf("IO READ ERROR: Incorrect width: %d", width);
    //     }
    // }
    // mylog(IO_LOG_FILE, "IO_READ addr = 0x%04X, width = %d bytes value: 0x%04X\n", addr, width, ret_val);
    return ret_val;
}

int store_io(void) {
    FILE *file = fopen(IO_DUMP_FILE, "wb");
    if (file == NULL) {
        printf("Failed to open file %s\n", IO_DUMP_FILE);
        io_error = 1;
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
        io_error = 1;
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

__declspec(dllexport)
void module_reset(void) {
    IO_SPACE = (uint8_t*)calloc(sizeof(uint8_t), IO_SPACE_SIZE);
    // mda_init();
    // ppi_init();
    // timer_init();
    if(IO_SPACE == NULL) {
        printf("IO ERROR: Failed to allocate memory\n");
        io_error = 1;
        return;
    }
    // if(continue_simulation) {
    //     restore_io(IO_SPACE, IO_DUMP_FILE);
    // }
    // FILE *f;
    // f = fopen(IO_LOG_FILE, "a");
    // if(f == NULL) {
    //     printf("ERROR: Failed to open %s", IO_LOG_FILE);
    //     return EXIT_FAILURE;
    // }
    // fprintf(f, "%s I/O log start\n", get_time());
    // fclose(f);
    // return EXIT_SUCCESS;
}

int get_io_error(void) {
    return io_error;
}

__declspec(dllexport)
void module_save(void) {
    store_data(IO_SPACE, IO_SPACE_SIZE, IO_DUMP_FILE);
}

__declspec(dllexport)
void module_restore(void) {
    uint8_t temp[IO_SPACE_SIZE] = {0};
    if(EXIT_SUCCESS == restore_data(temp, IO_SPACE_SIZE, IO_DUMP_FILE)) {
        memcpy(IO_SPACE, temp, IO_SPACE_SIZE);
    }
}

__declspec(dllexport)
int module_tick(void) {
    return 0;
}