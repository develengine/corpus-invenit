#ifndef LINA_H_
#define LINA_H_

#include <math.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

typedef struct
{
    float data[16];
} mat4_t;

/* https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/
 *
 * Reversed Z approach to projection and depth testing.
 */
static inline mat4_t mat4_rz_projection(float y_fov_r, float w, float h, float near)
{
    float asp = w / h;
    float f = 1.0f / tanf(y_fov_r / 2.0f);

    return (mat4_t) {
        f / asp, 0.0f, 0.0f, 0.0f,
        0.0f,    f,    0.0f, 0.0f,
        0.0f,    0.0f, 0.0f,-1.0f,
        0.0f,    0.0f, near, 0.0f,
    };
}

static inline mat4_t mat4_identity(void)
{
    return (mat4_t) {
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f,
    };
}

static inline mat4_t mat4_transpose(mat4_t m)
{
    mat4_t res;

    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            res.data[x + y * 4] = m.data[y + x * 4];
        }
    }

    return res;
}

/* https://gitlab.freedesktop.org/mesa/glu/-/blob/glu-9.0.0/src/libutil/project.c
 *
 * static int __gluInvertMatrixd(const GLdouble m[16], GLdouble invOut[16])
 *
 * Contributed by David Moore (See Mesa bug #6748)
 *
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * http://oss.sgi.com/projects/FreeB/
 */
static inline mat4_t mat4_inverse(mat4_t mat)
{
    mat4_t res;

    float *m = mat.data;

    float inv[16];

    inv[0] =   m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
             + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4] =  -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
             - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8] =   m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
             + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
             - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1] =  -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
             - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5] =   m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
             + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9] =  -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
             - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
             + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2] =   m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
             + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
    inv[6] =  -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
             - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
             + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
             - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
    inv[3] =  -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
             - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
    inv[7] =   m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
             + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
             - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
             + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

    float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    if (det == 0)
        return mat;

    det = 1.0f / det;

    for (int i = 0; i < 16; i++) {
        res.data[i] = inv[i] * det;
    }

    return res;
}

static inline mat4_t mat4_scale(float x, float y, float z)
{
    return (mat4_t) {
         x,    0.0f, 0.0f, 0.0f,
         0.0f, y,    0.0f, 0.0f,
         0.0f, 0.0f, z,    0.0f,
         0.0f, 0.0f, 0.0f, 1.0f,
    };
}

static inline mat4_t mat4_translation(float x, float y, float z)
{
    return (mat4_t) {
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         x,    y,    z,    1.0f,
    };
}

static inline mat4_t mat4_rotation_x(float angle)
{
    return (mat4_t) {
         1.0f, 0.0f,        0.0f,        0.0f,
         0.0f, cosf(angle), sinf(angle), 0.0f,
         0.0f,-sinf(angle), cosf(angle), 0.0f,
         0.0f, 0.0f,        0.0f,        1.0f,
    };
}

static inline mat4_t mat4_rotation_y(float angle)
{
    return (mat4_t) {
         cosf(angle), 0.0f,-sinf(angle), 0.0f,
         0.0f,        1.0f, 0.0f,        0.0f,
         sinf(angle), 0.0f, cosf(angle), 0.0f,
         0.0f,        0.0f, 0.0f,        1.0f,
    };
}
static inline mat4_t mat4_rotation_z(float angle)
{
    return (mat4_t) {
         cosf(angle), sinf(angle), 0.0f, 0.0f,
        -sinf(angle), cosf(angle), 0.0f, 0.0f,
         0.0f,        0.0f,        1.0f, 0.0f,
         0.0f,        0.0f,        0.0f, 1.0f,
    };
}

static inline mat4_t mat4_mul(mat4_t mat1, mat4_t mat2)
{
    const float *m1 = mat1.data;
    const float *m2 = mat2.data;

    mat4_t res;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float value = 0;

            for (int k = 0; k < 4; k++)
                value += m2[j * 4 + k] * m1[k * 4 + i];

            res.data[j * 4 + i] = value;
        }
    }

    return res;
}


typedef struct
{
    union {
        float data[4];
        struct { float x, y, z, w; };
        struct { float r, g, b, a; };
    };
} vec4_t;

static inline vec4_t vec4_add(vec4_t a, vec4_t b)
{
    return (vec4_t) {
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
        .w = a.w + b.w,
    };
}

static inline vec4_t vec4_sub(vec4_t a, vec4_t b)
{
    return (vec4_t) {
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
        .w = a.w - b.w,
    };
}

static inline float vec4_length(vec4_t v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

static inline vec4_t vec4_scale(vec4_t v, float s)
{
    return (vec4_t) {
        .x = v.x * s,
        .y = v.y * s,
        .z = v.z * s,
        .w = v.w * s,
    };
}

static inline vec4_t vec4_normalize(vec4_t v)
{
    return vec4_scale(v, 1.0f / vec4_length(v));
}

static inline float vec4_dot(vec4_t a, vec4_t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}


typedef struct
{
    union {
        float data[3];
        struct { float x, y, z; };
        struct { float r, g, b; };
    };
} vec3_t;

static inline vec3_t vec3_add(vec3_t a, vec3_t b)
{
    return (vec3_t) {
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b)
{
    return (vec3_t) {
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b)
{
    return (vec3_t) {
        .x = a.y * b.z - a.z * b.y,
        .y = a.z * b.x - a.x * b.z,
        .z = a.x * b.y - a.y * b.x,
    };
}

static inline float vec3_length(vec3_t v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static vec3_t vec3_scale(vec3_t v, float s)
{
    return (vec3_t) {
        .x = v.x * s,
        .y = v.y * s,
        .z = v.z * s,
    };
}

static inline vec3_t vec3_normalize(vec3_t v)
{
    return vec3_scale(v, 1.0f / vec3_length(v));
}

static inline float vec3_dot(vec3_t a, vec3_t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}


typedef struct
{
    union {
        float data[2];
        struct { float x, y; };
        struct { float u, v; };
        struct { float r, i; };
    };
} vec2_t;

static inline vec2_t vec2_add(vec2_t a, vec2_t b)
{
    return (vec2_t) {
        .x = a.x + b.x,
        .y = a.y + b.y,
    };
}

static inline vec2_t vec2_sub(vec2_t a, vec2_t b)
{
    return (vec2_t) {
        .x = a.x - b.x,
        .y = a.y - b.y,
    };
}

static inline vec2_t vec2_mul(vec2_t a, vec2_t b)
{
    return (vec2_t) {
        .x = a.x * b.x,
        .y = a.y * b.y,
    };
}

static inline vec2_t vec2_cmul(vec2_t a, vec2_t b)
{
    return (vec2_t) {
        .r = a.r * b.r - a.i * b.i,
        .i = a.r * b.i + a.i * b.r,
    };
}

static inline float vec2_length(vec2_t v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}

static inline vec2_t vec2_scale(vec2_t v, float s)
{
    return (vec2_t) {
        .x = v.x * s,
        .y = v.y * s,
    };
}

static inline vec2_t vec2_normalize(vec2_t v)
{
    return vec2_scale(v, 1.0f / vec2_length(v));
}

static inline float vec2_dot(vec2_t a, vec2_t b)
{
    return a.x * b.x + a.y * b.y;
}

typedef struct {
    union {
        int data[2];
        struct { int x, y; };
        struct { int u, v; };
    };
} ivec2_t;

typedef struct {
    union {
        int data[3];
        struct { int x, y, z; };
    };
} ivec3_t;

typedef struct {
    union {
        int data[4];
        struct { int x, y, z, w; };
    };
} ivec4_t;


// TODO: Remove and create a custom random number gen.
#include <stdint.h>
#include <stdlib.h>
static inline float lina_rand_norm(void)
{
    return rand() / (float)RAND_MAX;
}



#endif // LINA_H_
