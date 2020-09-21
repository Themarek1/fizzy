// Fizzy: A fast WebAssembly interpreter
// Copyright 2019-2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0

mod sys;

pub fn validate(input: &[u8]) -> bool {
    unsafe { sys::fizzy_validate(input.as_ptr(), input.len()) }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn validate_wasm() {
        assert_eq!(validate(&[0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00]), true);
        assert_eq!(validate(&[0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x01]), false);
        assert_eq!(validate(&[0x00]), false);
    }
}
