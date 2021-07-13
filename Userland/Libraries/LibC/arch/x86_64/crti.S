/*
 * Copyright (c) 2021, Daniel Bertalan <dani@danielbertalan.dev>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

.section .init, "ax", @progbits
.align 4
.global _init
.type _init, @function
_init:
    push %rbp

.section .fini, "ax", @progbits
.align 4
.global _fini
.type _init, @function
_fini:
    push %rbp
