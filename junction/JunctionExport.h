#pragma once

#ifdef JUNCTION_STATIC_LIB
    #define JUNCTION_API 
#else
    #ifdef _WIN32
        #ifdef JUNCTION_EXPORTS_API
            #define JUNCTION_API __declspec(dllexport)
        #else
            #define JUNCTION_API __declspec(dllimport)
        #endif
    #else
        #ifdef JUNCTION_EXPORTS_API
            #define JUNCTION_API __attribute__ ((visibility ("default")))
        #else
            #define JUNCTION_API
        #endif
    #endif
#endif