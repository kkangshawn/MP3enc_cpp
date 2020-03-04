# MP3enc_cpp

WAV to MP3 Encoder, written in C++
---------------------------------------------

- uses LAME library (https://lame.sourceforge.io/)
- supports encoding multiple files using pthread by putting input in directory path
- works on Linux (x86_64), Windows 10(x64), MinGW system

## Build
- Linux, MinGW: make
- Windows: build by Microsoft Visual Studio 2019 project or nmake

## Note for Linux system
- Some system may require glibc-static library.
```sh
dnf install glibc-static
```
- If arch does not match,  
 . build lame static library and install or copy the file(libmp3lame.a) to ./lib

## Note for Windows system
- Microsoft Visual Studio 2019 solution/project files in .\\msvc_solution.
- When you build libmp3lame library,  
 . as recommended in **README**, use nmake instead of Visual Studio project.  
 . this needs to eliminate /opt:NOWIN98 from $(LN_OPTS)/$(LN_DLL) in **Makefile.MSVC** since it is deprecated.  
 . for x64 build, eliminate /GL from $(CC_OPTS).  

- In case that you encounter a link error on building pthread-w32 library,  
 . to build a static library of pthread, you need to modify **Makefile** - '/MD' in $(XCFLAGS) to '/MT'.  
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

To see technical detail, open ./html/index.html document generated through Doxygen
