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

struct fizzy_instance
{
    fizzy::Instance* instance;
};

fizzy_instance* fizzy_instantiate(fizzy_module* module,
    const fizzy_external_function* imported_functions, uint32_t imported_functions_size)
{
    try
    {
        std::vector<fizzy::ExternalFunction> functions(imported_functions_size);
        for (size_t imported_func_idx = 0; imported_func_idx < imported_functions_size;
             ++imported_func_idx)
        {
            auto func = [cfunc = imported_functions[imported_func_idx]](fizzy::Instance& instance,
                            fizzy::span<const fizzy::Value> args,
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

            // TODO get type from input array
            auto func_type = module->module.imported_function_types[imported_func_idx];

            functions[imported_func_idx] =
                fizzy::ExternalFunction{std::move(func), std::move(func_type)};
        }

        auto instance = fizzy::instantiate(std::move(module->module), std::move(functions));

        auto cinstance = std::make_unique<fizzy_instance>();
        cinstance->instance = instance.release();

        fizzy_free_module(module);
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
