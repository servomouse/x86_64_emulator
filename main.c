#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "8086.h"
#include "8086_io.h"
#include "devices/8259a_interrupt_controller.h"

#include "log_server_iface/logs_win.h"

int main(int argc, char *argv[]) {
    uint8_t continue_simulation = 0;
    if(argc > 1) {
        for(int i=1; i<argc; i++) {
            if(strcmp(argv[i], "--continue") == 0) {
                continue_simulation = 1;
            }
        }
    }
    log_server_init(8765);
    if (EXIT_FAILURE == init_cpu(continue_simulation)) {
        return EXIT_FAILURE;
    }
    if(EXIT_FAILURE == module_reset(continue_simulation)) {
        return EXIT_FAILURE;
    }
    int_controller_reset();
    // map_device(0xA0, 0xAF, &int_controller_write, &int_controller_read);
    while (EXIT_SUCCESS == cpu_tick()) { // Run CPU
        // sleep_ms(5);
		// clear_console();
    }
    log_server_close();
    sleep_ms(10000);
    // cpu_save_state();
    return 0;
}
