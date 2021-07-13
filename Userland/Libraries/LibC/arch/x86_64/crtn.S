/*
 * Copyright (c) 2021, Daniel Bertalan <dani@danielbertalan.dev>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

.section .init, "ax", @progbits
    pop %rbp
    ret

.section .init, "ax", @progbits
.section .fini
    pop %rbp
    ret
