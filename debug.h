#ifndef _DEBUG_H
#define _DEBUG_H

#include "common.h"

class DEBUG {
protected:
    static void ERR(const char* s) { if (verbose) std::cerr << "DEBUG(ERROR): " << s << std::endl; }
    static void WARN(const char* s) { if (verbose) std::cout << "DEBUG(WARN): " << s << std::endl; }
    static void INFO(const char* s) { if (verbose) std::cout << "DEBUG(INFO): " << s << std::endl; }
    static void SET() { verbose = true; }
    static void CLEAR() { verbose = false; }
private:
    static bool verbose;
};

#endif
