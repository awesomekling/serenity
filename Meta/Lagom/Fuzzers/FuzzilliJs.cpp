/*
 * Copyright (c) 2020, the SerenityOS developers.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <AK/Function.h>
#include <AK/String.h>
#include <AK/StringView.h>
#include <LibJS/Forward.h>
#include <LibJS/Interpreter.h>
#include <LibJS/Lexer.h>
#include <LibJS/Parser.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <signal.h>

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <sys/mman.h>

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

//
// BEGIN FUZZING CODE
//

#define REPRL_CRFD 100
#define REPRL_CWFD 101
#define REPRL_DRFD 102
#define REPRL_DWFD 103
#define REPRL_MAX_DATA_SIZE (16 * 1024 * 1024)

#define SHM_SIZE 0x100000
#define MAX_EDGES ((SHM_SIZE - 4) * 8)

#define CHECK(cond)                                \
    if (!(cond)) {                                 \
        fprintf(stderr, "\"" #cond "\" failed\n"); \
        _exit(-1);                                 \
    }

struct shmem_data {
    uint32_t num_edges;
    unsigned char edges[];
};

struct shmem_data* __shmem;
uint32_t *__edges_start, *__edges_stop;

void __sanitizer_cov_reset_edgeguards()
{
    uint64_t N = 0;
    for (uint32_t* x = __edges_start; x < __edges_stop && N < MAX_EDGES; x++)
        *x = ++N;
}

extern "C" void __sanitizer_cov_trace_pc_guard_init(uint32_t* start, uint32_t* stop)
{
    // Avoid duplicate initialization
    if (start == stop || *start)
        return;

    if (__edges_start != NULL || __edges_stop != NULL) {
        fprintf(stderr, "Coverage instrumentation is only supported for a single module\n");
        _exit(-1);
    }

    __edges_start = start;
    __edges_stop = stop;

    // Map the shared memory region
    const char* shm_key = getenv("SHM_ID");
    if (!shm_key) {
        puts("[COV] no shared memory bitmap available, skipping");
        __shmem = (struct shmem_data*)malloc(SHM_SIZE);
    } else {
        int fd = shm_open(shm_key, O_RDWR, S_IREAD | S_IWRITE);
        if (fd <= -1) {
            fprintf(stderr, "Failed to open shared memory region: %s\n", strerror(errno));
            _exit(-1);
        }

        __shmem = (struct shmem_data*)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (__shmem == MAP_FAILED) {
            fprintf(stderr, "Failed to mmap shared memory region\n");
            _exit(-1);
        }
    }

    __sanitizer_cov_reset_edgeguards();

    __shmem->num_edges = stop - start;
    printf("[COV] edge counters initialized. Shared memory: %s with %u edges\n", shm_key, __shmem->num_edges);
}

extern "C" void __sanitizer_cov_trace_pc_guard(uint32_t* guard)
{
    // There's a small race condition here: if this function executes in two threads for the same
    // edge at the same time, the first thread might disable the edge (by setting the guard to zero)
    // before the second thread fetches the guard value (and thus the index). However, our
    // instrumentation ignores the first edge (see libcoverage.c) and so the race is unproblematic.
    uint32_t index = *guard;
    // If this function is called before coverage instrumentation is properly initialized we want to return early.
    if (!index)
        return;
    __shmem->edges[index / 8] |= 1 << (index % 8);
    *guard = 0;
}

//
// END FUZZING CODE
//

class TestRunnerGlobalObject : public JS::GlobalObject {
public:
    TestRunnerGlobalObject();
    virtual ~TestRunnerGlobalObject() override;

    virtual void initialize() override;

private:
    virtual const char* class_name() const override { return "TestRunnerGlobalObject"; }

    JS_DECLARE_NATIVE_FUNCTION(fuzzilli);
};

TestRunnerGlobalObject::TestRunnerGlobalObject()
{
}

TestRunnerGlobalObject::~TestRunnerGlobalObject()
{
}

JS_DEFINE_NATIVE_FUNCTION(TestRunnerGlobalObject::fuzzilli)
{
    if (!vm.argument_count())
        return JS::js_undefined();

    auto operation = vm.argument(0).to_string(global_object);
    if (vm.exception())
        return {};

    if (operation == "FUZZILLI_CRASH") {
        auto type = vm.argument(1).to_i32(global_object);
        if (vm.exception())
            return {};
        switch (type) {
        case 0:
            *((int*)0x41414141) = 0x1337;
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    } else if (operation == "FUZZILLI_PRINT") {
        static FILE* fzliout = fdopen(REPRL_DWFD, "w");
        if (!fzliout) {
            dbgln("Fuzzer output not available");
            fzliout = stdout;
        }

        auto string = vm.argument(1).to_string(global_object);
        if (vm.exception())
            return {};
        fprintf(fzliout, "%s\n", string.characters());
        fflush(fzliout);
    }

    return JS::js_undefined();
}

void TestRunnerGlobalObject::initialize()
{
    JS::GlobalObject::initialize();
    define_property("global", this, JS::Attribute::Enumerable);
    define_native_function("fuzzilli", fuzzilli, 2);
}

int main(int, char**)
{
    char* reprl_input = nullptr;

    char helo[] = "HELO";
    if (write(REPRL_CWFD, helo, 4) != 4 || read(REPRL_CRFD, helo, 4) != 4) {
        ASSERT_NOT_REACHED();
    }

    ASSERT(memcmp(helo, "HELO", 4) == 0);
    reprl_input = (char*)mmap(0, REPRL_MAX_DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, REPRL_DRFD, 0);
    ASSERT(reprl_input != MAP_FAILED);

    auto vm = JS::VM::create();
    auto interpreter = JS::Interpreter::create<TestRunnerGlobalObject>(*vm);

    while (true) {
        unsigned action;
        ASSERT(read(REPRL_CRFD, &action, 4) == 4);
        ASSERT(action == 'cexe');

        size_t script_size;
        ASSERT(read(REPRL_CRFD, &script_size, 8) == 8);
        ASSERT(script_size < REPRL_MAX_DATA_SIZE);
        ByteBuffer data_buffer;
        if (data_buffer.size() < script_size)
            data_buffer.grow(script_size - data_buffer.size());
        ASSERT(data_buffer.size() >= script_size);
        memcpy(data_buffer.data(), reprl_input, script_size);

        int result = 0;

        auto js = AK::StringView(static_cast<const unsigned char*>(data_buffer.data()), script_size);

        auto lexer = JS::Lexer(js);
        auto parser = JS::Parser(lexer);
        auto program = parser.parse_program();
        if (parser.has_errors()) {
            result = 1;
        } else {
            interpreter->run(interpreter->global_object(), *program);
            if (interpreter->exception()) {
                result = 1;
                vm->clear_exception();
            }
        }

        fflush(stdout);
        fflush(stderr);

        int status = (result & 0xff) << 8;
        ASSERT(write(REPRL_CWFD, &status, 4) == 4);
        __sanitizer_cov_reset_edgeguards();
    }

    return 0;
}
