#ifndef _UTILS_H
#define _UTILS_H

#include "common.h"

#include <sys/stat.h>

class Utils {
protected:
#if defined (__linux)
    const char DELIMITER = '/';
#else
    const char DELIMITER = '\\';
#endif

    int     read_32_bits_high_low(std::ifstream* in);
    int     read_32_bits_low_high(std::ifstream* in);
    int     read_16_bits_low_high(std::ifstream* in);
    long    make_even_number_of_bytes_in_length(long x);
    double  get_file_size(const char* file);
    size_t  min_size(size_t a, size_t b) { return a < b ? a : b; }
    bool    isWAV(const std::string path);
    int     scmp(const char* a, const char* b);
};

#endif
