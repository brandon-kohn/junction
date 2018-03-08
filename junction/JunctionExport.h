#pragma once
#if defined(TURF_USE_DLL) || defined(TURF_EXPORTS_API)
    #ifdef _WIN32
        #ifdef TURF_EXPORTS_API
            #define TURF_API __declspec(dllexport)
        #else
            #define TURF_API __declspec(dllimport)
        #endif
    #else
        #ifdef TURF_EXPORTS_API
            #define TURF_API __attribute__ ((visibility ("default")))
        #else
            #define TURF_API
    #endif
#endif
    #define TURF_API 
#endif

