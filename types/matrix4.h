#pragma once

#include "vector4.h"

typedef struct {
    float m[4][4];//Row,Column
} matrix4x4;

static inline matrix4x4 matrix_zero(){
    return (matrix4x4){
        .m = {
            { 0.0, 0.0, 0.0, 0.0},
            { 0.0, 0.0, 0.0, 0.0},
            { 0.0, 0.0, 0.0, 0.0},
            { 0.0, 0.0, 0.0, 0.0}, 
        }
    };
}

static inline vector4 matrix4_mul(vector4 v, matrix4x4 m){
    vector4 out = {};
    
    out.x = v.x*m.m[0][0] + v.y*m.m[1][0] + v.z*m.m[2][0] + v.w*m.m[3][0];
    out.y = v.x*m.m[0][1] + v.y*m.m[1][1] + v.z*m.m[2][1] + v.w*m.m[3][1];
    out.z = v.x*m.m[0][2] + v.y*m.m[1][2] + v.z*m.m[2][2] + v.w*m.m[3][2];
    out.w = v.x*m.m[0][3] + v.y*m.m[1][3] + v.z*m.m[2][3] + v.w*m.m[3][3];
    
    return out;
}