#include "koks_project_def.h"
#include <stdint.h>

#if defined(KOKS_PROJECT_M31) || defined(KOKS_PROJECT_M32) || defined(KOKS_PROJECT_M31A) || defined(KOKS_PROJECT_M33) || defined(KOKS_PROJECT_M35)
    unsigned short resolutions[] = {608, 800, 480, 640, 608, 800, 608, 800, 480, 640, 608, 800};
#elif defined(KOKS_PROJECT_M21A) || defined(KOKS_PROJECT_M21B) || defined(KOKS_PROJECT_M25I) \
    || defined(KOKS_PROJECT_M53) || defined(KOKS_PROJECT_M53A) || defined(KOKS_PROJECT_M53B) || defined(KOKS_PROJECT_M55) \
    || defined(KOKS_PROJECT_M24B) || defined(KOKS_PROJECT_M24B1) || defined(KOKS_PROJECT_M24B2) || defined(KOKS_PROJECT_M24B3) \
    || defined(KOKS_PROJECT_M24CP) || defined(KOKS_PROJECT_M53D) || defined(KOKS_PROJECT_M25P)
    unsigned short resolutions[] = {800, 608, 480, 640, 800, 608, 800, 608, 480, 640, 800, 608};
#else
    #define KOKS_NO_UVC
#endif