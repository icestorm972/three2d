#pragma once

#include "string/slice.h"
#include "mesh.h"

mesh parse_obj(void* obj, size_t size, primitives prim_type);