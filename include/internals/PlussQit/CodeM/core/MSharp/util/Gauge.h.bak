#pragma once

#include <memory>
#include <string_view>

#include <MSharp/macros/Macros.h>
#include <MSharp/util/SmallVector.h>

namespace MSharp::monitor {
namespace detail {

class GaugeImpl;

class GaugeBackendIf {
 public:
  virtual ~GaugeBackendIf() = default;
  virtual void record(int64_t value) noexcept = 0;
};

class GaugeBackendFactoryIf {
 public:
  virtual ~GaugeBackendFactoryIf() = default;

  // May return nullptr if the gauge will be ignored by the given backend.
  virtual std::unique_ptr<GaugeBackendIf> create(
      std::string_view key) noexcept = 0;
};

void C10_API registerGaugeBackend(std::unique_ptr<GaugeBackendFactoryIf>);
} // namespace detail

// A handle to a Gauge.
class C10_API GaugeHandle {
 public:
  explicit GaugeHandle(std::string_view key);
  void record(int64_t value);

 private:
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  detail::GaugeImpl& impl_;
};

} // namespace MSharp::monitor

#define STATIC_GAUGE(_key)                            \
  []() -> ::MSharp::monitor::GaugeHandle& {              \
    static ::MSharp::monitor::GaugeHandle handle(#_key); \
    return handle;                                    \
  }()
