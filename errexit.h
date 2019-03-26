#ifndef ERREXIT_H
#define ERREXIT_H

#include <iostream>
#include <errno.h>
#include <string.h>

inline void errExit(const char *err) {
    std::cout << err << std::endl;
    std::cout << strerror(errno) << std::endl;
    return;
}

#endif // ERREXIT_H