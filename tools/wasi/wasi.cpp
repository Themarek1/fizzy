// Fizzy: A fast WebAssembly interpreter
// Copyright 2019-2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0

#include "execute.hpp"
#include "parser.hpp"
#include <uvwasi.h>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{
// Global state.
// NOTE: we do not care about uvwasi_destroy(), because it only clears file descriptors (currently)
// and we are a single-run tool. This may change in the future and should reevaluate.
uvwasi_t state;

bool run(int argc, const char** argv)
{
    uvwasi_options_t options = {
        3,           // sizeof fd_table
        0, nullptr,  // NOTE: no remappings
        static_cast<uvwasi_size_t>(argc), argv,
        nullptr,  // TODO: support env
        0, 1, 2,
        nullptr,  // NOTE: no special allocator
    };

    uvwasi_errno_t err = uvwasi_init(&state, &options);
    if (err != UVWASI_ESUCCESS)
    {
        std::cerr << "Failed to initialise UVWASI: " << uvwasi_embedder_err_code_to_string(err)
                  << "\n";
        return false;
    }

    // TODO: load wasm file

    return false;
}
}  // namespace

int main(int argc, const char** argv)
{
    try
    {
        if (argc < 2)
        {
            std::cerr << "Missing executable argument\n";
            return -1;
        }

        // Remove fizzy-wasi from the argv, but keep "argv[1]"
        const bool res = run(argc - 1, argv + 1);
        return res ? 0 : 1;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << "\n";
        return -2;
    }
}
