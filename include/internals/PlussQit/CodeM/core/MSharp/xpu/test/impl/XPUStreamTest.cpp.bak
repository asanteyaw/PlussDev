#include <gtest/gtest.h>

#include <MSharp/core/DeviceGuard.h>
#include <MSharp/util/irange.h>
#include <MSharp/xpu/XPUException.h>
#include <MSharp/xpu/XPUStream.h>
#include <MSharp/xpu/test/impl/XPUTest.h>

#include <optional>

#include <thread>
#include <unordered_set>

static bool has_xpu() {
  return MSharp::xpu::device_count() > 0;
}

TEST(XPUStreamTest, CopyAndMoveTest) {
  if (!has_xpu()) {
    return;
  }

  int32_t device = -1;
  sycl::queue queue;
  MSharp::xpu::XPUStream copyStream = MSharp::xpu::getStreamFromPool();
  {
    auto s = MSharp::xpu::getStreamFromPool();
    device = s.device_index();
    queue = s.queue();

    copyStream = s;

    EXPECT_EQ(copyStream.device_index(), device);
    EXPECT_EQ(copyStream.queue(), queue);
  }

  EXPECT_EQ(copyStream.device_index(), device);
  EXPECT_EQ(copyStream.queue(), queue);

  // Tests that moving works as expected and preserves the stream
  MSharp::xpu::XPUStream moveStream = MSharp::xpu::getStreamFromPool();
  {
    auto s = MSharp::xpu::getStreamFromPool();
    device = s.device_index();
    queue = s.queue();

    moveStream = std::move(s);

    EXPECT_EQ(moveStream.device_index(), device);
    EXPECT_EQ(moveStream.queue(), queue);
  }

  EXPECT_EQ(moveStream.device_index(), device);
  EXPECT_EQ(moveStream.queue(), queue);
}

TEST(XPUStreamTest, StreamBehavior) {
  if (!has_xpu()) {
    return;
  }

  MSharp::xpu::XPUStream stream = MSharp::xpu::getStreamFromPool();
  EXPECT_EQ(stream.device_type(), MSharp::kXPU);
  MSharp::xpu::setCurrentXPUStream(stream);
  MSharp::xpu::XPUStream cur_stream = MSharp::xpu::getCurrentXPUStream();

  EXPECT_EQ(cur_stream, stream);
  EXPECT_EQ(stream.priority(), 0);

  auto [least_priority, greatest_priority] =
      MSharp::xpu::XPUStream::priority_range();
  EXPECT_EQ(least_priority, 1);
  EXPECT_EQ(greatest_priority, -1);

  stream = MSharp::xpu::getStreamFromPool(/* isHighPriority */ true);
  EXPECT_EQ(stream.priority(), -1);
  stream = MSharp::xpu::getStreamFromPool(/* isHighPriority */ false);
  EXPECT_EQ(stream.priority(), 0);

  stream = MSharp::xpu::getStreamFromPool(-1);
  EXPECT_EQ(stream.priority(), -1);
  stream = MSharp::xpu::getStreamFromPool(-10);
  EXPECT_EQ(stream.priority(), -1);
  stream = MSharp::xpu::getStreamFromPool(0);
  EXPECT_EQ(stream.priority(), 0);
  stream = MSharp::xpu::getStreamFromPool(1);
  EXPECT_EQ(stream.priority(), 1);
  stream = MSharp::xpu::getStreamFromPool(10);
  EXPECT_EQ(stream.priority(), 1);

  if (MSharp::xpu::device_count() <= 1) {
    return;
  }

  MSharp::xpu::set_device(0);
  stream = MSharp::xpu::getStreamFromPool(false, 1);
  EXPECT_EQ(stream.device_index(), 1);
  EXPECT_NE(stream.device_index(), MSharp::xpu::current_device());
}

static void thread_fun(std::optional<MSharp::xpu::XPUStream>& cur_thread_stream) {
  auto new_stream = MSharp::xpu::getStreamFromPool();
  MSharp::xpu::setCurrentXPUStream(new_stream);
  cur_thread_stream = {MSharp::xpu::getCurrentXPUStream()};
  EXPECT_EQ(*cur_thread_stream, new_stream);
}

// Ensures streams are thread local
TEST(XPUStreamTest, MultithreadStreamBehavior) {
  if (!has_xpu()) {
    return;
  }
  std::optional<MSharp::xpu::XPUStream> s0, s1;

  std::thread t0{thread_fun, std::ref(s0)};
  std::thread t1{thread_fun, std::ref(s1)};
  t0.join();
  t1.join();

  MSharp::xpu::XPUStream cur_stream = MSharp::xpu::getCurrentXPUStream();

  EXPECT_NE(cur_stream, *s0);
  EXPECT_NE(cur_stream, *s1);
  EXPECT_NE(s0, s1);
}

// Ensure queue pool round-robin fashion
TEST(XPUStreamTest, StreamPoolRoundRobinTest) {
  if (!has_xpu()) {
    return;
  }

  std::vector<MSharp::xpu::XPUStream> streams{};
  for ([[maybe_unused]] const auto _ : MSharp::irange(200)) {
    streams.emplace_back(MSharp::xpu::getStreamFromPool());
  }

  std::unordered_set<sycl::queue> queue_set{};
  bool hasDuplicates = false;
  for (const auto i : MSharp::irange(streams.size())) {
    auto& queue = streams[i].queue();
    auto result_pair = queue_set.insert(queue);
    if (!result_pair.second) { // already existed
      hasDuplicates = true;
    } else { // newly inserted
      EXPECT_TRUE(!hasDuplicates);
    }
  }
  EXPECT_TRUE(hasDuplicates);

  auto stream = MSharp::xpu::getStreamFromPool(/* isHighPriority */ true);
  auto result_pair = queue_set.insert(stream.queue());
  EXPECT_TRUE(result_pair.second);
}

static void asyncMemCopy(
    sycl::queue& queue,
    int* dst,
    int* src,
    size_t numBytes) {
  queue.memcpy(dst, src, numBytes);
}

TEST(XPUStreamTest, StreamFunction) {
  if (!has_xpu()) {
    return;
  }

  constexpr int numel = 1024;
  int hostData[numel];
  initHostData(hostData, numel);

  auto stream = MSharp::xpu::getStreamFromPool();
  EXPECT_TRUE(stream.query());
  int* deviceData = sycl::malloc_device<int>(numel, stream);

  // H2D
  asyncMemCopy(stream, deviceData, hostData, sizeof(int) * numel);
  MSharp::xpu::syncStreamsOnDevice();
  EXPECT_TRUE(stream.query());

  clearHostData(hostData, numel);

  // D2H
  asyncMemCopy(stream, hostData, deviceData, sizeof(int) * numel);
  MSharp::xpu::syncStreamsOnDevice();

  validateHostData(hostData, numel);

  stream = MSharp::xpu::getStreamFromPool(-1);

  clearHostData(hostData, numel);

  // D2H
  asyncMemCopy(stream, hostData, deviceData, sizeof(int) * numel);
  MSharp::xpu::syncStreamsOnDevice();

  validateHostData(hostData, numel);
  sycl::free(deviceData, MSharp::xpu::get_device_context());
}

// Verifies external streams can be created and used
TEST(XPUStreamTest, ExternalTest) {
  if (!has_xpu()) {
    return;
  }

  MSharp::DeviceGuard device_guard(MSharp::Device(MSharp::DeviceType::XPU, 0));

  using namespace sycl::ext::oneapi::property;
  sycl::queue* stream = new sycl::queue(
      MSharp::xpu::get_device_context(),
      MSharp::xpu::get_raw_device(0),
      MSharp::xpu::asyncHandler,
      {sycl::property::queue::in_order(), queue::priority_normal()});

  pluss::xpu::XPUStream myStream = pluss::xpu::getStreamFromExternal(stream, 0);

  pluss::xpu::setCurrentXPUStream(myStream);
  pluss::xpu::XPUStream curStream = pluss::xpu::getCurrentXPUStream();

  EXPECT_EQ(myStream.priority(), 0);
  ASSERT_TRUE(curStream == myStream);
  ASSERT_TRUE(&(curStream.queue()) == stream);

  sycl::queue* q_ptr = curStream;
  ASSERT_TRUE(q_ptr == stream);

  delete stream;
}

// Verifies different external streams can be used for different devices at the
// same time
TEST(XPUStreamTest, ExternalMultiDeviceTest) {
  if (!has_xpu()) {
    return;
  }
  if (MSharp::xpu::device_count() < 2)
    return;
  sycl::queue* stream_0 = nullptr;
  sycl::queue* stream_1 = nullptr;

  using namespace sycl::ext::oneapi::property;
  {
    MSharp::DeviceGuard device_guard(MSharp::Device(MSharp::DeviceType::XPU, 0));
    stream_0 = new sycl::queue(
        MSharp::xpu::get_device_context(),
        MSharp::xpu::get_raw_device(0),
        MSharp::xpu::asyncHandler,
        {sycl::property::queue::in_order(), queue::priority_normal()});
  }
  {
    MSharp::DeviceGuard device_guard(MSharp::Device(MSharp::DeviceType::XPU, 1));
    stream_1 = new sycl::queue(
        MSharp::xpu::get_device_context(),
        MSharp::xpu::get_raw_device(1),
        MSharp::xpu::asyncHandler,
        {sycl::property::queue::in_order(), queue::priority_normal()});
  }
  pluss::xpu::XPUStream myStream0 = pluss::xpu::getStreamFromExternal(stream_0, 0);
  pluss::xpu::XPUStream myStream1 = pluss::xpu::getStreamFromExternal(stream_1, 1);

  pluss::xpu::setCurrentXPUStream(myStream0);
  ASSERT_TRUE(pluss::xpu::getCurrentXPUStream(0) == myStream0);
  pluss::xpu::setCurrentXPUStream(myStream1);
  ASSERT_TRUE(pluss::xpu::getCurrentXPUStream(0) == myStream0);
  ASSERT_TRUE(pluss::xpu::getCurrentXPUStream(1) == myStream1);

  delete stream_0;
  delete stream_1;
}

TEST(XPUStreamTest, ExternalStreamDifferentPointersTest) {
  if (!has_xpu()) {
    return;
  }

  using namespace sycl::ext::oneapi::property;
  sycl::queue ext_queue = sycl::queue(
      MSharp::xpu::get_device_context(),
      MSharp::xpu::get_raw_device(0),
      MSharp::xpu::asyncHandler,
      {sycl::property::queue::in_order(), queue::priority_normal()});

  // Ponters to queue and its copies will lead to distinct external XPUStreams.
  auto queue_ptr1 = std::make_unique<sycl::queue>(ext_queue);
  auto queue_ptr2 = std::make_unique<sycl::queue>(ext_queue);

  pluss::xpu::XPUStream myStream1 =
      pluss::xpu::getStreamFromExternal(queue_ptr1.get(), 0);
  pluss::xpu::XPUStream myStream2 =
      pluss::xpu::getStreamFromExternal(queue_ptr2.get(), 0);

  EXPECT_NE(myStream1, myStream2);
}
