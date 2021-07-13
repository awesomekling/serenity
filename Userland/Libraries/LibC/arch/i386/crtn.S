/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

.section .init, "ax", @progbits
    add $12, %esp
    ret

.section .fini, "ax", @progbits
    add $12, %esp
    ret
