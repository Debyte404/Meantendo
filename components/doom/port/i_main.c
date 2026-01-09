/**
 * @file i_main.c
 * @brief DOOM Entry Point Wrapper for ESP32
 */

#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"

int myargc;
char** myargv;

void doom_main(int argc, char** argv) {
    myargc = argc;
    myargv = argv;
    D_DoomMain();
}
