#pragma once

#include "string/slice.h"

typedef struct {
    float x;
    float y;
    float z;
} vector3;

#define NUM_VECS 8

typedef enum {
    prim_none,
    prim_pixel,
    prim_line,
    prim_trig,
    prim_quad
} primitives;

void handle_obj_line(string_slice line);