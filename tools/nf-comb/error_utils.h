#pragma once

#include <stdio.h>
#include <stdlib.h>

// Macro para exibir uma mensagem de erro e abortar o programa
#define ERROR_MSG(msg) \
    do { \
        fprintf(stderr, "Error: %s\n", msg); \
        exit(EXIT_FAILURE); \
    } while (0)