#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "8086.h"

int main(int argc, char *argv[]) {
    uint8_t continue_simulation = 0;
    if(argc > 1) {
        for(int i=1; i<argc; i++) {
            if(strcmp(argv[i], "--continue") == 0) {
                continue_simulation = 1;
            }
        }
    }
    if (EXIT_FAILURE == init_cpu(continue_simulation)) {
        return EXIT_FAILURE;
    }
    while (EXIT_SUCCESS == cpu_tick()) { // Run CPU
        sleep_ms(5);
		// clear_console();
    }
    // cpu_save_state();
    return 0;
}
