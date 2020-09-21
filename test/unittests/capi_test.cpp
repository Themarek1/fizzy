// Fizzy: A fast WebAssembly interpreter
// Copyright 2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0

#include <fizzy/fizzy.h>
#include <gtest/gtest.h>
#include <test/utils/hex.hpp>

using namespace fizzy::test;

TEST(capi, fizzy_validate)
{
    uint8_t wasm_prefix[]{0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    EXPECT_TRUE(fizzy_validate(wasm_prefix, sizeof(wasm_prefix)));
    wasm_prefix[7] = 1;
    EXPECT_FALSE(fizzy_validate(wasm_prefix, sizeof(wasm_prefix)));
}

TEST(capi, parse)
{
    uint8_t wasm_prefix[]{0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    auto module = fizzy_parse(wasm_prefix, sizeof(wasm_prefix));
    EXPECT_TRUE(module);
    fizzy_free_module(module);
}

TEST(capi, instantiate)
{
    uint8_t wasm_prefix[]{0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    auto module = fizzy_parse(wasm_prefix, sizeof(wasm_prefix));

    auto instance = fizzy_instantiate(module, nullptr, 0);
    EXPECT_TRUE(instance);

    fizzy_free_instance(instance);
}

TEST(capi, execute)
{
    /* wat2wasm
      (func (result i32) i32.const 42)
      (func (param i32 i32) (result i32)
        (i32.div_u (local.get 0) (local.get 1))
      )
    */
    const auto wasm = from_hex(
        "0061736d01000000010b026000017f60027f7f017f03030200010a0e020400412a0b0700200020016e0b");

    auto module = fizzy_parse(wasm.data(), wasm.size());

    auto instance = fizzy_instantiate(module, nullptr, 0);

    auto res = fizzy_execute(instance, 0, nullptr, 0, 0);
    EXPECT_FALSE(res.trapped);
    EXPECT_TRUE(res.has_value);
    EXPECT_EQ(res.value.i64, 42);

    fizzy_value args[] = {{42}, {2}};
    auto res2 = fizzy_execute(instance, 1, args, 2, 0);
    EXPECT_FALSE(res2.trapped);
    EXPECT_TRUE(res2.has_value);
    EXPECT_EQ(res2.value.i64, 21);


    fizzy_free_instance(instance);
}

TEST(capi, execute_with_host_function)
{
    /* wat2wasm
      (func (import "mod1" "foo1") (result i32))
      (func (import "mod1" "foo2") (param i32 i32) (result i32))
    */
    const auto wasm = from_hex(
        "0061736d01000000010b026000017f60027f7f017f021902046d6f643104666f6f310000046d6f643104666f6f"
        "320001");
    auto module = fizzy_parse(wasm.data(), wasm.size());

    fizzy_external_function host_funcs[] = {
        {[](void*, fizzy_instance*, const fizzy_value*, uint32_t, int) {
             return fizzy_execution_result{false, true, {42}};
         },
            nullptr},
        {[](void*, fizzy_instance*, const fizzy_value* args, uint32_t, int) {
             return fizzy_execution_result{false, true, {args[0].i64 / args[1].i64}};
         },
            nullptr}};

    auto instance = fizzy_instantiate(module, host_funcs, 2);

    auto res = fizzy_execute(instance, 0, nullptr, 0, 0);
    EXPECT_FALSE(res.trapped);
    EXPECT_TRUE(res.has_value);
    EXPECT_EQ(res.value.i64, 42);


    fizzy_value args[] = {{42}, {2}};
    auto res2 = fizzy_execute(instance, 1, args, 2, 0);
    EXPECT_FALSE(res2.trapped);
    EXPECT_TRUE(res2.has_value);
    EXPECT_EQ(res2.value.i64, 21);

    fizzy_free_instance(instance);
}

TEST(capi, imported_function_from_another_module)
{
    /* wat2wasm
    (module
      (func $sub (param $lhs i32) (param $rhs i32) (result i32)
        get_local $lhs
        get_local $rhs
        i32.sub)
      (export "sub" (func $sub))
    )
    */
    const auto bin1 = from_hex(
        "0061736d0100000001070160027f7f017f030201000707010373756200000a09010700200020016b0b");
    auto module1 = fizzy_parse(bin1.data(), bin1.size());
    auto instance1 = fizzy_instantiate(module1, nullptr, 0);

    /* wat2wasm
    (module
      (func $sub (import "m1" "sub") (param $lhs i32) (param $rhs i32) (result i32))

      (func $main (param i32) (param i32) (result i32)
        get_local 0
        get_local 1
        call $sub
      )
    )
    */
    const auto bin2 = from_hex(
        "0061736d0100000001070160027f7f017f020a01026d31037375620000030201000a0a0108002000200110000"
        "b");
    auto module2 = fizzy_parse(bin2.data(), bin2.size());

    // TODO fizzy_find_exported_function

    auto sub = [](void* context, fizzy_instance*, const fizzy_value* args, uint32_t args_size,
                   int depth) -> fizzy_execution_result {
        auto* called_instance = static_cast<fizzy_instance*>(context);
        return fizzy_execute(called_instance, 0, args, args_size, depth);
    };
    fizzy_external_function host_funcs[] = {{sub, instance1}};

    auto instance2 = fizzy_instantiate(module2, host_funcs, 1);

    fizzy_value args[] = {{44}, {2}};
    auto res2 = fizzy_execute(instance2, 1, args, 2, 0);
    EXPECT_FALSE(res2.trapped);
    EXPECT_TRUE(res2.has_value);
    EXPECT_EQ(res2.value.i64, 42);

    fizzy_free_instance(instance2);
    fizzy_free_instance(instance1);
}
