#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Floppy Disk controller (NEC uPD765) p163

void module_reset(void);
void data_write(uint32_t addr, uint16_t value, uint8_t width);
uint16_t data_read(uint32_t addr, uint8_t width);
void module_save(void);
void module_restore(void);
int module_tick(uint32_t ticks);

// Page 17 of Datasheet
// Status 0 register bits: 7 6
//                         0 0 - Normal termination of a command
//                         0 1 - Abnormal termination of command (command execution started, but wasn't successfully completed)
//                         1 0 - Invalid Command Issue - Issue was never started
//                         1 1 - Abnormal Termination because during execution FDD ready signal changed state
//                     bit 5: Seek End: When FDC completes the SEEK command, this bit is set to 1
//                     bit 4: Equipment check: This bit is set if a fault signal is received from FDD
//                                             or if the Track 0 signal fails to occur during Recalibrate command
//                     bit 3: Not Ready Flag
//                     bit 2: Head Address: This bit is used to indicate the state of the head at interrupt
//                     bit 1: Unit Select 1 These bits are used to the Drive unit number at interrupt
// MSB                 bit 1: Unit Select 0

// Status 1 register bits: