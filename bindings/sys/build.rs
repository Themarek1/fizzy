// Fizzy: A fast WebAssembly interpreter
// Copyright 2019-2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0

extern crate bindgen;
extern crate cmake;

use cmake::Config;

use std::env;
use std::path::PathBuf;

fn main() {
    let src = "fizzy";

    let dst = Config::new(src)
        .define("FIZZY_TESTING", "OFF")
        .define("FIZZY_FUZZING", "OFF")
        .build();

    println!("cargo:rustc-link-lib=static={}", "fizzy");
    println!("cargo:rustc-link-search=native={}/lib", dst.display());

    // We need to link against C++ std lib
    if let Some(cpp_stdlib) = get_cpp_stdlib() {
        println!("cargo:rustc-link-lib={}", cpp_stdlib);
    }

    let bindings = bindgen::Builder::default()
        .header(format!("{}/include/fizzy/fizzy.h", src))
        // See https://github.com/rust-lang-nursery/rust-bindgen/issues/947
        .trust_clang_mangling(false)
        .generate_comments(true)
        // https://github.com/rust-lang-nursery/rust-bindgen/issues/947#issuecomment-327100002
        .layout_tests(false)
        // do not generate an empty enum for EVMC_ABI_VERSION
        .constified_enum("")
        // generate Rust enums for each evmc enum
        .rustified_enum("*")
        // force deriving the Hash trait on basic types (address, bytes32)
        .derive_hash(true)
        // force deriving the PratialEq trait on basic types (address, bytes32)
        .derive_partialeq(true)
        //        .whitelist_type("fizzy_*")
        //        .whitelist_function("fizzy_*")
        //        .opaque_type("evmc_host_context")
        //        .whitelist_type("evmc_.*")
        //        .whitelist_function("evmc_.*")
        //        .whitelist_var("EVMC_ABI_VERSION")
        // TODO: consider removing this
        .size_t_is_usize(true)
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}

// See https://github.com/alexcrichton/gcc-rs/blob/88ac58e25/src/lib.rs#L1197
fn get_cpp_stdlib() -> Option<String> {
    env::var("TARGET").ok().and_then(|target| {
        if target.contains("msvc") {
            None
        } else if target.contains("darwin") {
            Some("c++".to_string())
        } else if target.contains("freebsd") {
            Some("c++".to_string())
        } else if target.contains("musl") {
            Some("static=stdc++".to_string())
        } else {
            Some("stdc++".to_string())
        }
    })
}
