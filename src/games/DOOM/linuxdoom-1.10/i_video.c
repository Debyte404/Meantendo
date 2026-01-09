// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for ESP32 (ST7735).
//
//-----------------------------------------------------------------------------

#ifndef ESP32
#define ESP32 1
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

// Meantendo / Arduino Includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

// External bindings to C++ Display class
// Defined in src/core/DisplayAdaptor.cpp
void Meantendo_InitDisplay(void);
void Meantendo_FlushDisplay(uint16_t* buffer);

// DOOM render buffer dimensions
#define DOOM_WIDTH  320
#define DOOM_HEIGHT 200

// Internal display buffer
// DOOM engine uses screens[0] as the primary framebuffer (8-bit palettized)
// We don't necessarily need to malloc it if we want to use static memory,
// but `screens[0]` is a pointer, so malloc is fine.
static byte* screen_buffer = NULL;

// Palette for RGB565 conversion
static uint16_t palette_rgb565[256];

void I_ShutdownGraphics(void)
{
    if (screen_buffer) {
        free(screen_buffer);
        screen_buffer = NULL;
    }
}

void I_StartFrame (void)
{
    // Input polling could happen here
}

void I_GetEvent(void)
{
    event_t event;
    // Stub for input event processing
    // meantendo_input_poll(&event);
}

void I_StartTic (void)
{
    I_GetEvent();
}

void I_UpdateNoBlit (void)
{
    // Empty
}

// Convert 8-bit Doom buffer to 16-bit RGB565 and send to display
void I_FinishUpdate (void)
{
    if (!screen_buffer) return;

    // Target: 160x128 ST7735
    // Simple downscale: Center crop 160x100 from 320x200?
    // Or nearest neighbor downscale 320x200 -> 160x100?
    // 320/2 = 160. 200/2 = 100.
    // We have 128 height, so we center the 100 height image (14 pixels top/bottom padding)

    static uint16_t frame_rgb565[160 * 128]; 
    
    const byte* src = screens[0];
    
    // Clear buffer (black) - optional if filling fully, but we have letterboxing
    // memset(frame_rgb565, 0, sizeof(frame_rgb565));

    int y_offset = 14; 
    
    // Fill the 100 lines of content
    for (int y = 0; y < 100; y++) {
        int src_y = y * 2;
        int dst_y = y + y_offset;
        
        for (int x = 0; x < 160; x++) {
            int src_x = x * 2;
            
            // Sample from DOOM buffer
            byte color_idx = src[src_y * DOOM_WIDTH + src_x];
            
            // Convert to RGB565
            frame_rgb565[dst_y * 160 + x] = palette_rgb565[color_idx];
        }
    }
    
    // Fill top/bottom bars with black (palette index 0 usually black?)
    // Or just rely on previous clear or specific black color
    // Let's ensure the borders are black 0x0000
    // Top bar
    memset(frame_rgb565, 0, 160 * 14 * 2);
    // Bottom bar
    memset(frame_rgb565 + (114 * 160), 0, 160 * 14 * 2);

    Meantendo_FlushDisplay(frame_rgb565);
}

void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], DOOM_WIDTH * DOOM_HEIGHT);
}

void I_SetPalette (byte* palette)
{
    for (int i=0 ; i<256 ; i++)
    {
        byte r = gammatable[usegamma][*palette++];
        byte g = gammatable[usegamma][*palette++];
        byte b = gammatable[usegamma][*palette++];
        
        // RGB565: RRRRRGGGGGGBBBBB
        palette_rgb565[i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
}

void I_InitGraphics(void)
{
    // Initialize Hardware
    Meantendo_InitDisplay();
    
    // Allocate DOOM framebuffer (320x200 8-bit)
    if (!screen_buffer) {
        screen_buffer = (byte*) malloc(DOOM_WIDTH * DOOM_HEIGHT);
        if (!screen_buffer) {
            I_Error("I_InitGraphics: Couldn't allocate framebuffer");
        }
    }
    
    // Clear it
    memset(screen_buffer, 0, DOOM_WIDTH * DOOM_HEIGHT);

    // Assign to DOOM global
    screens[0] = screen_buffer;
}
