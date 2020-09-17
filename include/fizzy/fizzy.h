// Fizzy: A fast WebAssembly interpreter
// Copyright 2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool fizzy_validate(const uint8_t* wasm_binary, size_t wasm_binary_size);

struct fizzy_module;

struct fizzy_module* fizzy_parse(const uint8_t* wasm_binary, size_t wasm_binary_size);

void fizzy_free_module(struct fizzy_module*);

union fizzy_value
{
    uint64_t i64;
    float f32;
    double f64;
};

typedef struct fizzy_execution_result
{
    bool trapped;
    bool has_value;
    union fizzy_value value;
} fizzy_execution_result;

struct fizzy_instance;

typedef struct fizzy_execution_result (*fizzy_external_fn)(void* context,
    struct fizzy_instance* instance, const union fizzy_value* args, uint32_t cargs_size, int depth);

typedef struct fizzy_external_function
{
    // TODO function type
    fizzy_external_fn function;
    void* context;
} fizzy_external_function;

struct fizzy_external_function_vector;

struct fizzy_external_function_vector* fizzy_new_external_function_vector(
    const struct fizzy_external_function* functions, uint32_t size);

void fizzy_free_external_function_vector(struct fizzy_external_function_vector*);

// Takes ownership of module and imported_functions.
// imported_functions may be NULL.
struct fizzy_instance* fizzy_instantiate(
    struct fizzy_module* module, struct fizzy_external_function_vector* imported_functions);

void fizzy_free_instance(struct fizzy_instance*);

fizzy_execution_result fizzy_execute(struct fizzy_instance* instance, uint32_t func_idx,
    const union fizzy_value* args, uint32_t args_size, int depth);

#ifdef __cplusplus
}
#endif
