/**
 * @file i_net.c
 * @brief DOOM Network Interface Stubs for ESP32
 */

#include "i_net.h"
#include "d_net.h"
#include "doomdef.h"

void I_InitNetwork(void) {
    doomcom = malloc(sizeof(*doomcom));
    memset(doomcom, 0, sizeof(*doomcom));
    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = 1;
    doomcom->numnodes = 1;
    doomcom->deathmatch = 0;
    doomcom->consoleplayer = 0;
    doomcom->ticdup = 1;
    doomcom->extratics = 0;
}

void I_NetCmd(void) {}
