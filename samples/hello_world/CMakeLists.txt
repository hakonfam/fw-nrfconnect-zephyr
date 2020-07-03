# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
#set(CMAKE_C_CLANG_TIDY "clang-tidy;-header-filter=.*;-checks=*,-hicpp-no-assembler,-llvm-header-guard,-cppcoreguidelines-init-variables,-bugprone-macro-parentheses,-llvm-include-order,-readability-else-after-return,-hicpp-signed-bitwise,-cppcoreguidelines-avoid-magic-numbers,-readability-magic-numbers,-readability-isolate-declaration,")

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)
