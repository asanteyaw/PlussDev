#include <gtest/gtest.h>

#include <MSharp/xpu/XPUFunctions.h>

static bool has_xpu() {
  return MSharp::xpu::device_count() > 0;
}

TEST(XPUDeviceTest, DeviceBehavior) {
  if (!has_xpu()) {
    return;
  }

  MSharp::xpu::set_device(0);
  EXPECT_EQ(MSharp::xpu::current_device(), 0);

  if (MSharp::xpu::device_count() <= 1) {
    return;
  }

  MSharp::xpu::set_device(1);
  EXPECT_EQ(MSharp::xpu::current_device(), 1);
  EXPECT_EQ(MSharp::xpu::exchange_device(0), 1);
  EXPECT_EQ(MSharp::xpu::current_device(), 0);
}

TEST(XPUDeviceTest, DeviceProperties) {
  if (!has_xpu()) {
    return;
  }

  MSharp::xpu::DeviceProp device_prop{};
  MSharp::xpu::get_device_properties(&device_prop, 0);

  EXPECT_TRUE(device_prop.max_compute_units > 0);
  EXPECT_TRUE(device_prop.gpu_eu_count > 0);
}

TEST(XPUDeviceTest, PointerGetDevice) {
  if (!has_xpu()) {
    return;
  }

  sycl::device& raw_device = MSharp::xpu::get_raw_device(0);
  void* ptr =
      sycl::malloc_device(8, raw_device, MSharp::xpu::get_device_context());

  EXPECT_EQ(MSharp::xpu::get_device_idx_from_pointer(ptr), 0);
  sycl::free(ptr, MSharp::xpu::get_device_context());

  int dummy = 0;
  ASSERT_THROW(MSharp::xpu::get_device_idx_from_pointer(&dummy), MSharp::Error);
}
