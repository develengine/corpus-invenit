#ifndef UI_H_
#define UI_H_

#include <raylib.h>

#include "core/utils.h"
#include "core/dck.h"

#define UI_WINDOW_NAME_CAP 64

typedef struct
{
    i32 lol;
} ui_t;

extern ui_t ui;

#endif // UI_H_

#ifdef UI_IMPL

ui_t ui = {0};

#endif
