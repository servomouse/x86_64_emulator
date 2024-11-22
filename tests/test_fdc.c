#include "FDC.h"
#include "utils.h"
#include "wires.h"
#include "test_fdc.h"
#include <string.h>

int main(void) {
    module_reset();
    uint8_t dor = data_read(0x3F2, 1);
    uint8_t msr = data_read(0x3F4, 1);
    uint8_t data = data_read(0x3F5, 1);
    if(dor != 0) {
        printf("ERROR: DOR != 0!; DOR = 0x%02X\n", dor);
    }
    if(msr != 0) {
        printf("ERROR: MSR != 0!; MSR = 0x%02X\n", msr);
    }
    if(data != 0) {
        printf("ERROR: DATA != 0!; DATA = 0x%02X\n", data);
    }
    data_write(0x3F2, 0xA5, 1);
    data_write(0x3F4, 0xA5, 1);
    data_write(0x3F5, 0xA5, 1);

    dor = data_read(0x3F2, 1);
    msr = data_read(0x3F4, 1);
    data = data_read(0x3F5, 1);
    if(dor != 0xA5) {
        printf("ERROR: DOR != 0xA5!; DOR = 0x%02X\n", dor);
    }
    if(msr != 0xA5) {
        printf("ERROR: MSR != 0xA5!; MSR = 0x%02X\n", msr);
    }
    if(data != 0xA5) {
        printf("ERROR: DATA != 0xA5!; DATA = 0x%02X\n", data);
    }
    module_save();
    module_reset();
    dor = data_read(0x3F2, 1);
    msr = data_read(0x3F4, 1);
    data = data_read(0x3F5, 1);
    if(dor != 0) {
        printf("ERROR: DOR != 0!; DOR = 0x%02X\n", dor);
    }
    if(msr != 0) {
        printf("ERROR: MSR != 0!; MSR = 0x%02X\n", msr);
    }
    if(data != 0) {
        printf("ERROR: DATA != 0!; DATA = 0x%02X\n", data);
    }
    module_restore();
    dor = data_read(0x3F2, 1);
    msr = data_read(0x3F4, 1);
    data = data_read(0x3F5, 1);
    if(dor != 0xA5) {
        printf("ERROR: DOR != 0xA5!; DOR = 0x%02X\n", dor);
    }
    if(msr != 0xA5) {
        printf("ERROR: MSR != 0xA5!; MSR = 0x%02X\n", msr);
    }
    if(data != 0xA5) {
        printf("ERROR: DATA != 0xA5!; DATA = 0x%02X\n", data);
    }
}