#include <pluss/pluss.h>
#include <pluss/array_file.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void require(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "array file test failed: " << message << '\n';
    ++failures;
  }
}

template <class F>
bool throws(F&& body) {
  try {
    body();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

// Files written by another numerical tool, kept as bytes so the test needs no data files:
// a column-major int32 array, a big-endian float64 array, and a deflated archive holding
// a float32 array of 100 values and an int64 2 x 2 array.
const std::string kColumnMajor = std::string("\x93\x4e\x55\x4d\x50\x59\x01\x00\x76\x00\x7b\x27\x64\x65\x73\x63\x72\x27\x3a\x20\x27\x3c\x69\x34\x27\x2c\x20\x27\x66\x6f\x72\x74\x72\x61\x6e\x5f\x6f\x72\x64\x65\x72\x27\x3a\x20\x54\x72\x75\x65\x2c\x20\x27\x73\x68\x61\x70\x65\x27\x3a\x20\x28\x32\x2c\x20\x33\x29\x2c\x20\x7d\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x0a\x00\x00\x00\x00\x03\x00\x00\x00\x01\x00\x00\x00\x04\x00\x00\x00\x02\x00\x00\x00\x05\x00\x00\x00", 152);
const std::string kBigEndian = std::string("\x93\x4e\x55\x4d\x50\x59\x01\x00\x76\x00\x7b\x27\x64\x65\x73\x63\x72\x27\x3a\x20\x27\x3e\x66\x38\x27\x2c\x20\x27\x66\x6f\x72\x74\x72\x61\x6e\x5f\x6f\x72\x64\x65\x72\x27\x3a\x20\x46\x61\x6c\x73\x65\x2c\x20\x27\x73\x68\x61\x70\x65\x27\x3a\x20\x28\x33\x2c\x29\x2c\x20\x7d\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x0a\x3f\xf8\x00\x00\x00\x00\x00\x00\xc0\x00\x00\x00\x00\x00\x00\x00\x40\x8f\x42\x00\x00\x00\x00\x00", 152);
const std::string kDeflated = std::string("\x50\x4b\x03\x04\x2d\x00\x00\x00\x08\x00\x00\x00\x21\x00\x50\xbf\xf6\xd5\xff\xff\xff\xff\xff\xff\xff\xff\x05\x00\x14\x00\x78\x2e\x6e\x70\x79\x01\x00\x10\x00\x10\x02\x00\x00\x00\x00\x00\x00\x11\x01\x00\x00\x00\x00\x00\x00\x9d\xc8\xa1\x4b\x03\x61\x1c\xc6\xf1\x9f\x22\x32\x86\xc8\x10\x91\x21\x43\x5e\x44\x7c\x4f\x39\xe4\x94\x21\x63\x0c\x3d\x0f\x19\x16\x65\x45\xd0\xe4\x0e\x77\x63\x41\xdc\xb8\x1b\x22\xc8\xe0\x82\xc1\x60\x18\x68\x30\x18\x16\x0c\x06\xc3\x82\xc1\x60\xb8\x60\x30\x18\x16\x0c\x06\xc3\x82\xc1\x60\x58\x30\x18\x0c\x7e\xc5\xff\xc0\x07\x3e\xe1\xf9\x9e\x6d\x6c\xae\x17\xb6\xfb\xe4\x40\x8e\x74\xc9\x0b\x76\x7d\x9d\x55\x3a\x57\x4e\x6b\x53\xe9\x72\xd5\xaf\xfb\xee\xfe\x4e\xd5\x2f\x79\xbf\x3d\xef\xee\x05\x1e\x3d\xa8\xb8\x35\x8f\x6f\xcc\x5b\x96\x39\x63\xaa\x86\xfa\xf7\xe2\xf2\xb7\x65\x91\x10\x11\xc4\x16\x51\xb0\x51\x44\x88\x26\x5a\x68\x23\x42\x07\x5d\xf4\x20\x2b\x22\x31\x24\x90\x84\x82\x01\x0b\x19\xd8\x58\x43\x01\x5b\x28\xa2\x82\x1a\x0e\x11\xe2\x18\x27\x38\x45\x13\xe7\xb8\xc0\x25\x5a\xb8\xc2\x35\x6e\xd0\xc6\x2d\xee\x70\x8f\x08\x0f\x78\xc4\x13\x3a\x78\xc6\x0b\x5e\xd1\xc5\x1b\xde\xf1\x81\x1e\x3e\xf1\x85\x6f\x88\x23\xd2\x8f\x01\x0c\x22\x86\x38\x86\x30\x8c\x04\x46\x30\x8a\x31\x24\x31\x8e\x14\x26\xa0\x30\x89\x29\x4c\xc3\xc0\x2c\x4c\xcc\xc1\xc2\x02\xd2\x58\x44\x06\x59\xe4\xb0\x04\x1b\x0e\x56\x91\x77\x7e\x00\x50\x4b\x03\x04\x2d\x00\x00\x00\x08\x00\x00\x00\x21\x00\xa0\x44\x4b\x5b\xff\xff\xff\xff\xff\xff\xff\xff\x05\x00\x14\x00\x79\x2e\x6e\x70\x79\x01\x00\x10\x00\xa0\x00\x00\x00\x00\x00\x00\x00\x52\x00\x00\x00\x00\x00\x00\x00\x9b\xec\x17\xea\x1b\x10\xc9\xc8\x50\xc6\x50\xad\x9e\x92\x5a\x9c\x5c\xa4\x6e\xa5\xa0\x6e\x93\x69\xa1\xae\xa3\xa0\x9e\x96\x5f\x54\x52\x94\x98\x17\x9f\x5f\x94\x92\x0a\x12\x77\x4b\xcc\x29\x4e\x05\x8a\x17\x67\x24\x16\xa4\x02\xf9\x1a\x46\x3a\x0a\x46\x9a\x3a\x0a\xb5\x0a\x64\x03\x2e\x46\x06\x08\x60\x82\xd2\xcc\x50\x9a\x05\x4a\x03\x00\x50\x4b\x01\x02\x2d\x03\x2d\x00\x00\x00\x08\x00\x00\x00\x21\x00\x50\xbf\xf6\xd5\x11\x01\x00\x00\x10\x02\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x01\x00\x00\x00\x00\x78\x2e\x6e\x70\x79\x50\x4b\x01\x02\x2d\x03\x2d\x00\x00\x00\x08\x00\x00\x00\x21\x00\xa0\x44\x4b\x5b\x52\x00\x00\x00\xa0\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x01\x48\x01\x00\x00\x79\x2e\x6e\x70\x79\x50\x4b\x05\x06\x00\x00\x00\x00\x02\x00\x02\x00\x66\x00\x00\x00\xd1\x01\x00\x00\x00\x00", 589);

std::filesystem::path write_bytes(const std::string& name, const std::string& bytes) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::ofstream(path, std::ios::binary | std::ios::trunc).write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  return path;
}

template <class T>
std::vector<T> values(const pluss::Series& s) {
  return s.contiguous().reshape({static_cast<std::int64_t>(s.numel())}).template host<T>();
}

}  // namespace

int main() {
  using pluss::codem::ScalarType;
  namespace fs = std::filesystem;
  const auto tmp = fs::temp_directory_path();

  // Round trip: every dtype that has a counterpart keeps its values, shape and dtype.
  {
    const auto f64 = pluss::make_series({1.5, -2.25, 3.0, 4.0, 5.5, 6.0}, {2, 3});
    pluss::io::save_npy(f64, tmp / "pluss_a.npy");
    const auto back = pluss::io::load_npy(tmp / "pluss_a.npy");
    require(back.scalar_type() == ScalarType::Float64 && back.numel() == 6, "float64 keeps its dtype and size");
    require(values<double>(back) == values<double>(f64), "float64 values are exact");
    require(back.sizes().size() == 2, "the rank is kept");

    const auto ints = pluss::make_series(std::vector<std::int64_t>{-3, 0, 9, 1LL << 40}, {4}, pluss::Config{}.with_dtype(ScalarType::Int64));
    pluss::io::save_npy(ints, tmp / "pluss_b.npy");
    const auto ints_back = pluss::io::load_npy(tmp / "pluss_b.npy");
    require(ints_back.scalar_type() == ScalarType::Int64 && values<std::int64_t>(ints_back) == values<std::int64_t>(ints), "int64 round trip");

    const auto flags = pluss::make_series(std::vector<bool>{true, false, true, true});
    pluss::io::save_npy(flags, tmp / "pluss_c.npy");
    require(pluss::io::load_npy(tmp / "pluss_c.npy").scalar_type() == ScalarType::Bool, "bool round trip");

    const auto scalar = pluss::make_series(0.25);
    pluss::io::save_npy(scalar, tmp / "pluss_d.npy");
    const auto scalar_back = pluss::io::load_npy(tmp / "pluss_d.npy");
    require(scalar_back.sizes().size() == 0 && values<double>(scalar_back) == std::vector<double>{0.25}, "a scalar keeps its empty shape");

    const auto empty = pluss::make_series(std::vector<double>{}, {0, 3}, pluss::Config{});
    pluss::io::save_npy(empty, tmp / "pluss_e.npy");
    require(pluss::io::load_npy(tmp / "pluss_e.npy").numel() == 0, "an empty array round trips");
  }

  // The header is aligned to 64 bytes, as the format asks.
  {
    pluss::io::save_npy(pluss::make_series({1.0, 2.0}), tmp / "pluss_f.npy");
    std::ifstream in(tmp / "pluss_f.npy", std::ios::binary);
    std::string head(10, '\0');
    in.read(head.data(), 10);
    const std::size_t header = static_cast<unsigned char>(head[8]) | (static_cast<std::size_t>(static_cast<unsigned char>(head[9])) << 8);
    require((10 + header) % 64 == 0, "the preamble is a multiple of 64 bytes");
    require(head.substr(1, 5) == "NUMPY", "the magic string is there");
  }

  // Files from elsewhere: column-major order and big-endian bytes are read correctly.
  {
    const auto column_major = pluss::io::array_detail::series_from_npy(kColumnMajor);
    require(values<std::int32_t>(column_major) == std::vector<std::int32_t>({0, 1, 2, 3, 4, 5}), "column-major data is put in row-major order");
    require(column_major.sizes().size() == 2, "the column-major array keeps its shape");
    const auto big = pluss::io::array_detail::series_from_npy(kBigEndian);
    require(values<double>(big) == std::vector<double>({1.5, -2.0, 1000.25}), "big-endian floats are swapped");
  }

  // An archive: named arrays round trip in order, and a deflated archive is read.
  {
    const auto a = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2});
    const auto b = pluss::make_series(std::vector<std::int64_t>{7, 8, 9}, {3}, pluss::Config{}.with_dtype(ScalarType::Int64));
    pluss::io::save_npz({{"first", a}, {"second", b}}, tmp / "pluss_g.npz");
    const auto arrays = pluss::io::load_npz(tmp / "pluss_g.npz");
    require(arrays.size() == 2 && arrays[0].first == "first" && arrays[1].first == "second", "names and order are kept");
    require(values<double>(arrays[0].second) == values<double>(a) && values<std::int64_t>(arrays[1].second) == values<std::int64_t>(b), "archive members round trip");

    const auto deflated = pluss::io::load_npz(write_bytes("pluss_h.npz", kDeflated));
    require(deflated.size() == 2 && deflated[0].first == "x" && deflated[1].first == "y", "a deflated archive lists its members");
    const auto x = values<float>(deflated[0].second);
    bool x_ok = x.size() == 100;
    for (std::size_t i = 0; x_ok && i < x.size(); ++i) x_ok = x[i] == static_cast<float>(i) * 0.5F;
    require(x_ok, "a deflated float32 member is read exactly");
    require(values<std::int64_t>(deflated[1].second) == std::vector<std::int64_t>({1, 2, 3, 4}), "a deflated int64 member is read exactly");
  }

  // Mistakes are errors that say what is wrong.
  {
    require(throws([&] { (void)pluss::io::load_npy(write_bytes("pluss_i.npy", "not an array file at all")); }), "a file without the magic bytes is an error");
    require(throws([&] { (void)pluss::io::load_npy(tmp / "pluss_no_such_file.npy"); }), "a missing file is an error");
    std::string cut = kBigEndian.substr(0, kBigEndian.size() - 8);
    require(throws([&] { (void)pluss::io::array_detail::series_from_npy(cut); }), "data shorter than the shape is an error");
    std::string damaged = kDeflated;
    damaged[damaged.size() / 2] ^= 0x55;
    require(throws([&] { (void)pluss::io::load_npz(write_bytes("pluss_j.npz", damaged)); }), "a damaged archive is an error");
    require(throws([&] { (void)pluss::io::load_npz(write_bytes("pluss_k.npz", "PK but not a zip")); }), "a file that is not a zip is an error");
    const auto brain = pluss::make_series({1.0, 2.0}, {2}, pluss::Config{}.with_dtype(ScalarType::BFloat16));
    require(throws([&] { pluss::io::save_npy(brain, tmp / "pluss_l.npy"); }), "a dtype with no array-file form is an error");
  }

  if (failures == 0) std::cout << "array file tests passed\n";
  return failures == 0 ? 0 : 1;
}
