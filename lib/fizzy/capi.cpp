// Fizzy: A fast WebAssembly interpreter
// Copyright 2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0

#include "execute.hpp"
#include "instantiate.hpp"
#include "parser.hpp"
#include <fizzy/fizzy.h>
#include <memory>

namespace
{
/// Simple implementation of C++20's std::bit_cast.
template <typename DstT, typename SrcT>
DstT bit_cast(SrcT x) noexcept
{
    DstT z;
    static_assert(sizeof(x) == sizeof(z));
    __builtin_memcpy(&z, &x, sizeof(x));
    return z;
}
}  // namespace

extern "C" {
bool fizzy_validate(const uint8_t* wasm_binary, size_t wasm_binary_size)
{
    try
    {
        fizzy::parse({wasm_binary, wasm_binary_size});
        return true;
    }
    catch (...)
    {
        return false;
    }
}

struct fizzy_module
{
    fizzy::Module module;
};

fizzy_module* fizzy_parse(const uint8_t* wasm_binary, size_t wasm_binary_size)
{
    try
    {
        auto cmodule = std::make_unique<fizzy_module>();
        cmodule->module = fizzy::parse({wasm_binary, wasm_binary_size});
        return cmodule.release();
    }
    catch (...)
    {
        return nullptr;
    }
}

void fizzy_free_module(fizzy_module* module)
{
    delete module;
}

struct fizzy_external_function_vector
{
    std::vector<fizzy::ExternalFunction> vector;
};

struct fizzy_instance
{
    fizzy::Instance* instance;
};

fizzy_external_function_vector* fizzy_new_external_function_vector(
    const fizzy_external_function* cfunctions, uint32_t size)
{
    std::vector<fizzy::ExternalFunction> functions(size);
    std::transform(
        cfunctions, cfunctions + size, functions.begin(), [](const fizzy_external_function& cfunc) {
            auto func = [cfunc](fizzy::Instance& instance, fizzy::span<const fizzy::Value> args,
                            int depth) -> fizzy::ExecutionResult {
                fizzy_instance cinstance{&instance};
                const auto cres = cfunc.function(cfunc.context, &cinstance,
                    reinterpret_cast<const fizzy_value*>(args.data()),
                    static_cast<uint32_t>(args.size()), depth);

                if (cres.trapped)
                    return fizzy::Trap;
                else if (!cres.has_value)
                    return fizzy::Void;
                else
                    return bit_cast<fizzy::Value>(cres.value);
            };
            // TODO leave type empty for now
            return fizzy::ExternalFunction{std::move(func), {}};
        });

    return new fizzy_external_function_vector{std::move(functions)};
}

void fizzy_free_external_function_vector(fizzy_external_function_vector* vector)
{
    delete vector;
}

fizzy_instance* fizzy_instantiate(
    fizzy_module* module, fizzy_external_function_vector* imported_functions)
{
    try
    {
        // TODO temp: fill types of imported funcs
        if (imported_functions)
        {
            for (size_t imported_func_idx = 0;
                 imported_func_idx < imported_functions->vector.size(); ++imported_func_idx)
            {
                imported_functions->vector[imported_func_idx].type =
                    module->module.imported_function_types[imported_func_idx];
            }
        }

        auto instance = fizzy::instantiate(
            std::move(module->module), imported_functions ? std::move(imported_functions->vector) :
                                                            std::vector<fizzy::ExternalFunction>{});

        auto cinstance = std::make_unique<fizzy_instance>();
        cinstance->instance = instance.release();

        fizzy_free_module(module);
        fizzy_free_external_function_vector(imported_functions);
        return cinstance.release();
    }
    catch (...)
    {
        return nullptr;
    }
}

void fizzy_free_instance(fizzy_instance* instance)
{
    delete instance->instance;
    delete instance;
}

fizzy_execution_result fizzy_execute(fizzy_instance* instance, uint32_t func_idx,
    const fizzy_value* cargs, uint32_t cargs_size, int depth)
{
    const auto args = reinterpret_cast<const fizzy::Value*>(cargs);

    const auto result = fizzy::execute(
        *instance->instance, func_idx, fizzy::span<const fizzy::Value>(args, cargs_size), depth);

    return {result.trapped, result.has_value, bit_cast<fizzy_value>(result.value)};
}
}
