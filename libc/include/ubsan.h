#pragma once

#include <stdint.h>

typedef struct {
    uint16_t kind;
    uint16_t info;
    char name[];
} ubsan_type_t;

typedef struct {
    const char* file;
    uint32_t line;
    uint32_t column;
} ubsan_source_location_t;

typedef struct {
    ubsan_source_location_t location;
    ubsan_type_t* type;
    uint8_t align;
    uint8_t kind;
} ubsan_mismatch_data_t;

typedef struct {
    ubsan_source_location_t location;
    ubsan_type_t* type;
} ubsan_overflow_data_t;

typedef struct {
    ubsan_source_location_t location;
    ubsan_type_t* lhs_type;
    ubsan_type_t* rhs_type;
} ubsan_shift_out_of_bounds_data_t;

typedef struct {
    ubsan_source_location_t location;
    ubsan_type_t* array_type;
    ubsan_type_t* index_type;
} ubsan_out_of_bounds_data_t;
