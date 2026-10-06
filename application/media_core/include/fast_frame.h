#pragma once
#include "koks_project_def.h"

#ifdef KOKS_PROJECT_M53
#define FRAME_PYADDR 0x03295000
#elif defined(KOKS_PROJECT_M53A)
#define FRAME_PYADDR 0x03100000
#elif defined(KOKS_PROJECT_M53B)
#define FRAME_PYADDR 0x0497b800
#elif defined(KOKS_PROJECT_M53D)
    #ifdef KOKS_FEATURE_T33_ZN
        #define FRAME_PYADDR 0x0449e000 
    #else
        #define FRAME_PYADDR 0x0330e800
    #endif
#elif defined(KOKS_PROJECT_M55)
#define FRAME_PYADDR 0x0490b000
#elif defined(KOKS_PROJECT_M31)
#define FRAME_PYADDR 0x031dc200
#elif defined(KOKS_PROJECT_M32) || defined(KOKS_PROJECT_M31A) || defined(KOKS_PROJECT_M33)
#define FRAME_PYADDR 0x03550200
#elif defined(KOKS_PROJECT_M24CP) || defined(KOKS_PROJECT_M25P)
#define FRAME_PYADDR 0x0329e000
#elif defined(KOKS_PROJECT_M24B3)
#define FRAME_PYADDR 0x02f0e800
#elif defined(KOKS_PROJECT_M21A) || defined(KOKS_PROJECT_M21B) || defined(KOKS_PROJECT_M25I)
#define FRAME_PYADDR 0x0330e800
#elif defined(KOKS_PROJECT_M35)
#define FRAME_PYADDR 0x0550e800
#else
#define FRAME_PYADDR 0x0330e800
#endif
