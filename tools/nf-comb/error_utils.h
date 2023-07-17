#pragma once

#include <stdio.h>
#include <stdlib.h>

#define ERROR_MSG(msg) \
    do { \
        fprintf(stderr, "Error: %s\n", msg); \
        exit(EXIT_FAILURE); \
    } while (0)