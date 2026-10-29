#include "FunctionInternal.h"
#include "OperatorInternal.h"

#include "pluss/error.h"
#include "pluss/io.h"
#include "pluss/subsp/math/calculus_io.h"

#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::subsp::math::detail {
namespace {

constexpr std::uint32_t function_schema = 1;
constexpr std::string_view function_magic = "pluss_math_function";
constexpr std::uint32_t operator_schema = 1;
constexpr std::string_view operator_magic = "pluss_math_operator";

std::uint64_t checksum(std::string_view bytes) noexcept {
  std::uint64_t value = 1469598103934665603ULL;
  for (const unsigned char byte : bytes) {
    value ^= byte;
    value *= 1099511628211ULL;
  }
  return value;
}

class Writer final {
public:
  void u8(std::uint8_t value) { bytes_.push_back(static_cast<char>(value)); }
  void u16(std::uint16_t value) { integer(value); }
  void u32(std::uint32_t value) { integer(value); }
  void u64(std::uint64_t value) { integer(value); }
  void i64(std::int64_t value) { u64(std::bit_cast<std::uint64_t>(value)); }
  void f64(double value) { u64(std::bit_cast<std::uint64_t>(value)); }
  void text(std::string_view value) {
    u64(value.size());
    bytes_.append(value);
  }
  [[nodiscard]] const std::string &bytes() const noexcept { return bytes_; }

private:
  template <class Integer> void integer(Integer value) {
    for (std::size_t i = 0; i < sizeof(Integer); ++i) {
      u8(static_cast<std::uint8_t>(value >> (i * 8U)));
    }
  }
  std::string bytes_{};
};

class Reader final {
public:
  explicit Reader(std::string_view bytes) : bytes_(bytes) {}
  std::uint8_t u8() {
    require(1);
    return static_cast<std::uint8_t>(bytes_[position_++]);
  }
  std::uint16_t u16() { return integer<std::uint16_t>(); }
  std::uint32_t u32() { return integer<std::uint32_t>(); }
  std::uint64_t u64() { return integer<std::uint64_t>(); }
  std::int64_t i64() { return std::bit_cast<std::int64_t>(u64()); }
  double f64() { return std::bit_cast<double>(u64()); }
  std::string text() {
    const std::uint64_t size = u64();
    PLUSS_CHECK(size <= bytes_.size() - position_,
                "math::io: corrupted length-prefixed field");
    std::string result(bytes_.substr(position_, size));
    position_ += static_cast<std::size_t>(size);
    return result;
  }
  void finish() const {
    PLUSS_CHECK(position_ == bytes_.size(),
                "math::io: artifact contains trailing bytes");
  }

private:
  void require(std::size_t size) const {
    PLUSS_CHECK(position_ <= bytes_.size() && size <= bytes_.size() - position_,
                "math::io: artifact is truncated or corrupted");
  }
  template <class Integer> Integer integer() {
    require(sizeof(Integer));
    Integer value = 0;
    for (std::size_t i = 0; i < sizeof(Integer); ++i) {
      value |= static_cast<Integer>(u8()) << (i * 8U);
    }
    return value;
  }
  std::string_view bytes_{};
  std::size_t position_ = 0;
};

std::vector<pluss::msharp::intrusive_ptr<Node>>
postorder(const pluss::msharp::intrusive_ptr<Node> &root) {
  std::vector<pluss::msharp::intrusive_ptr<Node>> result;
  std::vector<std::pair<pluss::msharp::intrusive_ptr<Node>, bool>> pending{
      {root, false}};
  std::unordered_map<const Node *, bool> seen;
  while (!pending.empty()) {
    auto [node, finish] = std::move(pending.back());
    pending.pop_back();
    if (finish) {
      if (!seen.contains(node.get())) {
        seen.emplace(node.get(), true);
        result.push_back(std::move(node));
      }
      continue;
    }
    if (seen.contains(node.get())) {
      continue;
    }
    pending.emplace_back(node, true);
    for (auto it = node->children().rbegin(); it != node->children().rend();
         ++it) {
      pending.emplace_back(*it, false);
    }
  }
  return result;
}

void write_body(Writer &writer, const Function &function) {
  const FunctionImpl &impl = FunctionAccess::get(function);
  writer.u64(impl.trace().variables().size());
  for (std::size_t i = 0; i < impl.trace().variables().size(); ++i) {
    writer.text(impl.trace().variables().get(static_cast<std::int64_t>(i)));
    writer.u8(static_cast<std::uint8_t>(impl.trace().assumptions()[i]));
  }
  writer.u64(impl.bound_variables().size());
  for (const std::int64_t variable : impl.bound_variables()) {
    writer.i64(variable);
  }
  const auto nodes = postorder(impl.root_ptr());
  std::unordered_map<const Node *, std::uint64_t> ids;
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    ids.emplace(nodes[i].get(), i);
  }
  writer.u64(nodes.size());
  for (const auto &node : nodes) {
    PLUSS_CHECK(node->kind() != NodeKind::Opaque,
                "math::io::save: opaque Function callables are not "
                "serializable");
    writer.u8(static_cast<std::uint8_t>(node->kind()));
    writer.u64(node->output().shape.size());
    for (const std::int64_t extent : node->output().shape) {
      writer.i64(extent);
    }
    switch (node->kind()) {
    case NodeKind::Projection:
      writer.i64(node->variable_id());
      break;
    case NodeKind::ScalarConstant:
      writer.f64(node->scalar_value());
      break;
    case NodeKind::SeriesConstant: {
      std::ostringstream stream(std::ios::binary);
      pluss::io::save(node->series_value(), stream);
      writer.text(stream.str());
      break;
    }
    case NodeKind::Primitive:
      writer.u16(static_cast<std::uint16_t>(node->primitive_id()));
      writer.u64(node->children().size());
      for (const auto &child : node->children()) {
        writer.u64(ids.at(child.get()));
      }
      if (carries_attributes(node->primitive_id())) {
        writer.u64(node->attributes().size());
        for (const std::int64_t attribute : node->attributes()) {
          writer.i64(attribute);
        }
      }
      break;
    case NodeKind::Integral:
      writer.u64(ids.at(node->children()[0].get()));
      writer.i64(node->integral_variable_id());
      writer.f64(node->integral_lower());
      writer.u8(node->integral_upper().has_value());
      if (node->integral_upper()) {
        writer.f64(*node->integral_upper());
      }
      break;
    case NodeKind::Opaque:
      PLUSS_FAIL("math::io::save: unreachable opaque Function node");
    }
  }
  writer.u64(ids.at(impl.root_ptr().get()));
}

Function read_body(Reader &reader) {
  const std::uint64_t variable_count = reader.u64();
  PLUSS_CHECK(variable_count <= 100000,
              "math::io::load_function: unreasonable variable count");
  std::vector<std::string> names;
  std::vector<Assumption> assumptions;
  names.reserve(variable_count);
  assumptions.reserve(variable_count);
  for (std::uint64_t i = 0; i < variable_count; ++i) {
    names.push_back(reader.text());
    const auto assumption = static_cast<Assumption>(reader.u8());
    PLUSS_CHECK(assumption <= Assumption::Negative,
                "math::io::load_function: invalid assumption value");
    assumptions.push_back(assumption);
  }
  const std::uint64_t bound_count = reader.u64();
  PLUSS_CHECK(bound_count <= variable_count,
              "math::io::load_function: invalid bound variable count");
  std::vector<std::int64_t> bound_variables;
  bound_variables.reserve(bound_count);
  for (std::uint64_t i = 0; i < bound_count; ++i) {
    const std::int64_t variable = reader.i64();
    PLUSS_CHECK(variable >= 0 &&
                    static_cast<std::uint64_t>(variable) < variable_count,
                "math::io::load_function: invalid bound variable");
    bound_variables.push_back(variable);
  }
  pluss::StackArray<std::string_view> owned(std::move(names));
  auto trace = pluss::msharp::make_intrusive<TraceState>(
      fresh_trace_id(), std::move(owned).lister(), std::move(assumptions));

  const std::uint64_t node_count = reader.u64();
  PLUSS_CHECK(node_count > 0 && node_count <= 1000000,
              "math::io::load_function: invalid node count");
  std::vector<pluss::msharp::intrusive_ptr<Node>> nodes;
  nodes.reserve(node_count);
  for (std::uint64_t i = 0; i < node_count; ++i) {
    const auto kind = static_cast<NodeKind>(reader.u8());
    const std::uint64_t rank = reader.u64();
    PLUSS_CHECK(rank <= 64, "math::io::load_function: invalid output rank");
    OutputMetadata output;
    for (std::uint64_t axis = 0; axis < rank; ++axis) {
      output.shape.push_back(reader.i64());
    }
    switch (kind) {
    case NodeKind::Projection: {
      const std::int64_t variable = reader.i64();
      PLUSS_CHECK(variable >= 0 &&
                      static_cast<std::uint64_t>(variable) < variable_count,
                  "math::io::load_function: invalid projection binding");
      nodes.push_back(Node::projection(trace->id(), variable));
      break;
    }
    case NodeKind::ScalarConstant:
      nodes.push_back(Node::scalar(reader.f64()));
      break;
    case NodeKind::SeriesConstant: {
      const std::string payload = reader.text();
      std::istringstream stream(payload, std::ios::binary);
      nodes.push_back(Node::series(pluss::io::load<pluss::Series>(stream)));
      break;
    }
    case NodeKind::Primitive: {
      const auto id = static_cast<PrimitiveId>(reader.u16());
      (void)primitive(id);
      const std::uint64_t child_count = reader.u64();
      std::vector<pluss::msharp::intrusive_ptr<Node>> children;
      children.reserve(child_count);
      for (std::uint64_t child = 0; child < child_count; ++child) {
        const std::uint64_t index = reader.u64();
        PLUSS_CHECK(index < nodes.size(),
                    "math::io::load_function: invalid primitive child");
        children.push_back(nodes[index]);
      }
      std::vector<std::int64_t> attributes;
      if (carries_attributes(id)) {
        const std::uint64_t count = reader.u64();
        PLUSS_CHECK(count <= 64,
                    "math::io::load_function: invalid tensor attribute count");
        attributes.reserve(count);
        for (std::uint64_t k = 0; k < count; ++k) {
          attributes.push_back(reader.i64());
        }
      }
      nodes.push_back(Node::primitive(id, std::move(children),
                                      std::move(output),
                                      std::move(attributes)));
      break;
    }
    case NodeKind::Integral: {
      const std::uint64_t child = reader.u64();
      PLUSS_CHECK(child < nodes.size(),
                  "math::io::load_function: invalid integral child");
      const std::int64_t variable = reader.i64();
      const double lower = reader.f64();
      const bool has_upper = reader.u8() != 0;
      const std::optional<double> upper =
          has_upper ? std::optional<double>(reader.f64()) : std::nullopt;
      nodes.push_back(Node::integral(nodes[child], variable, lower, upper));
      break;
    }
    case NodeKind::Opaque:
      PLUSS_FAIL("math::io::load_function: opaque nodes are not persistable");
    default:
      PLUSS_FAIL("math::io::load_function: unknown node kind");
    }
  }
  const std::uint64_t root = reader.u64();
  PLUSS_CHECK(root < nodes.size(), "math::io::load_function: invalid root ID");
  return FunctionAccess::make(std::move(trace), nodes[root], true,
                              variable_count > 0,
                              std::move(bound_variables));
}

std::string artifact(std::string_view magic, std::uint32_t schema,
                     std::string_view body) {
  Writer writer;
  writer.text(magic);
  writer.u32(schema);
  writer.u64(checksum(body));
  writer.text(body);
  return writer.bytes();
}

std::string read_artifact(std::istream &input, std::string_view magic,
                          std::uint32_t schema) {
  std::ostringstream buffer(std::ios::binary);
  buffer << input.rdbuf();
  const std::string bytes = buffer.str();
  Reader reader(bytes);
  PLUSS_CHECK(reader.text() == magic, "math::io: invalid artifact magic");
  PLUSS_CHECK(reader.u32() == schema,
              "math::io: unsupported artifact schema version");
  const std::uint64_t expected = reader.u64();
  const std::string body = reader.text();
  reader.finish();
  PLUSS_CHECK(checksum(body) == expected,
              "math::io: checksum mismatch; artifact is corrupted");
  return body;
}

} // namespace

void FunctionAccess::save(const Function &function, std::ostream &output) {
  Writer body;
  write_body(body, function);
  const std::string bytes =
      artifact(function_magic, function_schema, body.bytes());
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  PLUSS_CHECK(output.good(), "math::io::save: failed to write Function");
}

Function FunctionAccess::load(std::istream &input) {
  const std::string body =
      read_artifact(input, function_magic, function_schema);
  Reader reader(body);
  Function function = read_body(reader);
  reader.finish();
  return function;
}

} // namespace pluss::subsp::math::detail

namespace pluss::subsp::math::operators::detail {
namespace {

using math::detail::Reader;
using math::detail::Writer;

void write_descriptor(Writer &writer, const Operator &op) {
  const OperatorConcept &descriptor = OperatorAccess::get(op);
  if (descriptor.kind() == OperatorKind::Composition) {
    writer.u8(1);
    const auto &children = OperatorAccess::chain(op);
    writer.u64(children.size());
    for (const Operator &child : children) {
      Writer nested;
      write_descriptor(nested, child);
      writer.text(nested.bytes());
    }
    return;
  }
  PLUSS_CHECK(op.capabilities().serializable,
              "math::io::save(Operator): operator '" + std::string(op.key()) +
                  "' is not serializable");
  writer.u8(0);
  writer.text(op.key());
  writer.u32(op.version());
  writer.text(OperatorAccess::payload(op));
}

Operator read_descriptor(Reader &reader) {
  const std::uint8_t kind = reader.u8();
  if (kind == 0) {
    const std::string key = reader.text();
    const std::uint32_t version = reader.u32();
    const std::string payload = reader.text();
    return OperatorAccess::restore(key, version, payload);
  }
  PLUSS_CHECK(kind == 1, "math::io: unknown operator descriptor kind");
  const std::uint64_t count = reader.u64();
  PLUSS_CHECK(count > 0 && count <= 100000,
              "math::io: invalid operator composition size");
  std::vector<Operator> children;
  children.reserve(count);
  for (std::uint64_t i = 0; i < count; ++i) {
    const std::string payload = reader.text();
    Reader nested(payload);
    children.push_back(read_descriptor(nested));
    nested.finish();
  }
  Operator result = children.front();
  for (std::size_t i = 1; i < children.size(); ++i) {
    result = compose(children[i], result);
  }
  return result;
}

} // namespace

void OperatorAccess::save(const Operator &op, std::ostream &output) {
  Writer body;
  write_descriptor(body, op);
  const std::string bytes =
      math::detail::artifact(math::detail::operator_magic,
                             math::detail::operator_schema, body.bytes());
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  PLUSS_CHECK(output.good(), "math::io::save: failed to write Operator");
}

Operator OperatorAccess::load(std::istream &input) {
  const std::string body = math::detail::read_artifact(
      input, math::detail::operator_magic, math::detail::operator_schema);
  Reader reader(body);
  Operator op = read_descriptor(reader);
  reader.finish();
  return op;
}

} // namespace pluss::subsp::math::operators::detail

namespace pluss::subsp::math::io {
namespace {

template <class Callback>
void with_output(std::string_view path, Callback callback) {
  PLUSS_CHECK(!path.empty(), "math::io::save: path cannot be empty");
  std::ofstream output(std::filesystem::path(path),
                       std::ios::binary | std::ios::trunc);
  PLUSS_CHECK(output.is_open(), "math::io::save: could not open output path");
  callback(output);
}

template <class Callback>
auto with_input(std::string_view path, Callback callback) {
  PLUSS_CHECK(!path.empty(), "math::io::load: path cannot be empty");
  std::ifstream input(std::filesystem::path(path), std::ios::binary);
  PLUSS_CHECK(input.is_open(), "math::io::load: could not open input path");
  return callback(input);
}

} // namespace

void save(const Function &function, std::ostream &output) {
  detail::FunctionAccess::save(function, output);
}
void save(const Function &function, std::string_view path) {
  with_output(path, [&](std::ostream &output) { save(function, output); });
}
Function load_function(std::istream &input) {
  return detail::FunctionAccess::load(input);
}
Function load_function(std::string_view path) {
  return with_input(path,
                    [](std::istream &input) { return load_function(input); });
}

void save(const operators::Operator &op, std::ostream &output) {
  operators::detail::OperatorAccess::save(op, output);
}
void save(const operators::Operator &op, std::string_view path) {
  with_output(path, [&](std::ostream &output) { save(op, output); });
}
operators::Operator load_operator(std::istream &input) {
  return operators::detail::OperatorAccess::load(input);
}
operators::Operator load_operator(std::string_view path) {
  return with_input(path,
                    [](std::istream &input) { return load_operator(input); });
}

} // namespace pluss::subsp::math::io
