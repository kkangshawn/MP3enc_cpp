/**
 * @file        debug.h
 * @version     0.9
 * @brief       MP3enc_cpp debug module header
 * @date        Mar 4, 2020
 * @author      Siwon Kang (kkangshawn@gmail.com)
 */

#ifndef _DEBUG_H
#define _DEBUG_H

#include <iostream>

/**
 * @class   DEBUG debug.h "debug.h"
 *
 * @brief   DEBUG class for debug print functions.
 *          This class is supposed to be used as global functions.
 *
 * @todo    A function to set debug level can be introduced but not really
 *          necessary as of now since there are few debug messages yet.
 */
class DEBUG {
protected:
    /**
     * @fn      DEBUG::ERR(const char* s)
     * @brief   print ERROR messages.
     * @param [in]  s   a string to print.
     */
    static void ERR(const char* s) { if (verbose) std::cerr << "DEBUG(ERROR): " << s << std::endl; }
    /**
     * @fn      DEBUG::WARN(const char* s)
     * @brief   print WARNING messages.
     * @param [in]  s   a string to print.
     */
    static void WARN(const char* s) { if (verbose) std::cout << "DEBUG(WARN): " << s << std::endl; }
    /**
     * @fn      DEBUG::INFO(const char* s)
     * @brief   print INFORMATION messages.
     * @param [in]  s   a string to print.
     */
    static void INFO(const char* s) { if (verbose) std::cout << "DEBUG(INFO): " << s << std::endl; }
    static void SET() { verbose = true; }       /**< enable debug messages */
    static void CLEAR() { verbose = false; }    /**< disable debug messages */
    static bool IS_SET() { return verbose; }    /**< check if debug messages are to be printed */
private:
    static bool verbose;
};

#endif  /* _DEBUG_H */
