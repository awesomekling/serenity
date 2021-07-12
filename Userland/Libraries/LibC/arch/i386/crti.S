/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

.section .init, "ax", @progbits
.align 4
.global _init
.type _init, @function
_init:
    sub $12, %esp


.section .fini, "ax", @progbits
.align 4
.global _fini
.type _fini, @function
_fini:
    sub $12, %esp
