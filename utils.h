/**
 * @file        utils.h
 * @version     0.9
 * @brief       MP3enc_cpp utility functions header
 * @date        Mar 4, 2020
 * @author      Siwon Kang (kkangshawn@gmail.com)
 */

#ifndef _UTILS_H
#define _UTILS_H

#include "common.h"

#include <sys/stat.h>

/**
 * @class   Utils utils.h "utils.h"
 *
 * @brief   Class for utility functions.
 *          Intended to be used by inheritance.
 */
class Utils {
protected:
#if defined (_WIN32)
    const char DELIMITER = '\\';
#else
    const char DELIMITER = '/';
#endif

    /**
     * @fn      int read_32_bits_high_low(std::ifstream* in)
     * @brief   A function to read bits from input file stream.
     * @param [in]  in      a file stream to read bits
     * @return  int value
     */
    int     read_32_bits_high_low(std::ifstream* in);
    /**
     * @fn      int read_32_bits_low_high(std::ifstream* in)
     * @brief   A function to read bits from input file stream.
     * @param [in]  in      a file stream to read bits
     * @return  int value
     */
    int     read_32_bits_low_high(std::ifstream* in);
    /**
     * @fn      int read_16_bits_low_high(std::ifstream* in)
     * @brief   A function to read bits from input file stream.
     * @param [in]  in      a file stream to read bits
     * @return  int value
     */
    int     read_16_bits_low_high(std::ifstream* in);
    long    make_even_number_of_bytes_in_length(long x);    /**< padding 1 if given number is odd. */
    /**
     * @fn      double get_file_size(const char* file)
     * @brief   A function to get file size by its path/name.
     * @param [in]  file    a filename with path to get size
     * @return  size of file
     */
    double  get_file_size(const char* file);
    size_t  min_size(size_t a, size_t b) { return a < b ? a : b; }  /**< get smaller one between two. */
    /**
     * @fn      bool is_wav(const std::string path)
     * @brief   check if the extension of given path is ".wav".
     * @param [in]  path    path(filename) to check its extension
     * @return  true if the path ends with ".wav"
     */
    bool    is_wav(const std::string path);
    int     scmp(const char* a, const char* b); /**< compare two strings are identical. Return 0 if same. */
};

#endif  /* _UTILS_H */
