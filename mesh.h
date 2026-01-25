#pragma once

#include "data/struct/chunk_array.h"
#include "types/vector3.h"

typedef enum {
    prim_none,
    prim_pixel,
    prim_line,
    prim_trig,
    prim_quad
} primitives;

typedef struct {
    chunk_array_t *vertices;
    chunk_array_t *segments;
    primitives primitive_type;
} mesh_t;

size_t mesh_num_verts(mesh_t *m);
size_t mesh_num_segments(mesh_t *m);

int mesh_get_segment(mesh_t *m, size_t index);
vector3 mesh_get_vertices(mesh_t *m, size_t index);