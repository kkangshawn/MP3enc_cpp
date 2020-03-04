/**
 * @file        main.h
 * @version     0.9
 * @brief       MP3enc_cpp main application header
 * @date        Mar 4, 2020
 * @author      Siwon Kang (kkangshawn@gmail.com)
 */

#ifndef _MAIN_H
#define _MAIN_H

#include "common.h"
#include "audio.h"
#include "utils.h"

/**
 * @class   MP3enc main.h "main.h"
 * @brief   main class for MP3enc application.
 *          Designed as singleton to prevent from any conflicts by multiple access
 *          to encoding library.
 */
class MP3enc : public Utils, DEBUG {
public:
    static int main(int argc, char** argv);             /**< main function. */
    static MP3enc* getInstance(int argc, char** argv);  /**< getInstance() that replaces constructor. */
    void freeInstance();                                /**< freeInstance() that replaces destructor. */
    void showUsage();                                   /**< show usage. */

private:
    /**
     * @struct  Options main.h "main.h"
     * @brief   Options structure which stores setting arguments of user input.
     */
    struct Options {
        /**
         * @var     std::string inPath
         * @brief   Path of wav file or directory to encode.
         */
        std::string inPath;
        /**
         * @var     std::string outPath
         * @brief   Output filename delivered through -o option.
         */
        std::string outPath;
        /**
         * @var     bool        recursive
         * @brief   Flag if to search for sub directories, delivered through -r option.
         */
        bool        recursive;
        /**
         * @var     bool        verbose
         * @brief   Flag to show debug messages, delivered through -v option.
         */
        bool        verbose;
    };

    MP3enc() : m_opt{ {}, {}, false, false } {}
    virtual ~MP3enc() {}

    /**
     * @fn      bool parseOption(int argc, char** argv)
     * @brief   A function to parse input arguments.
     * @param [in]  argc    The number of arguments
     * @param [in]  argv    Argument strings
     * @return  bool    true if successful in parsing all the arguments
     */
    bool parseOption(int argc, char** argv);
    /**
     * @fn      void checkPath(string path)
     * @brief   A function to process input path. This can handle both single file and a directory.
     * @param [in]  path    input path
     */
    void checkPath(std::string path);

    Options         m_opt;          /**< input arguments */
    static MP3enc*  m_instance;     /**< pointer to the instance */
    static size_t   refCnt;         /**< reference counter to the instance */
};
#endif  /* _MAIN_H */
