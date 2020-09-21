// Fizzy: A fast WebAssembly interpreter
// Copyright 2019-2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0

mod sys;

use std::ptr::NonNull;

pub fn validate(input: &[u8]) -> bool {
    unsafe { sys::fizzy_validate(input.as_ptr(), input.len()) }
}

pub struct Module {
    ptr: NonNull<sys::fizzy_module>,
}

impl Drop for Module {
    fn drop(&mut self) {
        println!("Dropping module");
        unsafe { sys::fizzy_free_module(self.ptr.as_ptr()) }
    }
}

pub fn parse(input: &[u8]) -> Option<Module> {
    let ptr = unsafe { sys::fizzy_parse(input.as_ptr(), input.len()) };
    if ptr.is_null() {
        return None;
    }
    Some(Module {
        ptr: unsafe { NonNull::new_unchecked(ptr) },
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn validate_wasm() {
        assert_eq!(
            validate(&[0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00]),
            true
        );
        assert_eq!(
            validate(&[0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x01]),
            false
        );
        assert_eq!(validate(&[0x00]), false);
    }

    #[test]
    fn parse_wasm() {
        assert!(parse(&[0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00]).is_some());
        assert!(parse(&[0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x01]).is_none());
    }
}
