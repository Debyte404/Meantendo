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

#include <stdlib.h>
#include <unistd.h>

#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

// Meantendo / Arduino Includes
// Note: We need to extern C these if they are C++ headers or use a C wrapper.
// Since this is a .c file, we can't include C++ headers directly unless we change extension.
// However, the project structure implies we can use `extern "C"` functions from main.

// Assume meantendo_display module or similar provides these C-compatible or extern C functions
void meantendo_display_init(void);
void meantendo_display_draw_frame(const uint8_t* screen_buffer);

// Screen dimensions
#define DOOM_WIDTH  320
#define DOOM_HEIGHT 200

// ESP32/ST7735 usually 160x128
#define ESP_WIDTH   160
#define ESP_HEIGHT  128

// Internal display buffer
static byte* screen_buffer = NULL;

void I_ShutdownGraphics(void)
{
    // Free any allocated memory if needed
    if (screen_buffer) {
        // Z_Free(screen_buffer); // If allocated with Z_Malloc
        screen_buffer = NULL;
    }
}

void I_StartFrame (void)
{
    // Optional: Input polling or setup for frame
}

void I_GetEvent(void)
{
    // Input is likely handled by a separate task or polling in loop.
    // If we need to pull events here (like in X11), we would call meantendo_input_poll();
    
    // Stub for now.
    event_t event;
    // ...
}

void I_StartTic (void)
{
    // Process events
    I_GetEvent();
}

void I_UpdateNoBlit (void)
{
    // Empty
}

// Downscale 320x200 -> 160x100 (preserve aspect, letterbox) or 160x128 (stretch/crop)?
// For simplicity, let's do a simple nearest neighbor center crop or downsample.
// Meantendo display likely expects a specific format.
// Assuming meantendo_display_draw_frame handles the framebuffer.
// If not, we do it here.

// Simple Direct Blit Implementation 
// DOOM screens[0] is 320x200 palettized (8-bit)
// We need to send this to ST7735. The ST7735 usually takes RGB565.
// DOES meantendo_display_draw_frame take 8-bit palettized or RGB565?
// Usually, we need to convert palette to RGB.

// Let's assume meantendo_display handles the conversion or we just send raw data for now
// and let the display driver handle it if it has a palette setup.
// OR we convert here.
// Given strict "remove X11" instructions, I will assume the meantendo_display component
// has a function to take the raw DOOM buffer or we assume a simple stub for now.

// Wait, looking at `Display.hpp`, it uses Adafruit_ST7735 which expects 16-bit color.
// We need the DOOM palette.
// DOOM's palette is updated via I_SetPalette.

static uint16_t palette_rgb565[256];

void I_FinishUpdate (void)
{
    // Don't draw if not ready
    if (!screen_buffer) return;

    // Convert 320x200 8-bit to ... 160x128 16-bit?
    // Let's implement a simple downsampler: Skip every other pixel and line.
    // 320 -> 160
    // 200 -> 100 (centered in 128 height)

    static uint16_t frame_rgb565[160 * 128]; // 40KB, fits in RAM
    
    const byte* src = screens[0];
    uint16_t* dst = frame_rgb565;

    // Clear top/bottom bars (14 lines each to center 100 in 128)
    // memset(frame_rgb565, 0, 160*128*2); 
    // Actually, let's just write the 100 lines in the middle.

    int y_offset = 14; 
    
    for (int y = 0; y < 100; y++) {
        for (int x = 0; x < 160; x++) {
            // Source coordinates: x*2, y*2
            // 320 width
            int src_idx = (y * 2) * 320 + (x * 2);
            byte color_idx = src[src_idx];
            
            // Map to centered destination
            int dst_idx = (y + y_offset) * 160 + x;
            frame_rgb565[dst_idx] = palette_rgb565[color_idx];
        }
    }

    // Now send frame_rgb565 to display
    // We can't call C++ object methods directly here easily (it's a .c file).
    // So we use the extern wrapper we assumed exists or we define a helper in a .cpp file.
    // For this task, I'll add the extern declaration and call it.
    
    // extern void meantendo_video_flush(uint16_t* buffer, int width, int height);
    // meantendo_video_flush(frame_rgb565, 160, 128);
    
    // TEMPORARY: Just stub or direct call if we can link.
    // I will add a declaration for a function I'll add to a cpp wrapper later.
    extern void Meantendo_FlushDisplay(uint16_t* buffer);
    Meantendo_FlushDisplay(frame_rgb565);
}

void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

void I_SetPalette (byte* palette)
{
    // Convert RGB24 palette to RGB565
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
    // Initialize display system
    // Handle in main setup usually, but we can signal init here
    extern void Meantendo_InitDisplay();
    Meantendo_InitDisplay();
    
    screen_buffer = screens[0]; // DOOM engine allocates screens[0]
}
	| KeyReleaseMask
	// | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
	| ExposureMask;

    attribs.colormap = X_cmap;
    attribs.border_pixel = 0;

    // create the main window
    X_mainWindow = XCreateWindow(	X_display,
					RootWindow(X_display, X_screen),
					x, y,
					X_width, X_height,
					0, // borderwidth
					8, // depth
					InputOutput,
					X_visual,
					attribmask,
					&attribs );

    XDefineCursor(X_display, X_mainWindow,
		  createnullcursor( X_display, X_mainWindow ) );

    // create the GC
    valuemask = GCGraphicsExposures;
    xgcvalues.graphics_exposures = False;
    X_gc = XCreateGC(	X_display,
  			X_mainWindow,
  			valuemask,
  			&xgcvalues );

    // map the window
    XMapWindow(X_display, X_mainWindow);

    // wait until it is OK to draw
    oktodraw = 0;
    while (!oktodraw)
    {
	XNextEvent(X_display, &X_event);
	if (X_event.type == Expose
	    && !X_event.xexpose.count)
	{
	    oktodraw = 1;
	}
    }

    // grabs the pointer so it is restricted to this window
    if (grabMouse)
	XGrabPointer(X_display, X_mainWindow, True,
		     ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
		     GrabModeAsync, GrabModeAsync,
		     X_mainWindow, None, CurrentTime);

    if (doShm)
    {

	X_shmeventtype = XShmGetEventBase(X_display) + ShmCompletion;

	// create the image
	image = XShmCreateImage(	X_display,
					X_visual,
					8,
					ZPixmap,
					0,
					&X_shminfo,
					X_width,
					X_height );

	grabsharedmemory(image->bytes_per_line * image->height);


	// UNUSED
	// create the shared memory segment
	// X_shminfo.shmid = shmget (IPC_PRIVATE,
	// image->bytes_per_line * image->height, IPC_CREAT | 0777);
	// if (X_shminfo.shmid < 0)
	// {
	// perror("");
	// I_Error("shmget() failed in InitGraphics()");
	// }
	// fprintf(stderr, "shared memory id=%d\n", X_shminfo.shmid);
	// attach to the shared memory segment
	// image->data = X_shminfo.shmaddr = shmat(X_shminfo.shmid, 0, 0);
	

	if (!image->data)
	{
	    perror("");
	    I_Error("shmat() failed in InitGraphics()");
	}

	// get the X server to attach to it
	if (!XShmAttach(X_display, &X_shminfo))
	    I_Error("XShmAttach() failed in InitGraphics()");

    }
    else
    {
	image = XCreateImage(	X_display,
    				X_visual,
    				8,
    				ZPixmap,
    				0,
    				(char*)malloc(X_width * X_height),
    				X_width, X_height,
    				8,
    				X_width );

    }

    if (multiply == 1)
	screens[0] = (unsigned char *) (image->data);
    else
	screens[0] = (unsigned char *) malloc (SCREENWIDTH * SCREENHEIGHT);

}


unsigned	exptable[256];

void InitExpand (void)
{
    int		i;
	
    for (i=0 ; i<256 ; i++)
	exptable[i] = i | (i<<8) | (i<<16) | (i<<24);
}

double		exptable2[256*256];

void InitExpand2 (void)
{
    int		i;
    int		j;
    // UNUSED unsigned	iexp, jexp;
    double*	exp;
    union
    {
	double 		d;
	unsigned	u[2];
    } pixel;
	
    printf ("building exptable2...\n");
    exp = exptable2;
    for (i=0 ; i<256 ; i++)
    {
	pixel.u[0] = i | (i<<8) | (i<<16) | (i<<24);
	for (j=0 ; j<256 ; j++)
	{
	    pixel.u[1] = j | (j<<8) | (j<<16) | (j<<24);
	    *exp++ = pixel.d;
	}
    }
    printf ("done.\n");
}

int	inited;

void
Expand4
( unsigned*	lineptr,
  double*	xline )
{
    double	dpixel;
    unsigned	x;
    unsigned 	y;
    unsigned	fourpixels;
    unsigned	step;
    double*	exp;
	
    exp = exptable2;
    if (!inited)
    {
	inited = 1;
	InitExpand2 ();
    }
		
		
    step = 3*SCREENWIDTH/2;
	
    y = SCREENHEIGHT-1;
    do
    {
	x = SCREENWIDTH;

	do
	{
	    fourpixels = lineptr[0];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[0] = dpixel;
	    xline[160] = dpixel;
	    xline[320] = dpixel;
	    xline[480] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[1] = dpixel;
	    xline[161] = dpixel;
	    xline[321] = dpixel;
	    xline[481] = dpixel;

	    fourpixels = lineptr[1];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[2] = dpixel;
	    xline[162] = dpixel;
	    xline[322] = dpixel;
	    xline[482] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[3] = dpixel;
	    xline[163] = dpixel;
	    xline[323] = dpixel;
	    xline[483] = dpixel;

	    fourpixels = lineptr[2];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[4] = dpixel;
	    xline[164] = dpixel;
	    xline[324] = dpixel;
	    xline[484] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[5] = dpixel;
	    xline[165] = dpixel;
	    xline[325] = dpixel;
	    xline[485] = dpixel;

	    fourpixels = lineptr[3];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[6] = dpixel;
	    xline[166] = dpixel;
	    xline[326] = dpixel;
	    xline[486] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[7] = dpixel;
	    xline[167] = dpixel;
	    xline[327] = dpixel;
	    xline[487] = dpixel;

	    lineptr+=4;
	    xline+=8;
	} while (x-=16);
	xline += step;
    } while (y--);
}


