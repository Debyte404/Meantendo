/**
 * @file i_video.c
 * @brief DOOM Video Interface for ESP32
 */

#include "doomdef.h"
#include "d_main.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "m_argv.h"

#include "display.h" 
#include "input.h"

#define DOWN_SCALE 2 

// Render buffer (320x200 palette indices)
// We need to map this to our 160x128 RGB565 display
// Since 320/2 = 160, and 200/2 = 100, we fit perfectly horizontally, and leave 14 px top/bottom black.

static byte* s_video_buffer = NULL; // 320*200 bytes

// Palette handling
static uint16_t s_palette_rgb565[256];

void I_InitGraphics(void) {
    // Allocation done by DOOM engine usually? 
    // screens[0] is assigned by the engine if we don't do it.
    // But we need to set it up.
    
    // I_InitGraphics is called by D_DoomMain
}

void I_ShutdownGraphics(void) {
    // Nothing to do
}

void I_SetPalette(byte* palette) {
    // Convert palette to RGB565 table
    for (int i = 0; i < 256; i++) {
        uint8_t r = gammatable[usegamma][*palette++];
        uint8_t g = gammatable[usegamma][*palette++];
        uint8_t b = gammatable[usegamma][*palette++];
        
        s_palette_rgb565[i] = display_color565(r, g, b);
    }
}

void I_UpdateNoBlit(void) {}

void I_FinishUpdate(void) {
    // screens[0] contains the 320x200 frame (palettized)
    byte* src = screens[0];
    
    // We downscale to 160x100
    // Simple nearest neighbor: skip every other pixel/line
    
    // display_blit expects RGB565 buffer.
    // We can blit line by line to save memory, or reuse a buffer.
    // We have a 160x100 output area.
    // We can render directly to the LCD line by line using display_draw_rgb565 or similar
    // BUT meantendo_display has display_blit that takes a buffer.
    
    // Let's allocate a line buffer for 160 pixels
    uint16_t line_buffer[160];
    
    // Center vertically: (128 - 100) / 2 = 14
    int start_y = 14; 
    
    for (int y = 0; y < 100; y++) {
        // Source Y is y*2
        byte* src_row = src + (y * 2 * SCREENWIDTH);
        
        for (int x = 0; x < 160; x++) {
            // Source X is x*2
            byte pixel = src_row[x * 2];
            line_buffer[x] = s_palette_rgb565[pixel];
        }
        
        // Draw the line
        display_blit(0, start_y + y, 160, 1, line_buffer);
    }
    
    // Display status bar? Status bar is at bottom of 320x200.
    // It is included in the 200 height.
    // So the downscale covers it.
}

void I_ReadScreen(byte* scr) {
    memcpy(scr, screens[0], SCREENWIDTH * SCREENHEIGHT);
}

void I_StartTic(void) {
    // Read input and post events
    
    event_t event;
    
    // Map input to DOOM events
    // We read our input system
    
    // Simple mapping:
    // Joy Up -> Forward
    // Joy Down -> Back
    // Joy Left -> Turn Left
    // Joy Right -> Turn Right
    // Btn A -> Fire (CTRL)
    // Btn B -> Open (SPACE)
    // Btn X -> Strafe?
    // Btn Y -> Speed?
    // Select -> Enter
    // Back -> Escape
    
    input_direction_t dir = input_read_joystick();
    
    // We need to post key down/up events. 
    // This is tricky with continuous joystick.
    // DOOM handles keys.
    
    // Let's simulate keys.
    static int s_joy_state = 0; 
    
    // Helper to post event
    void PostKey(int key, boolean down) {
        event.type = down ? ev_keydown : ev_keyup;
        event.data1 = key;
        D_PostEvent(&event);
    }
    
    // Check Up
    boolean up = (dir == INPUT_DIR_UP);
    static boolean last_up = false;
    if (up != last_up) { PostKey(KEY_UPARROW, up); last_up = up; }

    boolean down = (dir == INPUT_DIR_DOWN);
    static boolean last_down = false;
    if (down != last_down) { PostKey(KEY_DOWNARROW, down); last_down = down; }
    
    boolean left = (dir == INPUT_DIR_LEFT);
    static boolean last_left = false;
    if (left != last_left) { PostKey(KEY_LEFTARROW, left); last_left = left; }

    boolean right = (dir == INPUT_DIR_RIGHT);
    static boolean last_right = false;
    if (right != last_right) { PostKey(KEY_RIGHTARROW, right); last_right = right; }
    
    // Buttons
    boolean btn_fire = input_button_pressed(BTN_A);
    static boolean last_fire = false;
    if (btn_fire != last_fire) { PostKey(KEY_RCTRL, btn_fire); last_fire = btn_fire; } // Fire
    
    boolean btn_use = input_button_pressed(BTN_B);
    static boolean last_use = false;
    if (btn_use != last_use) { PostKey(' ', btn_use); last_use = btn_use; } // Use
    
    boolean btn_esc = input_button_pressed(BTN_BACK);
    static boolean last_esc = false;
    if (btn_esc != last_esc) { PostKey(KEY_ESCAPE, btn_esc); last_esc = btn_esc; } // Menu
    
    boolean btn_enter = input_button_pressed(BTN_SELECT);
    static boolean last_enter = false;
    if (btn_enter != last_enter) { PostKey(KEY_ENTER, btn_enter); last_enter = btn_enter; } // Select
    
    // Strafe On? Button X
    boolean btn_strafe = input_button_pressed(BTN_X);
    static boolean last_strafe = false;
    if (btn_strafe != last_strafe) { PostKey(KEY_RALT, btn_strafe); last_strafe = btn_strafe; }
    
}

void I_StartFrame(void) {}
