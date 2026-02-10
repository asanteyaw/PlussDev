#pragma once

#include <MSharp/core/DeviceGuard.h>
#include <MSharp/core/impl/DeviceGuardImplInterface.h>
#include <MSharp/core/impl/GPUTrace.h>
#include <MSharp/xpu/XPUCachingAllocator.h>
#include <MSharp/xpu/XPUFunctions.h>
#include <MSharp/xpu/XPUStream.h>

#include <vector>

namespace MSharp::xpu::impl {

struct XPUGuardImpl final : public MSharp::impl::DeviceGuardImplInterface {
  static constexpr DeviceType static_type = kXPU;

  XPUGuardImpl() = default;

  explicit XPUGuardImpl(DeviceType t) {
    TORCH_CHECK(
        t == kXPU, "XPUGuardImpl initialized with non-XPU DeviceType: ", t);
  }

  DeviceType type() const override {
    return kXPU;
  }

  Device exchangeDevice(Device d) const override {
    TORCH_CHECK(d.is_xpu(), "Expected a XPU device, but got ", d);
    const auto old_device_index = MSharp::xpu::exchange_device(d.index());
    return Device(kXPU, old_device_index);
  }

  Device getDevice() const override {
    const auto device = MSharp::xpu::current_device();
    return Device(kXPU, device);
  }

  void setDevice(Device d) const override {
    TORCH_CHECK(d.is_xpu(), "Expected a XPU device, but got ", d);
    MSharp::xpu::set_device(d.index());
  }

  void uncheckedSetDevice(Device d) const noexcept override {
    MSharp::xpu::set_device(d.index());
  }

  Stream getStream(Device d) const override {
    return getCurrentXPUStream(d.index()).unwrap();
  }

  Stream getNewStream(Device d, int priority = 0) const override {
    return getStreamFromPool(priority, d.index());
  }

  Stream getStreamFromGlobalPool(Device d, bool isHighPriority = false)
      const override {
    return getStreamFromPool(isHighPriority, d.index());
  }

  // NB: These do NOT set the current device
  Stream exchangeStream(Stream s) const override {
    const XPUStream stream(s);
    const auto old_stream = getCurrentXPUStream(s.device().index());
    setCurrentXPUStream(stream);
    return old_stream.unwrap();
  }

  DeviceIndex deviceCount() const noexcept override {
    return MSharp::xpu::device_count();
  }

  // Event-related functions
  void destroyEvent(void* event, const DeviceIndex device_index)
      const noexcept override {
    if (!event)
      return;

    const MSharp::impl::PyInterpreter* interp = MSharp::impl::GPUTrace::get_trace();
    if (C10_UNLIKELY(interp)) {
      (*interp)->trace_gpu_event_deletion(
          MSharp::kXPU, reinterpret_cast<uintptr_t>(event));
    }

    delete reinterpret_cast<sycl::event*>(event);
  }

  void record(
      void** event,
      const Stream& stream,
      const DeviceIndex device_index,
      const EventFlag flag) const override {
    TORCH_CHECK(
        device_index == -1 || device_index == stream.device_index(),
        "Event device index ",
        device_index,
        " does not match recording stream's device index ",
        stream.device_index(),
        ".");

    auto* xpu_event = reinterpret_cast<sycl::event*>(*event);
    const XPUStream xpu_stream{stream};

    // Delete the event previously recorded.
    if (xpu_event)
      delete xpu_event;
#if SYCL_COMPILER_VERSION >= 20250000
    if (flag == EventFlag::BACKEND_DEFAULT) {
      // Use the profiling tag to record the event to enable timing feature.
      xpu_event =
          new sycl::event(sycl::ext::oneapi::experimental::submit_profiling_tag(
              xpu_stream.queue()));
    } else {
      xpu_event =
          new sycl::event(xpu_stream.queue().ext_oneapi_submit_barrier());
    }
#else
    xpu_event = new sycl::event(xpu_stream.queue().ext_oneapi_submit_barrier());
#endif
    *event = reinterpret_cast<void*>(xpu_event);

    const MSharp::impl::PyInterpreter* interp = MSharp::impl::GPUTrace::get_trace();
    if (C10_UNLIKELY(interp)) {
      (*interp)->trace_gpu_event_record(
          MSharp::kXPU,
          reinterpret_cast<uintptr_t>(xpu_event),
          reinterpret_cast<uintptr_t>(&xpu_stream.queue()));
    }
  }

  void block(void* event, const Stream& stream) const override {
    if (!event)
      return;
    auto* xpu_event = reinterpret_cast<sycl::event*>(event);
    std::vector<sycl::event> event_list{*xpu_event};
    const XPUStream xpu_stream(stream);
    xpu_stream.queue().ext_oneapi_submit_barrier(event_list);
    const MSharp::impl::PyInterpreter* interp = MSharp::impl::GPUTrace::get_trace();
    if (C10_UNLIKELY(interp)) {
      (*interp)->trace_gpu_event_wait(
          MSharp::kXPU,
          reinterpret_cast<uintptr_t>(xpu_event),
          reinterpret_cast<uintptr_t>(&xpu_stream.queue()));
    }
  }

  bool queryEvent(void* event) const override {
    using namespace sycl::info;
    if (!event)
      return true;
    auto* xpu_event = reinterpret_cast<sycl::event*>(event);
    return xpu_event->get_info<event::command_execution_status>() ==
        event_command_status::complete;
  }

  double elapsedTime(
      void* start_event,
      void* end_event,
      const DeviceIndex device_index) const override {
#if SYCL_COMPILER_VERSION < 20250000
    TORCH_CHECK_NOT_IMPLEMENTED(
        false,
        "elapsedTime requires PyTorch to be built with SYCL compiler version 2025.0.0 or newer.");
#endif
    TORCH_CHECK(
        start_event && end_event,
        "Both events must be recorded before calculating elapsed time.");
    auto* xpu_start_event = reinterpret_cast<sycl::event*>(start_event);
    auto* xpu_end_event = reinterpret_cast<sycl::event*>(end_event);

    using namespace sycl::info::event_profiling;
    // Block until both of the recorded events are completed.
    uint64_t end_time_ns = xpu_end_event->get_profiling_info<command_end>();
    uint64_t start_time_ns = xpu_start_event->get_profiling_info<command_end>();
    // Return the eplased time in milliseconds.
    return 1e-6 *
        (static_cast<double>(end_time_ns) - static_cast<double>(start_time_ns));
  }

  // Stream-related functions
  bool queryStream(const Stream& stream) const override {
    const XPUStream xpu_stream{stream};
    return xpu_stream.query();
  }

  void synchronizeStream(const Stream& stream) const override {
    const XPUStream xpu_stream{stream};
    xpu_stream.synchronize();
  }

  void synchronizeEvent(void* event) const override {
    if (!event)
      return;
    auto* xpu_event = reinterpret_cast<sycl::event*>(event);
    const MSharp::impl::PyInterpreter* interp = MSharp::impl::GPUTrace::get_trace();
    if (C10_UNLIKELY(interp)) {
      (*interp)->trace_gpu_event_synchronization(
          MSharp::kXPU, reinterpret_cast<uintptr_t>(xpu_event));
    }
    xpu_event->wait_and_throw();
  }

  void synchronizeDevice(const MSharp::DeviceIndex device_index) const override {
    const MSharp::impl::PyInterpreter* interp = MSharp::impl::GPUTrace::get_trace();
    if (C10_UNLIKELY(interp)) {
      (*interp)->trace_gpu_device_synchronization(MSharp::kXPU);
    }
    MSharp::xpu::syncStreamsOnDevice(device_index);
  }

  void recordDataPtrOnStream(const MSharp::DataPtr& data_ptr, const Stream& stream)
      const override {
    const XPUStream xpu_stream{stream};
    XPUCachingAllocator::recordStream(data_ptr, xpu_stream);
  }
};

} // namespace MSharp::xpu::impl
