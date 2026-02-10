#pragma once

#include <cstdint>
#include <memory>

namespace MSharp {

enum class DebugInfoKind : uint8_t {
	PRODUCER_INFO = 0,
	MOBILE_RUNTIME_INFO,
	PROFILER_STATE,
	INFERENCE_CONTEXT,
	PARAM_COMMS_INFO,
	TEST_INFO,
	TEST_INFO_2,
};

class DebugInfoBase {
 public:
	DebugInfoBase() = default;
	virtual ~DebugInfoBase() = default;
};

class ThreadLocalDebugInfo {
 public:
	static DebugInfoBase* get(DebugInfoKind) { return nullptr; }
	static std::shared_ptr<ThreadLocalDebugInfo> current() { return nullptr; }
	static void _push(DebugInfoKind, std::shared_ptr<DebugInfoBase>) {}
	static std::shared_ptr<DebugInfoBase> _pop(DebugInfoKind) { return nullptr; }
	static std::shared_ptr<DebugInfoBase> _peek(DebugInfoKind) { return nullptr; }
};

class DebugInfoGuard {
 public:
	DebugInfoGuard(DebugInfoKind, std::shared_ptr<DebugInfoBase>) {}
	explicit DebugInfoGuard(std::shared_ptr<ThreadLocalDebugInfo>) {}
	~DebugInfoGuard() {}
};

} // namespace MSharp
