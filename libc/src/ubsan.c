#include <ubsan.h>
#include <kernel/sys.h>

#include <stdio.h>
#include <stdlib.h>

void ub_panic_at(ubsan_source_location_t* location, const char* err) {
    if (location) {
        printf("[ubsan] %s:%d:%d -> %s\n", location->file, location->line, location->column, err);
    }

    abort();
}

void ub_panic(const char* err) {
    printf("[ubsan] %s\n", err);
    abort();
}

void __ubsan_handle_type_mismatch_v1(ubsan_mismatch_data_t* data, uintptr_t ptr) {
    if (!ptr) {
        ub_panic_at(&data->location, "NULL pointer dereference");
    } else if (data->align != 0 && (ptr & ((1 << data->align) - 1)) != 0) {
        printf("[ubsan] pointer %p not aligned to %d\n", ptr, 1 << data->align);
        ub_panic_at(&data->location, "alignment failed");
    } else {
        printf("[ubsan] pointer %p is not large enough for %s\n", ptr, data->type->name);
    }
}

void __ubsan_handle_add_overflow() {
    ub_panic("add_overflow");
}

void __ubsan_handle_sub_overflow() {
    ub_panic("sub overflow");
}

void __ubsan_handle_mul_overflow(void* d, void* l, void* r) {
    ubsan_overflow_data_t* data = (ubsan_overflow_data_t*) d;
    printf("[ubsan] overflow in %d*%d\n", (int32_t) l, (int32_t) r);
    ub_panic_at(&data->location, "mul overflow");
}

void __ubsan_handle_divrem_overflow() {
    ub_panic("divrem overflow");
}

void __ubsan_handle_negate_overflow() {
    ub_panic("negate overflow");
}

void __ubsan_handle_pointer_overflow(void* data_raw, void* lhs_raw, void* rhs_raw) {
    ubsan_overflow_data_t* data = (ubsan_overflow_data_t*) data_raw;
    printf("[ubsan] pointer overflow with operands %p, %p\n", lhs_raw, rhs_raw);
    ub_panic_at(&data->location, "pointer overflow");
}

void __ubsan_handle_out_of_bounds(void* data, void* index) {
    ubsan_out_of_bounds_data_t* d = (ubsan_out_of_bounds_data_t*) data;
    printf("[ubsan] out of bounds at index %d\n", (uint32_t) index);
    ub_panic_at(&d->location, "out of bounds");
}

void __ubsan_handle_shift_out_of_bounds(void* data, void* lhs, void* rhs) {
    ubsan_shift_out_of_bounds_data_t* d = (ubsan_shift_out_of_bounds_data_t*) data;
    printf("[ubsan] %d << %d\n", (uint32_t) lhs, (uint32_t) rhs);
    ub_panic_at(&d->location, "shift out of bounds");
}

void __ubsan_handle_load_invalid_value() {
    ub_panic("load invalid value");
}

void __ubsan_handle_float_cast_overflow() {
    ub_panic("float cast overflow");
}

void __ubsan_handle_builtin_unreachable() {
    ub_panic("builtin unreachable");
}