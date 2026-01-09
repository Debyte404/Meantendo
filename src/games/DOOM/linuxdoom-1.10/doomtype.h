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
// DESCRIPTION:
//	Simple basic typedefs, isolated here to make it easier
//	 separating modules.
//    
//-----------------------------------------------------------------------------


#ifndef __DOOMTYPE__
#define __DOOMTYPE__


#ifndef __BYTEBOOL__
#define __BYTEBOOL__
// Fixed to use builtin bool type with C++.
#ifdef __cplusplus
typedef bool boolean;
#else
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif
typedef enum {false, true} boolean;
#endif
typedef unsigned char byte;
#endif



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef MAXINT
#define MAXINT INT_MAX
#endif

#ifndef MININT
#define MININT INT_MIN
#endif

#ifndef MAXLONG
#define MAXLONG LONG_MAX
#endif

#ifndef MINLONG
#define MINLONG LONG_MIN
#endif

#ifndef MAXCHAR
#define MAXCHAR SCHAR_MAX
#endif

#ifndef MINCHAR
#define MINCHAR SCHAR_MIN
#endif

#ifndef MAXSHORT
#define MAXSHORT SHRT_MAX
#endif

#ifndef MINSHORT
#define MINSHORT SHRT_MIN
#endif




#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
