#include <gtest/gtest.h>

#include <MSharp/core/DeviceGuard.h>
#include <MSharp/core/Event.h>
#include <MSharp/xpu/XPUStream.h>
#include <MSharp/xpu/test/impl/XPUTest.h>

static bool has_xpu() {
  return MSharp::xpu::device_count() > 0;
}

TEST(XPUGuardTest, GuardBehavior) {
  if (!has_xpu()) {
    return;
  }

  {
    auto device = MSharp::Device(MSharp::kXPU);
    const MSharp::DeviceGuard device_guard(device);
    EXPECT_EQ(MSharp::xpu::current_device(), 0);
  }

  std::vector<MSharp::xpu::XPUStream> streams0 = {
      MSharp::xpu::getStreamFromPool(), MSharp::xpu::getStreamFromPool(true)};
  EXPECT_EQ(streams0[0].device_index(), 0);
  EXPECT_EQ(streams0[1].device_index(), 0);
  MSharp::xpu::setCurrentXPUStream(streams0[0]);
  EXPECT_EQ(MSharp::xpu::getCurrentXPUStream(), streams0[0]);

  if (MSharp::xpu::device_count() <= 1) {
    return;
  }

  // Test DeviceGuard for XPU.
  std::vector<MSharp::xpu::XPUStream> streams1;
  {
    auto device = MSharp::Device(MSharp::kXPU, 1);
    const MSharp::DeviceGuard device_guard(device);
    streams1.push_back(MSharp::xpu::getStreamFromPool());
    streams1.push_back(MSharp::xpu::getStreamFromPool());
  }

  EXPECT_EQ(streams1[0].device_index(), 1);
  EXPECT_EQ(streams1[1].device_index(), 1);
  EXPECT_EQ(MSharp::xpu::current_device(), 0);
}

TEST(XPUGuardTest, EventBehavior) {
  if (!has_xpu()) {
    return;
  }

  auto device = MSharp::Device(MSharp::kXPU, MSharp::xpu::current_device());
  MSharp::impl::VirtualGuardImpl impl(device.type());
  MSharp::Stream stream1 = impl.getStream(device);
  MSharp::Stream stream2 = impl.getStream(device);
  MSharp::Event event1(device.type(), MSharp::EventFlag::BACKEND_DEFAULT);
  // event is lazily created.
  EXPECT_FALSE(event1.eventId());

  constexpr int numel = 1024;
  int hostData1[numel];
  initHostData(hostData1, numel);
  int hostData2[numel];
  clearHostData(hostData2, numel);

  auto xpu_stream1 = MSharp::xpu::XPUStream(stream1);
  int* deviceData1 = sycl::malloc_device<int>(numel, xpu_stream1);

  // Copy hostData1 to deviceData1 via stream1, and then copy deviceData1 to
  // hostData2 via stream2.
  xpu_stream1.queue().memcpy(deviceData1, hostData1, sizeof(int) * numel);
  // stream2 wait on stream1's completion.
  event1.record(stream1);
  event1.block(stream2);
  auto xpu_stream2 = MSharp::xpu::XPUStream(stream2);
  xpu_stream2.queue().memcpy(hostData2, deviceData1, sizeof(int) * numel);
  xpu_stream2.synchronize();

  EXPECT_TRUE(event1.query());
  validateHostData(hostData2, numel);
  event1.record(stream2);
  event1.synchronize();
  EXPECT_TRUE(event1.query());

  clearHostData(hostData2, numel);
  xpu_stream1.queue().memcpy(deviceData1, hostData1, sizeof(int) * numel);
  // stream2 wait on stream1's completion.
  event1.record(stream1);
  event1.block(stream2);
  // event1 will overwrite the previously captured state.
  event1.record(stream2);
  xpu_stream2.queue().memcpy(hostData2, deviceData1, sizeof(int) * numel);
  xpu_stream2.synchronize();
  EXPECT_TRUE(event1.query());
  validateHostData(hostData2, numel);

  clearHostData(hostData2, numel);
  // ensure deviceData1 and deviceData2 are different buffers.
  int* deviceData2 = sycl::malloc_device<int>(numel, xpu_stream1);
  sycl::free(deviceData1, MSharp::xpu::get_device_context());
  MSharp::Event event2(device.type(), MSharp::EventFlag::BACKEND_DEFAULT);

  // Copy hostData1 to deviceData2 via stream1, and then copy deviceData2 to
  // hostData1 via stream1.
  xpu_stream1.queue().memcpy(deviceData2, hostData1, sizeof(int) * numel);
  event2.record(xpu_stream1);
  event2.synchronize();
  EXPECT_TRUE(event2.query());
  clearHostData(hostData1, numel);
  xpu_stream1.queue().memcpy(hostData1, deviceData2, sizeof(int) * numel);
  event2.record(xpu_stream1);
  event2.synchronize();
  EXPECT_TRUE(event2.query());
  EXPECT_NE(event1.eventId(), event2.eventId());
#if SYCL_COMPILER_VERSION < 20250000
  ASSERT_THROW(event1.elapsedTime(event2), MSharp::Error);
#else
  event1.elapsedTime(event2);
#endif
  sycl::free(deviceData2, MSharp::xpu::get_device_context());
}
