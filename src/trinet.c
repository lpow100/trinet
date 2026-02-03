#if defined(_WIN32)
    #include "windows.c"
#elif defined(__linux__)
    #include "linux.c"
#else
    #error "Platform trinet!"
#endif