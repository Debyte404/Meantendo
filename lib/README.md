# Meantendo — Local Libraries

This directory is for project-specific static libraries that get compiled and linked into the firmware.

## Structure

```
lib/
└── README.md          ← you are here
```

## Adding a Library

Place each library in its own subdirectory:

```
lib/
├── MyLib/
│   ├── src/
│   │   ├── MyLib.cpp
│   │   └── MyLib.hpp
│   └── library.json   ← optional
└── README.md
```

PlatformIO's Library Dependency Finder (LDF) will scan `src/` for `#include` directives and automatically link any matching libraries found here.

## Note

Most of Meantendo's code lives directly in `src/core/` and `src/games/` as directly-compiled `.cpp` files. This `lib/` directory is reserved for third-party or reusable components that need separate compilation.