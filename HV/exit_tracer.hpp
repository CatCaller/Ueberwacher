#pragma once

#include <cstdint>


enum class ExitReason { Vmcall, Cpuid, Rdmsr, Wrmsr, Hlt, Unknown };

struct ExitEvent {
    ExitReason type{};
    union {
        struct { std::uint64_t signature; } vmcall;               // from rcx
        struct { std::uint32_t leaf, subleaf; } cpuid;            // from rax (leaf), rcx (subleaf)
        struct { std::uint32_t msr_id; } rdmsr;                   // from rcx
        struct { std::uint32_t msr_id; std::uint64_t value; } wrmsr; // msr_id from rcx, value=(rdx<<32)|(rax & 0xFFFFFFFF)
        struct { } hlt;                                           // empty
        struct { std::uint64_t code; } unknown;                   // generic payload (MVP sets 0)
    };
};

struct ExitStorage {
    ExitEvent* events{nullptr};   // caller-allocated array
    std::uint32_t capacity{0};    // max number of events
    std::uint32_t count{0};       // incremented on store
    bool enabled{true};           // if false, log calls are no-ops (return false)
};

struct ExitTracer {
    bool tracing{false};            // true when started
    ExitStorage* storage{nullptr};  // not owned
    std::uint64_t rip_base{0};      // inclusive
    std::uint64_t rip_end{~0ull};   // exclusive
};

struct ExitContext {
    uint32_t exit_reason;
    std::uint64_t rip{0};                  // guest rip
    std::uint64_t rax{0};                  // cpuid leaf, wrmsr low
    std::uint64_t rcx{0};                  // vmcall signature, cpuid subleaf, msr id
    std::uint64_t rdx{0};                  // wrmsr high
};

void init_exit_tracer(ExitTracer& t);
bool start_exit_tracer(ExitTracer& t, ExitStorage* storage, std::uint64_t rip_base = 0, std::uint64_t rip_end = ~0ull);
void stop_exit_tracer(ExitTracer& t);
bool log_vmexit(ExitTracer& t, const ExitContext& ctx);
