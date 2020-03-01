#include "utils.h"

int
Utils::read_32_bits_high_low(std::ifstream* in)
{
    char bytes[4] = { 0, 0, 0, 0 };

    in->read(bytes, 4);

    int32_t const low = bytes[3];
    int32_t const midl = bytes[2];
    int32_t const midh = bytes[1];
    int32_t const high = (signed char)(bytes[0]);

    return (high << 24) | (midh << 16) | (midl << 8) | low;
}

int
Utils::read_32_bits_low_high(std::ifstream* in)
{
    char bytes[4] = { 0, 0, 0, 0 };

    in->read(bytes, 4);

    int32_t const low = bytes[0];
    int32_t const midl = bytes[1];
    int32_t const midh = bytes[2];
    int32_t const high = (signed char)(bytes[3]);

    return (high << 24) | (midh << 16) | (midl << 8) | low;
}

int
Utils::read_16_bits_low_high(std::ifstream* in)
{
    char bytes[2] = { 0, 0 };

    in->read(bytes, 2);

    int32_t const low = bytes[0];
    int32_t const high = (signed char)(bytes[1]);

    return (high << 8) | low;
}

long
Utils::make_even_number_of_bytes_in_length(long x)
{
    if ((x & 0x01) != 0) x++;

    return x;
}

double
Utils::get_file_size(const char* file)
{
    struct stat st;

    if (stat(file, &st) == 0) {
        return st.st_size;
    }

    return -1;
}

bool
Utils::isWAV(std::string file)
{
    if (file.size() < 5) {
        /* 
         * minimum length of filename will be 5 assuming that the filename
         * contains the extension ".wav"
         */
        return false;
    }

    std::string::reverse_iterator it = file.rbegin();

    if (*(it + 3) == '.' &&
        *(it + 2) == 'w' &&
        *(it + 1) == 'a' &&
        *(it)     == 'v') {
        return true;
    }

    return false;
}

int
Utils::scmp(const char* a, const char* b) {
    while (*a && *b && !(*a - *b)) {
        a++; b++;
    }
    return *a - *b;
}
