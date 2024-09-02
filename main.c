#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

char *BIOS_FILENAME_0XF0000 = "BIOS/F0000.BIN";
char *BIOS_FILENAME_0XF8000 = "BIOS/F8000.BIN";

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
        perror("Fale is too large!");
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

int main(void) {
    uint8_t memory[0x100000] = {0};
    if ((EXIT_SUCCESS == copy_bios_into_memory(memory, 0xF0000, BIOS_FILENAME_0XF0000)) &&
        (EXIT_SUCCESS == copy_bios_into_memory(memory, 0xF8000, BIOS_FILENAME_0XF8000))) {
            for (int i=0; i<16; i++) {
                printf("0x%02X ", memory[0xFFFF0 + i]);
            }
            printf("\n");
        }
    return 0;
}