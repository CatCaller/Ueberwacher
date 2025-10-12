#include "exit_tracer.hpp"
#include "ia32.hpp"

namespace {
    inline bool rip_in_range(std::uint64_t rip, std::uint64_t base, std::uint64_t end) noexcept {
        return rip >= base && rip < end;
    }
}

void init_exit_tracer(ExitTracer& t) {
    t.tracing = false;
    t.storage = nullptr;
    t.rip_base = 0;
    t.rip_end = ~0ull;
}

bool start_exit_tracer(ExitTracer& t, ExitStorage* storage, std::uint64_t rip_base, std::uint64_t rip_end) {
    if (t.tracing) {
        return true;
    }

    if (!storage || !storage->events || storage->capacity == 0) {
        return false;
    }
    if (!(rip_base < rip_end)) {
        return false;
    }

    t.storage = storage;
    t.rip_base = rip_base;
    t.rip_end = rip_end;
    t.tracing = true;
    return true;
}

void stop_exit_tracer(ExitTracer& t) {
    t.tracing = false;
    t.storage = nullptr;
    t.rip_base = 0;
    t.rip_end = ~0ull;
}

bool log_vmexit(ExitTracer& t, const ExitContext& ctx) {
    if (!t.tracing) {
        return false;
    }
    if (!t.storage) {
        return false;
    }
    ExitStorage& s = *t.storage;
    if (!s.enabled) {
        return false;
    }
    if (!rip_in_range(ctx.rip, t.rip_base, t.rip_end)) {
        return false; // filtered out by rip range
    }
    if (s.count >= s.capacity) {
        return false; // out of space
    }

    ExitEvent e{}; // zero-initialize
    switch (ctx.exit_reason) {
        case VMX_EXIT_REASON_EXECUTE_VMCALL:
            e.type = ExitReason::Vmcall;
            e.vmcall.signature = ctx.rcx;
            break;
        case VMX_EXIT_REASON_EXECUTE_CPUID:
            e.type = ExitReason::Cpuid;
            e.cpuid.leaf = static_cast<std::uint32_t>(ctx.rax);
            e.cpuid.subleaf = static_cast<std::uint32_t>(ctx.rcx);
            break;
        case VMX_EXIT_REASON_EXECUTE_RDMSR:
            e.type = ExitReason::Rdmsr;
            e.rdmsr.msr_id = static_cast<std::uint32_t>(ctx.rcx);
            break;
        case VMX_EXIT_REASON_EXECUTE_WRMSR:
            e.type = ExitReason::Wrmsr;
            e.wrmsr.msr_id = static_cast<std::uint32_t>(ctx.rcx);
            e.wrmsr.value = (ctx.rdx << 32) | (ctx.rax & 0xFFFFFFFFull);
            break;
        case VMX_EXIT_REASON_EXECUTE_HLT:
            e.type = ExitReason::Hlt;
            // empty payload
            break;
        default:
            e.type = ExitReason::Unknown;
            e.unknown.code = 0; //opaque placeholder
            break;
    }

    s.events[s.count] = e;
    ++s.count;
    return true;
}
