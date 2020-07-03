#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.13.1)
#set(CMAKE_C_CLANG_TIDY "clang-tidy;-header-filter=.*;-line-filter=[{\"name\":\"stream_flash.c\"}];-checks=*,-hicpp-no-assembler,-llvm-header-guard,-cppcoreguidelines-init-variables,-bugprone-macro-parentheses,-llvm-include-order,-readability-else-after-return,-hicpp-signed-bitwise,-cppcoreguidelines-avoid-magic-numbers,-readability-magic-numbers,-readability-isolate-declaration,")
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(stream_flash)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
