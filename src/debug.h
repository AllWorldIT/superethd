
#ifndef DEBUG_H
#define DEBUG_H

// Debug output macro
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, "%s(): " fmt "\n", __func__, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) ((void)0)
#endif

// Normal fprintf macro
#define FPRINTF(fmt, ...) fprintf(stderr, "%s(): " fmt "\n", __func__, ##__VA_ARGS__)


#endif