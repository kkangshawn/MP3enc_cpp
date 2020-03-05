# MP3enc_cpp

WAV to MP3 Encoder, written in C++
---------------------------------------------

- uses LAME library (https://lame.sourceforge.io/)
- supports encoding multiple files using pthread by putting input in directory path
- works on Linux (x86_64), Windows 10(x86), MinGW system

## Build
- Linux, MinGW: make
- Windows: build by Microsoft Visual Studio 2019 project

## Note for Linux system
- Some system may require glibc-static library.
```sh
dnf install glibc-static
```
- If arch does not match,\
 . build lame static library and install or copy the file(libmp3lame.a) to ./lib

## Note for Windows system
- Use Microsoft Visual Studio 2019 solution/project files - **MP3enc_cpp.sln**.
- When you build libmp3lame library,\
 . as recommended in **README**, use nmake instead of Visual Studio project.\
 . this needs **nasm** for build (https://www.nasm.us/pub/nasm/releasebuilds/2.14.02/win64/) as it assumes nasm is available.
 ```sh
 #__ readme ____________________________________________________________________
 #	nmake -f Makefile.MSVC
 #		-> build lame, but not mp3x
 #		-> use Robert's code modifications
 #		-> assume MSVC 6.0 compiler available
 #		-> assume NASM available
 #		-> assemble MMX code with NASM
 #		-> no compiler warnings
 #		-> use single precision float
```
- and it needs a modification on its filename and to add the installation directory(default: C:\Program Files\NASM) to PATH.

```diff
diff --git a/Makefile.MSVC b/Makefile.MSVC
index 6538911..527a9a6 100644
--- a/Makefile.MSVC
+++ b/Makefile.MSVC
@@ -544,7 +544,7 @@ ASM_OBJ = $(ASM_OBJ) \
 .SUFFIXES : .nas
 .nas.obj:
        @echo $<
-       @nasmw -f $(OFF) -i libmp3lame/i386/ -DWIN32 $< -o $@
+       @nasm -f $(OFF) -i libmp3lame/i386/ -DWIN32 $< -o $@

 CC_SWITCHES = $(CC_SWITCHES) /DHAVE_NASM
 ASM_OBJ = $(ASM_OBJ) \
```
- pthread headers(.\\pthread_h\\*.h) should be placed in include directory of either Windows SDK(C:\Program Files (x86)\Windows Kits\10\Include\\{VERSION}\um) or MSVC(C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.24.28314\include).

- In case that you encounter a link error on building pthread-w32 library,\
 . to build a static library of pthread, you need to modify **Makefile** - '/MD' in $(XCFLAGS) to '/MT'.\
 . if timespec definition duplicated, add '&& !defined(_INC_TIME)' to **pthread.h**


```c
#if !defined(HAVE_STRUCT_TIMESPEC) && !defined(_INC_TIME)
#define HAVE_STRUCT_TIMESPEC
#if !defined(_TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
struct timespec {
        time_t tv_sec;
        long tv_nsec;
};
#endif /* _TIMESPEC_DEFINED */
#endif /* HAVE_STRUCT_TIMESPEC */
```


## Usage

```sh
MP3enc_cpp <input_directory | input_filename [-o <output_filename>]> [OPTIONS]

Options:
     -h            Show help
     -r            Search subdirectories recursively
     -q <mode>     Set quality level
         fast         fast encoding with small file size
         standard     standard quality - default
         best         best quality
     -v            Verbose detail

Example:
   MP3enc_cpp input.wav -o output.mp3
   MP3enc_cpp wav_dir/ -r -q fast -v
```

- To see technical detail, open ./html/index.html document generated through Doxygen
