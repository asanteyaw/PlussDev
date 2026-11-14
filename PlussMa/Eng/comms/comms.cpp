#include "pluss/subsp/eng/comms.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/eng/signal.h"
#include "../detail/series_util.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>
#include <vector>

namespace pluss::subsp::eng::comms {
namespace {

using Shape = std::vector<std::int64_t>;
constexpr double kPi = std::numbers::pi_v<double>;

// The real element type that goes with a signal's element type (a complex type gives its component type).
ScalarType real_type_of(ScalarType type) {
  if (type == ScalarType::Complex64) return ScalarType::Float32;
  if (type == ScalarType::Complex128) return ScalarType::Float64;
  return type;
}

std::int64_t gray(std::int64_t value) { return value ^ (value >> 1); }

bool power_of(std::int64_t value, std::int64_t base) {
  if (value < base) return false;
  while (value % base == 0) value /= base;
  return value == 1;
}

std::int64_t log2_exact(std::int64_t value) {
  std::int64_t bits = 0;
  while ((std::int64_t{1} << bits) < value) ++bits;
  return bits;
}

Series complex_points(const std::vector<double> &re, const std::vector<double> &im, ScalarType dtype) {
  PLUSS_CHECK(dtype == ScalarType::Complex128 || dtype == ScalarType::Complex64,
              "eng::comms: constellation dtype must be Complex128 or Complex64");
  const ScalarType real = real_type_of(dtype);
  return pluss::series::complex(pluss::series::make(re).to(real), pluss::series::make(im).to(real), dtype);
}

void check_points(const Constellation &c, const char *who) {
  PLUSS_CHECK(c.points.defined() && c.points.dtype().is_complex() && c.points.dim() == 1 && c.points.numel() == c.order &&
                  c.order == (std::int64_t{1} << c.bits),
              std::string("eng::comms::") + who + ": the constellation is not one made by psk or qam");
}

Shape leading(const Series &value) { return Shape(value.sizes().begin(), value.sizes().end() - 1); }

Shape with(Shape shape, std::int64_t tail) {
  shape.push_back(tail);
  return shape;
}

Series bit_weights(std::int64_t bits, Platform platform) {
  std::vector<double> w(static_cast<std::size_t>(bits));
  for (std::int64_t i = 0; i < bits; ++i) w[static_cast<std::size_t>(i)] = static_cast<double>(std::int64_t{1} << (bits - 1 - i));
  return pluss::series::make(w).to(platform);
}

} // namespace

Constellation psk(std::int64_t order, const ConstellationOptions &options) {
  PLUSS_CHECK(power_of(order, 2), "eng::comms::psk: order must be a power of two (2, 4, 8, ...)");
  PLUSS_CHECK(std::isfinite(options.phase), "eng::comms::psk: phase must be finite");
  std::vector<double> re(static_cast<std::size_t>(order)), im(static_cast<std::size_t>(order));
  for (std::int64_t k = 0; k < order; ++k) {
    const std::int64_t label = options.gray ? gray(k) : k;
    const double angle = 2.0 * kPi * static_cast<double>(k) / static_cast<double>(order) + options.phase;
    re[static_cast<std::size_t>(label)] = std::cos(angle);
    im[static_cast<std::size_t>(label)] = std::sin(angle);
  }
  return {complex_points(re, im, options.dtype), log2_exact(order), order, true};
}

Constellation qam(std::int64_t order, const ConstellationOptions &options) {
  PLUSS_CHECK(power_of(order, 4), "eng::comms::qam: order must be a power of four (4, 16, 64, ...): square QAM only");
  PLUSS_CHECK(std::isfinite(options.phase), "eng::comms::qam: phase must be finite");
  const std::int64_t side = static_cast<std::int64_t>(std::llround(std::sqrt(static_cast<double>(order))));
  const std::int64_t half = log2_exact(side);
  const double scale = 1.0 / std::sqrt(2.0 * (static_cast<double>(side) * side - 1.0) / 3.0);
  auto inverse_gray = [](std::int64_t g) {
    std::int64_t value = 0;
    for (; g != 0; g >>= 1) value ^= g;
    return value;
  };
  std::vector<double> re(static_cast<std::size_t>(order)), im(static_cast<std::size_t>(order));
  for (std::int64_t label = 0; label < order; ++label) {
    const std::int64_t high = label >> half, low = label & (side - 1);
    const std::int64_t m_i = options.gray ? inverse_gray(high) : high, m_q = options.gray ? inverse_gray(low) : low;
    const double x = (2.0 * static_cast<double>(m_i) - static_cast<double>(side - 1)) * scale;
    const double y = (2.0 * static_cast<double>(m_q) - static_cast<double>(side - 1)) * scale;
    re[static_cast<std::size_t>(label)] = x * std::cos(options.phase) - y * std::sin(options.phase);
    im[static_cast<std::size_t>(label)] = x * std::sin(options.phase) + y * std::cos(options.phase);
  }
  return {complex_points(re, im, options.dtype), 2 * half, order, false};
}

Series to_labels(const Series &bits, std::int64_t bits_per_symbol) {
  PLUSS_CHECK(bits_per_symbol >= 1, "eng::comms::to_labels: bits_per_symbol must be at least 1");
  PLUSS_CHECK(bits.defined() && bits.dim() >= 1 && !bits.dtype().is_complex(), "eng::comms::to_labels: bits must be a real [..., N] array");
  PLUSS_CHECK(bits.size(-1) % bits_per_symbol == 0, "eng::comms::to_labels: the number of bits must be a multiple of bits_per_symbol");
  PLUSS_CHECK(bits.ge(0).all().item<bool>() && bits.le(1).all().item<bool>(), "eng::comms::to_labels: bits must be zeros and ones");
  Shape shape = leading(bits);
  shape.push_back(bits.size(-1) / bits_per_symbol);
  shape.push_back(bits_per_symbol);
  return bits.to(ScalarType::Float64).reshape(shape).mul(bit_weights(bits_per_symbol, bits.platform())).sum(-1).round().to(ScalarType::Int64);
}

Series to_bits(const Series &labels, std::int64_t bits_per_symbol) {
  PLUSS_CHECK(bits_per_symbol >= 1, "eng::comms::to_bits: bits_per_symbol must be at least 1");
  PLUSS_CHECK(labels.defined() && labels.dim() >= 1 && !labels.dtype().is_complex(), "eng::comms::to_bits: labels must be a real [..., N] array");
  PLUSS_CHECK(labels.ge(0).all().item<bool>() && labels.lt(static_cast<double>(std::int64_t{1} << bits_per_symbol)).all().item<bool>(),
              "eng::comms::to_bits: labels must lie in [0, 2^bits_per_symbol)");
  const Series expanded = labels.to(ScalarType::Float64).unsqueeze(-1);
  const Series divisor = bit_weights(bits_per_symbol, labels.platform());
  const Series digit = expanded.div(divisor).floor();
  Shape shape = leading(labels);
  shape.push_back(labels.size(-1) * bits_per_symbol);
  return digit.sub(digit.div(2.0).floor().mul(2.0)).to(ScalarType::Int64).reshape(shape);
}

Series symbols(const Constellation &constellation, const Series &labels) {
  check_points(constellation, "symbols");
  PLUSS_CHECK(labels.defined() && !labels.dtype().is_complex() && labels.dim() >= 1,
              "eng::comms::symbols: labels must be a real integer array");
  PLUSS_CHECK(labels.ge(0).all().item<bool>() && labels.lt(static_cast<double>(constellation.order)).all().item<bool>(),
              "eng::comms::symbols: labels must lie in [0, order)");
  const Shape shape(labels.sizes().begin(), labels.sizes().end());
  return constellation.points.to(labels.platform()).index_select(0, labels.to(ScalarType::Int64).reshape(Shape{labels.numel()})).reshape(shape);
}

Series modulate(const Constellation &constellation, const Series &bits) {
  check_points(constellation, "modulate");
  return symbols(constellation, to_labels(bits, constellation.bits));
}

Series detect(const Constellation &constellation, const Series &received) {
  check_points(constellation, "detect");
  PLUSS_CHECK(received.defined() && received.dim() >= 1, "eng::comms::detect: received must be a [..., N] array");
  const Series points = constellation.points.to(received.platform());
  const Series gap = received.to(points.scalar_type()).unsqueeze(-1).sub(points).abs();
  return gap.argmin(-1, false).to(ScalarType::Int64);
}

Series demod(const Constellation &constellation, const Series &received) {
  return to_bits(detect(constellation, received), constellation.bits);
}

Series llr(const Constellation &constellation, const Series &received, double noise_variance) {
  check_points(constellation, "llr");
  PLUSS_CHECK(std::isfinite(noise_variance) && noise_variance > 0.0, "eng::comms::llr: noise_variance must be positive");
  PLUSS_CHECK(received.defined() && received.dim() >= 1, "eng::comms::llr: received must be a [..., N] array");
  const Series points = constellation.points.to(received.platform());
  const Series gap = received.to(points.scalar_type()).unsqueeze(-1).sub(points).abs();
  const Series distance = gap.mul(gap);   // [..., N, M]
  std::vector<Series> columns;
  for (std::int64_t b = 0; b < constellation.bits; ++b) {
    std::vector<double> bit_is_one(static_cast<std::size_t>(constellation.order)), bit_is_zero(static_cast<std::size_t>(constellation.order));
    for (std::int64_t label = 0; label < constellation.order; ++label) {
      const bool one = ((label >> (constellation.bits - 1 - b)) & 1) != 0;
      bit_is_one[static_cast<std::size_t>(label)] = one ? 1.0e30 : 0.0;
      bit_is_zero[static_cast<std::size_t>(label)] = one ? 0.0 : 1.0e30;
    }
    const Series keep_zero = pluss::series::make(bit_is_one).to(distance.scalar_type()).to(received.platform());
    const Series keep_one = pluss::series::make(bit_is_zero).to(distance.scalar_type()).to(received.platform());
    const Series d0 = distance.add(keep_zero).amin(-1, false);
    const Series d1 = distance.add(keep_one).amin(-1, false);
    columns.push_back(d1.sub(d0).div(noise_variance).unsqueeze(-1));
  }
  Series stacked = columns[0].unite(std::vector<Series>(columns.begin() + 1, columns.end()), -1);   // [..., N, bits]
  Shape shape = leading(received);
  shape.push_back(received.size(-1) * constellation.bits);
  return stacked.reshape(shape);
}

Series rrc(double rolloff, std::int64_t samples_per_symbol, std::int64_t span, ScalarType dtype) {
  PLUSS_CHECK(std::isfinite(rolloff) && rolloff >= 0.0 && rolloff <= 1.0, "eng::comms::rrc: rolloff must lie in [0, 1]");
  PLUSS_CHECK(samples_per_symbol >= 2, "eng::comms::rrc: samples_per_symbol must be at least 2");
  PLUSS_CHECK(span >= 2, "eng::comms::rrc: span must be at least 2 symbols");
  PLUSS_CHECK(dtype == ScalarType::Float64 || dtype == ScalarType::Float32, "eng::comms::rrc: dtype must be Float64 or Float32");
  const std::int64_t length = span * samples_per_symbol + 1;
  std::vector<double> taps(static_cast<std::size_t>(length));
  const double beta = rolloff;
  for (std::int64_t k = 0; k < length; ++k) {
    const double t = static_cast<double>(k - span * samples_per_symbol / 2) / static_cast<double>(samples_per_symbol);
    double value;
    if (std::abs(t) < 1e-12) {
      value = 1.0 - beta + 4.0 * beta / kPi;
    } else if (beta > 0.0 && std::abs(std::abs(4.0 * beta * t) - 1.0) < 1e-9) {
      value = beta / std::sqrt(2.0) * ((1.0 + 2.0 / kPi) * std::sin(kPi / (4.0 * beta)) + (1.0 - 2.0 / kPi) * std::cos(kPi / (4.0 * beta)));
    } else {
      const double numerator = std::sin(kPi * t * (1.0 - beta)) + 4.0 * beta * t * std::cos(kPi * t * (1.0 + beta));
      const double denominator = kPi * t * (1.0 - (4.0 * beta * t) * (4.0 * beta * t));
      value = numerator / denominator;
    }
    taps[static_cast<std::size_t>(k)] = value;
  }
  double energy = 0.0;
  for (double v : taps) energy += v * v;
  const double norm = 1.0 / std::sqrt(energy);
  for (double &v : taps) v *= norm;
  return pluss::series::make(taps).to(dtype);
}

Series shape(const Series &symbols, const Series &taps, std::int64_t samples_per_symbol) {
  PLUSS_CHECK(symbols.defined() && symbols.dim() >= 1, "eng::comms::shape: symbols must be a [..., N] array");
  PLUSS_CHECK(taps.defined() && taps.dim() == 1 && taps.numel() >= 1 && !taps.dtype().is_complex(),
              "eng::comms::shape: taps must be a real vector");
  PLUSS_CHECK(samples_per_symbol >= 1, "eng::comms::shape: samples_per_symbol must be at least 1");
  const std::int64_t count = symbols.size(-1);
  Shape up_shape = leading(symbols);
  up_shape.push_back(count);
  up_shape.push_back(samples_per_symbol);
  const Series zeros = pluss::series::zeros(up_shape, Config{}.with_dtype(symbols.scalar_type()).with_platform(symbols.platform()));
  Series first = symbols.unsqueeze(-1);
  Series spread = first.unite({zeros.slice(-1, 1, samples_per_symbol)}, -1);   // [..., N, sps]
  Shape flat = leading(symbols);
  flat.push_back(count * samples_per_symbol);
  const Series upsampled = spread.reshape(flat).slice(-1, 0, (count - 1) * samples_per_symbol + 1);
  return signal::conv(upsampled, taps.to(real_type_of(symbols.scalar_type())));
}

Series matched(const Series &waveform, const Series &taps) {
  PLUSS_CHECK(waveform.defined() && waveform.dim() >= 1, "eng::comms::matched: waveform must be a [..., samples] array");
  PLUSS_CHECK(taps.defined() && taps.dim() == 1 && taps.numel() >= 1 && !taps.dtype().is_complex(),
              "eng::comms::matched: taps must be a real vector");
  const ScalarType real = real_type_of(waveform.scalar_type());
  std::vector<double> reversed = taps.to(Platform::CPU, ScalarType::Float64).items<double>();
  std::reverse(reversed.begin(), reversed.end());
  return signal::conv(waveform, pluss::series::make(reversed).to(real).to(waveform.platform()));
}

Series match(const Series &waveform, const Series &taps, std::int64_t samples_per_symbol, std::int64_t count) {
  PLUSS_CHECK(waveform.defined() && waveform.dim() >= 1, "eng::comms::match: waveform must be a [..., samples] array");
  PLUSS_CHECK(taps.defined() && taps.dim() == 1 && taps.numel() >= 1 && !taps.dtype().is_complex(),
              "eng::comms::match: taps must be a real vector");
  PLUSS_CHECK(samples_per_symbol >= 1 && count >= 1, "eng::comms::match: samples_per_symbol and count must be at least 1");
  const std::int64_t delay = taps.numel() - 1;
  PLUSS_CHECK(waveform.size(-1) >= delay + (count - 1) * samples_per_symbol + 1,
              "eng::comms::match: the waveform is too short for the requested number of symbols");
  const Series filtered = matched(waveform, taps);
  std::vector<Series> picked;
  for (std::int64_t k = 0; k < count; ++k) picked.push_back(filtered.select(-1, delay + k * samples_per_symbol).unsqueeze(-1));
  return picked[0].unite(std::vector<Series>(picked.begin() + 1, picked.end()), -1);
}

Series noise(const Series &signal, double noise_variance, std::uint64_t seed) {
  PLUSS_CHECK(signal.defined(), "eng::comms::noise: signal must be defined");
  PLUSS_CHECK(std::isfinite(noise_variance) && noise_variance >= 0.0, "eng::comms::noise: noise_variance must be non-negative");
  const Shape shape(signal.sizes().begin(), signal.sizes().end());
  if (signal.dtype().is_complex()) {
    const ScalarType real = real_type_of(signal.scalar_type());
    const double sigma = std::sqrt(noise_variance / 2.0);
    const Series re = pluss::series::randn(shape, Config{}.with_dtype(real), seed).mul(sigma);
    const Series im = pluss::series::randn(shape, Config{}.with_dtype(real), seed + 1).mul(sigma);
    return signal.add(pluss::series::complex(re, im, signal.scalar_type()).to(signal.platform()));
  }
  PLUSS_CHECK(signal.dtype().is_floating(), "eng::comms::noise: signal must be floating or complex");
  return signal.add(pluss::series::randn(shape, Config{}.with_dtype(signal.scalar_type()), seed).mul(std::sqrt(noise_variance)).to(signal.platform()));
}

double n0(double ebn0_db, const Constellation &constellation) {
  PLUSS_CHECK(std::isfinite(ebn0_db), "eng::comms::n0: ebn0_db must be finite");
  PLUSS_CHECK(constellation.bits >= 1, "eng::comms::n0: the constellation is not one made by psk or qam");
  return 1.0 / (static_cast<double>(constellation.bits) * std::pow(10.0, ebn0_db / 10.0));
}

namespace {

Series mismatch(const Series &a, const Series &b, const char *who) {
  PLUSS_CHECK(a.defined() && b.defined() && a.dim() >= 1 && a.sizes() == b.sizes(),
              std::string("eng::comms::") + who + ": both arrays must have the same shape [..., N]");
  PLUSS_CHECK(a.size(-1) >= 1, std::string("eng::comms::") + who + ": the last axis must not be empty");
  return a.to(ScalarType::Float64).ne(b.to(ScalarType::Float64)).to(ScalarType::Float64).mean(-1);
}

} // namespace

Series ber(const Series &sent, const Series &received) { return mismatch(sent, received, "ber"); }

Series ser(const Series &sent, const Series &received) { return mismatch(sent, received, "ser"); }

Series evm(const Series &received, const Series &reference) {
  PLUSS_CHECK(received.defined() && reference.defined() && received.dim() >= 1 && received.sizes() == reference.sizes(),
              "eng::comms::evm: received and reference must have the same shape [..., N]");
  const Series error = received.sub(reference).abs();
  const Series power = reference.abs();
  return error.mul(error).mean(-1).div(power.mul(power).mean(-1)).sqrt();
}

Series theory(const Constellation &constellation, const Series &ebn0_db) {
  PLUSS_CHECK(constellation.bits >= 1, "eng::comms::theory: the constellation is not one made by psk or qam");
  PLUSS_CHECK(ebn0_db.defined() && ebn0_db.dtype().is_floating(), "eng::comms::theory: ebn0_db must be a real floating array");
  const Series ratio = ebn0_db.to(ScalarType::Float64).div(10.0).mul(std::log(10.0)).exp();
  const double k = static_cast<double>(constellation.bits);
  const double m = static_cast<double>(constellation.order);
  auto q = [](const Series &x) { return x.div(std::sqrt(2.0)).erfc().mul(0.5); };
  if (constellation.order <= 4 && constellation.phase_shift_keying) return q(ratio.mul(2.0).sqrt());
  if (constellation.phase_shift_keying) return q(ratio.mul(2.0 * k).sqrt().mul(std::sin(kPi / m))).mul(2.0 / k);
  return q(ratio.mul(3.0 * k / (m - 1.0)).sqrt()).mul(4.0 / k * (1.0 - 1.0 / std::sqrt(m)));
}

Series to_tones(const Series &symbols, std::int64_t fft_size, std::int64_t cyclic_prefix) {
  PLUSS_CHECK(fft_size >= 2 && cyclic_prefix >= 0 && cyclic_prefix < fft_size,
              "eng::comms::to_tones: fft_size must be at least 2 and cyclic_prefix in [0, fft_size)");
  PLUSS_CHECK(symbols.defined() && symbols.dim() >= 1 && symbols.dtype().is_complex() && symbols.size(-1) % fft_size == 0 && symbols.size(-1) > 0,
              "eng::comms::to_tones: symbols must be a complex [..., F * fft_size] array");
  Shape shape = leading(symbols);
  shape.push_back(symbols.size(-1) / fft_size);
  shape.push_back(fft_size);
  const Series blocks = symbols.reshape(shape).ifft(fft_size, -1, "ortho");
  const Series prefixed = cyclic_prefix == 0 ? blocks : blocks.slice(-1, fft_size - cyclic_prefix, fft_size).unite({blocks}, -1);
  Shape flat = leading(symbols);
  flat.push_back(symbols.size(-1) / fft_size * (fft_size + cyclic_prefix));
  return prefixed.reshape(flat);
}

Series from_tones(const Series &signal, std::int64_t fft_size, std::int64_t cyclic_prefix) {
  PLUSS_CHECK(fft_size >= 2 && cyclic_prefix >= 0 && cyclic_prefix < fft_size,
              "eng::comms::from_tones: fft_size must be at least 2 and cyclic_prefix in [0, fft_size)");
  const std::int64_t block = fft_size + cyclic_prefix;
  PLUSS_CHECK(signal.defined() && signal.dim() >= 1 && signal.dtype().is_complex() && signal.size(-1) % block == 0 && signal.size(-1) > 0,
              "eng::comms::from_tones: signal must be a complex [..., F * (fft_size + cyclic_prefix)] array");
  Shape shape = leading(signal);
  shape.push_back(signal.size(-1) / block);
  shape.push_back(block);
  const Series blocks = signal.reshape(shape).slice(-1, cyclic_prefix, block).fft(fft_size, -1, "ortho");
  Shape flat = leading(signal);
  flat.push_back(signal.size(-1) / block * fft_size);
  return blocks.reshape(flat);
}


namespace {

// Trellis tables of a feed-forward code: state = the previous K - 1 inputs, lag 1 in bit 0.
struct Trellis {
  std::int64_t states{}, memory{}, outputs{};
  std::vector<std::int64_t> output_bits;   // [state][input][output] flattened
  std::vector<std::int64_t> next_state;    // [state][input]
  std::vector<std::int64_t> pred_state;    // [next state][slot]: the two states that lead to it
  std::vector<std::int64_t> pred_branch;   // [next state][slot]: and the branch (state * 2 + input) taken
  std::vector<std::int64_t> terminate;     // [state]: the input that drives a recursive encoder to zero
};

// Fills the predecessor tables of a trellis from its next-state table (every state has two incoming branches).
void link_trellis(Trellis &t) {
  t.pred_state.assign(static_cast<std::size_t>(t.states * 2), 0);
  t.pred_branch.assign(static_cast<std::size_t>(t.states * 2), 0);
  std::vector<std::int64_t> filled(static_cast<std::size_t>(t.states), 0);
  for (std::int64_t s = 0; s < t.states; ++s)
    for (std::int64_t u = 0; u < 2; ++u) {
      const std::int64_t next = t.next_state[static_cast<std::size_t>(s * 2 + u)];
      const std::int64_t slot = filled[static_cast<std::size_t>(next)]++;
      t.pred_state[static_cast<std::size_t>(next * 2 + slot)] = s;
      t.pred_branch[static_cast<std::size_t>(next * 2 + slot)] = s * 2 + u;
    }
}

Series index_series(const std::vector<std::int64_t> &values, Platform platform) {
  return pluss::series::make(std::vector<double>(values.begin(), values.end())).to(ScalarType::Int64).to(platform);
}

// Zeros and ones are bits, not log-likelihood ratios: a decoder given them decodes towards zero and still reports success. A
// vector made only of 0 and 1 that contains a zero cannot be hard decisions (+1 / -1) or ratios, so it is refused.
void reject_bits_as_soft(const Series &soft, const char *who) {
  const Series product = soft.mul(soft.sub(1.0)).abs();
  PLUSS_CHECK(!(detail::max_abs(product) == 0.0 && soft.eq(0.0).any().item<bool>()),
              std::string("eng::comms::") + who +
                  ": every soft value is exactly 0 or 1, which are bits, not log-likelihood ratios; pass the output of llr "
                  "(positive favours zero) or +1 / -1 for hard decisions");
}

std::int64_t parity_of(std::int64_t v) {
  std::int64_t parity = 0;
  for (; v; v >>= 1) parity ^= v & 1;
  return parity;
}

Trellis make_trellis(const ConvolutionalCode &code, const char *who) {
  const std::int64_t k = code.constraint_length;
  PLUSS_CHECK(k >= 2 && k <= 12, std::string("eng::comms::") + who + ": constraint_length must lie in [2, 12]");
  PLUSS_CHECK(code.generators.size() >= 2, std::string("eng::comms::") + who + ": a code needs at least two generators");
  for (std::int64_t g : code.generators)
    PLUSS_CHECK(g > 0 && g < (std::int64_t{1} << k), std::string("eng::comms::") + who + ": every generator must be non-zero and fit in constraint_length bits");
  Trellis t;
  t.memory = k - 1;
  t.states = std::int64_t{1} << t.memory;
  t.outputs = static_cast<std::int64_t>(code.generators.size());
  for (std::int64_t s = 0; s < t.states; ++s)
    for (std::int64_t u = 0; u < 2; ++u) {
      const std::int64_t reg = (s << 1) | u;   // bit k of the register is the input k steps ago
      for (std::int64_t g : code.generators) {
        std::int64_t mask = 0;   // generator bit (K - 1 - lag) multiplies the input `lag` steps ago
        for (std::int64_t lag = 0; lag < k; ++lag) mask |= ((g >> (k - 1 - lag)) & 1) << lag;
        t.output_bits.push_back(parity_of(reg & mask));
      }
      t.next_state.push_back(reg & (t.states - 1));
    }
  link_trellis(t);
  return t;
}

Series gather_last(const Series &value, const Series &index) { return value.gather(-1, index); }

} // namespace

Series encode(const ConvolutionalCode &code, const Series &bits) {
  const Trellis t = make_trellis(code, "encode");
  PLUSS_CHECK(bits.defined() && bits.dim() >= 1 && bits.size(-1) >= 1 && !bits.dtype().is_complex(),
              "eng::comms::encode: bits must be a real [..., N] array with N >= 1");
  PLUSS_CHECK(bits.ge(0).all().item<bool>() && bits.le(1).all().item<bool>(), "eng::comms::encode: bits must be zeros and ones");
  const std::int64_t k = code.constraint_length;
  const Series input = bits.to(ScalarType::Float64);
  std::vector<Series> streams;
  for (std::int64_t g : code.generators) {
    std::vector<double> kernel(static_cast<std::size_t>(k));
    for (std::int64_t lag = 0; lag < k; ++lag) kernel[static_cast<std::size_t>(lag)] = static_cast<double>((g >> (k - 1 - lag)) & 1);
    const Series coded = signal::conv(input, pluss::series::make(kernel).to(bits.platform()), -1, signal::ConvMode::Full);   // N + K - 1 samples: the tail
    streams.push_back(coded.sub(coded.div(2.0).floor().mul(2.0)).round().unsqueeze(-1));
  }
  Series stacked = streams[0].unite(std::vector<Series>(streams.begin() + 1, streams.end()), -1);   // [..., N + K - 1, n]
  Shape shape = leading(bits);
  shape.push_back((bits.size(-1) + k - 1) * t.outputs);
  return stacked.reshape(shape).to(ScalarType::Int64);
}

Series decode(const ConvolutionalCode &code, const Series &soft) {
  const Trellis t = make_trellis(code, "decode");
  PLUSS_CHECK(soft.defined() && soft.dim() >= 1 && soft.dtype().is_floating() && soft.size(-1) % t.outputs == 0 &&
                  soft.size(-1) / t.outputs > t.memory,
              "eng::comms::decode: soft must be a real [..., (N + constraint_length - 1) * generators] array");
  reject_bits_as_soft(soft, "decode");
  const std::int64_t steps = soft.size(-1) / t.outputs;
  const std::int64_t info = steps - t.memory;
  const Platform platform = soft.platform();
  const ScalarType dtype = soft.scalar_type();
  Shape lead = leading(soft);
  // branch signs [2 S, n]: +1 for a coded 0 and -1 for a coded 1, so the metric to maximise is the correlation with the LLRs
  std::vector<double> sign(static_cast<std::size_t>(t.states * 2 * t.outputs));
  for (std::size_t i = 0; i < sign.size(); ++i) sign[i] = 1.0 - 2.0 * static_cast<double>(t.output_bits[i]);
  const Series signs = pluss::series::make(sign, {t.states * 2, t.outputs}).to(dtype).to(platform);
  const Series state_index = index_series(t.pred_state, platform);
  const Series branch_index = index_series(t.pred_branch, platform);
  // path metrics: start in state 0
  std::vector<double> start(static_cast<std::size_t>(t.states), -1.0e30);
  start[0] = 0.0;
  Series metric = pluss::series::zeros(with(lead, t.states), Config{}.with_dtype(dtype).with_platform(platform)).add(pluss::series::make(start).to(dtype).to(platform));
  std::vector<Series> decisions;
  Shape pair_shape = lead;
  pair_shape.push_back(t.states);
  pair_shape.push_back(2);
  for (std::int64_t step = 0; step < steps; ++step) {
    const Series observed = soft.slice(-1, step * t.outputs, (step + 1) * t.outputs);
    const Series branch = observed.matmul(signs.mT());                                           // [..., 2S]
    const Series candidates = metric.index_select(-1, state_index).add(branch.index_select(-1, branch_index)).reshape(pair_shape);
    decisions.push_back(candidates.argmax(-1, false));
    metric = candidates.amax(-1, false);
  }
  // traceback from the zero state (the code is terminated)
  Series state = pluss::series::zeros(lead, Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
  std::vector<Series> bits(static_cast<std::size_t>(steps));
  for (std::int64_t step = steps - 1; step >= 0; --step) {
    const Series branch = gather_last(decisions[static_cast<std::size_t>(step)], state.unsqueeze(-1)).squeeze(-1);
    bits[static_cast<std::size_t>(step)] = state.sub(state.div(2.0).floor().mul(2.0)).to(ScalarType::Int64);   // the input is the low bit of the state
    state = state.div(2.0).floor().to(ScalarType::Int64).add(branch.mul(std::int64_t{1} << (t.memory - 1)));
  }
  std::vector<Series> kept;
  for (std::int64_t step = 0; step < info; ++step) kept.push_back(bits[static_cast<std::size_t>(step)].unsqueeze(-1));
  return kept[0].unite(std::vector<Series>(kept.begin() + 1, kept.end()), -1);
}

namespace {

// exp(j 2 pi x) of a real array.
Series unit_phasor(const Series &cycles) {
  const Series angle = cycles.mul(2.0 * kPi);
  const ScalarType complex = angle.scalar_type() == ScalarType::Float32 ? ScalarType::Complex64 : ScalarType::Complex128;
  return pluss::series::complex(angle.cos(), angle.sin(), complex);
}

Series real_vector(const std::vector<double> &values, ScalarType real, Platform platform) {
  return pluss::series::make(values).to(real).to(platform);
}

} // namespace

Series timing(const Series &waveform, std::int64_t samples_per_symbol) {
  PLUSS_CHECK(samples_per_symbol >= 4, "eng::comms::timing: samples_per_symbol must be at least 4");
  PLUSS_CHECK(waveform.defined() && waveform.dim() >= 1 && waveform.size(-1) >= 4 * samples_per_symbol,
              "eng::comms::timing: waveform must hold at least four symbols per frame");
  const std::int64_t length = waveform.size(-1);
  const Series power = waveform.abs().mul(waveform.abs());
  std::vector<double> angle(static_cast<std::size_t>(length));
  for (std::int64_t n = 0; n < length; ++n) angle[static_cast<std::size_t>(n)] = -static_cast<double>(n) / static_cast<double>(samples_per_symbol);
  const Series phasor = unit_phasor(real_vector(angle, power.scalar_type(), power.platform()));
  const Series tone = power.to(phasor.scalar_type()).mul(phasor).sum(-1);
  const Series offset = tone.imag().atan2(tone.real()).mul(-static_cast<double>(samples_per_symbol) / (2.0 * kPi));
  return offset;
}

Series delay(const Series &signal, const Series &samples) {
  PLUSS_CHECK(signal.defined() && signal.dim() >= 1 && signal.size(-1) >= 2 && (signal.dtype().is_complex() || signal.dtype().is_floating()),
              "eng::comms::delay: signal must be a real or complex [..., L] array");
  PLUSS_CHECK(samples.defined() && samples.dtype().is_floating() && samples.isfinite().all().item<bool>(),
              "eng::comms::delay: samples must be finite real values");
  const std::int64_t length = signal.size(-1);
  std::vector<double> freq(static_cast<std::size_t>(length));
  for (std::int64_t k = 0; k < length; ++k) freq[static_cast<std::size_t>(k)] = static_cast<double>(k <= length / 2 ? k : k - length) / static_cast<double>(length);
  const ScalarType real = real_type_of(signal.scalar_type());
  const Series f = real_vector(freq, real, signal.platform());
  Series shift = samples.to(real).to(signal.platform());
  shift = shift.dim() == 0 ? shift : shift.unsqueeze(-1);
  const Series ramp = unit_phasor(f.mul(shift).neg());
  const Series spectrum = signal.to(ramp.scalar_type()).fft(length, -1, "backward").mul(ramp);
  const Series moved = spectrum.ifft(length, -1, "backward");
  return signal.dtype().is_complex() ? moved : moved.real();
}

Series derot(const Series &symbols, const Series &cycles_per_symbol, const Series &phase) {
  PLUSS_CHECK(symbols.defined() && symbols.dim() >= 1 && symbols.dtype().is_complex(), "eng::comms::derot: symbols must be a complex [..., N] array");
  const std::int64_t count = symbols.size(-1);
  const ScalarType real = real_type_of(symbols.scalar_type());
  std::vector<double> index(static_cast<std::size_t>(count));
  for (std::int64_t n = 0; n < count; ++n) index[static_cast<std::size_t>(n)] = static_cast<double>(n);
  const Series n = real_vector(index, real, symbols.platform());
  auto lane = [&](const Series &v) { return v.dim() == 0 ? v.to(real) : v.to(real).unsqueeze(-1); };
  const Series cycles = n.mul(lane(cycles_per_symbol)).add(lane(phase).div(2.0 * kPi));
  return symbols.mul(unit_phasor(cycles.neg()));
}

Series cfo(const Series &symbols, std::int64_t order) {
  PLUSS_CHECK(order == 2 || order == 4 || order == 8, "eng::comms::cfo: order must be 2, 4 or 8");
  PLUSS_CHECK(symbols.defined() && symbols.dim() >= 1 && symbols.dtype().is_complex() && symbols.size(-1) >= 16,
              "eng::comms::cfo: symbols must be a complex [..., N] array with N >= 16");
  Series power = symbols;
  for (std::int64_t k = 1; k < order; ++k) power = power.mul(symbols);
  const std::int64_t count = symbols.size(-1);
  std::int64_t size = 1;
  while (size < 8 * count) size <<= 1;
  const Series spectrum = power.fft(size, -1, "backward").abs();
  const Series peak = spectrum.argmax(-1, false);
  const Series left = gather_last(spectrum, peak.sub(1).add(peak.eq(0).to(ScalarType::Int64).mul(size)).unsqueeze(-1)).squeeze(-1);
  const Series centre = gather_last(spectrum, peak.unsqueeze(-1)).squeeze(-1);
  const Series right = gather_last(spectrum, peak.add(1).sub(peak.eq(size - 1).to(ScalarType::Int64).mul(size)).unsqueeze(-1)).squeeze(-1);
  const Series curvature = left.sub(centre.mul(2.0)).add(right);
  const Series fraction = left.sub(right).div(curvature.mul(2.0).add(1e-300));
  Series bin = peak.to(spectrum.scalar_type()).add(fraction);
  bin = bin.sub(bin.gt(static_cast<double>(size) / 2.0).to(bin.scalar_type()).mul(static_cast<double>(size)));
  return bin.div(static_cast<double>(size) * static_cast<double>(order));
}

CarrierResult pll(const Series &symbols, std::int64_t order, double loop_bandwidth) {
  PLUSS_CHECK(order == 2 || order == 4 || order == 8, "eng::comms::pll: order must be 2, 4 or 8");
  PLUSS_CHECK(symbols.defined() && symbols.dim() >= 1 && symbols.dtype().is_complex() && symbols.size(-1) >= 2,
              "eng::comms::pll: symbols must be a complex [..., N] array");
  PLUSS_CHECK(std::isfinite(loop_bandwidth) && loop_bandwidth > 0.0 && loop_bandwidth < 0.5,
              "eng::comms::pll: loop_bandwidth must lie in (0, 0.5)");
  const double zeta = 1.0 / std::sqrt(2.0);
  const double theta = loop_bandwidth / (zeta + 1.0 / (4.0 * zeta));
  const double d = 1.0 + 2.0 * zeta * theta + theta * theta;
  const double alpha = 4.0 * zeta * theta / d, beta = 4.0 * theta * theta / d;
  const std::int64_t count = symbols.size(-1);
  const ScalarType real = real_type_of(symbols.scalar_type());
  const Config cfg = Config{}.with_dtype(real).with_platform(symbols.platform());
  Series phase = pluss::series::zeros(leading(symbols), cfg), frequency = phase;
  std::vector<Series> corrected, phases;
  for (std::int64_t n = 0; n < count; ++n) {
    const Series rotor = unit_phasor(phase.div(2.0 * kPi).neg());
    const Series y = symbols.select(-1, n).mul(rotor);
    Series power = y;
    for (std::int64_t k = 1; k < order; ++k) power = power.mul(y);
    const Series error = power.imag().atan2(power.real()).div(static_cast<double>(order));   // phase error in (-pi / M, pi / M]
    corrected.push_back(y.unsqueeze(-1));
    phases.push_back(phase.unsqueeze(-1));
    frequency = frequency.add(error.mul(beta));
    phase = phase.add(frequency).add(error.mul(alpha));
  }
  return {corrected[0].unite(std::vector<Series>(corrected.begin() + 1, corrected.end()), -1),
          phases[0].unite(std::vector<Series>(phases.begin() + 1, phases.end()), -1)};
}


// ---- LDPC ----------------------------------------------------------------------------------------------------------

LdpcCode gallager(std::int64_t length, std::int64_t column_weight, std::int64_t row_weight, std::uint64_t seed) {
  PLUSS_CHECK(column_weight >= 2 && row_weight > column_weight && length >= 2 * row_weight && length % row_weight == 0 && length <= 20000,
              "eng::comms::gallager: need column_weight >= 2, row_weight > column_weight, and length a multiple of row_weight, at least twice it, and at most 20000");
  const std::int64_t n = length, block = n / row_weight, m = block * column_weight;
  std::vector<std::vector<std::uint8_t>> h(static_cast<std::size_t>(m), std::vector<std::uint8_t>(static_cast<std::size_t>(n), 0));
  for (std::int64_t b = 0; b < column_weight; ++b) {
    std::vector<std::int64_t> order(static_cast<std::size_t>(n));
    if (b == 0) {
      for (std::int64_t i = 0; i < n; ++i) order[static_cast<std::size_t>(i)] = i;
    } else {
      order = pluss::series::randn({n}, ScalarType::Float64, seed + static_cast<std::uint64_t>(b)).argsort().items<std::int64_t>();
    }
    for (std::int64_t r = 0; r < block; ++r)
      for (std::int64_t j = 0; j < row_weight; ++j) h[static_cast<std::size_t>(b * block + r)][static_cast<std::size_t>(order[static_cast<std::size_t>(r * row_weight + j)])] = 1;
  }
  // Gauss-Jordan elimination over GF(2): the pivot columns hold the parity bits, the others the information
  auto reduced = h;
  std::vector<std::int64_t> pivot_column;
  std::int64_t rank = 0;
  for (std::int64_t col = 0; col < n && rank < m; ++col) {
    std::int64_t row = rank;
    while (row < m && !reduced[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)]) ++row;
    if (row == m) continue;
    std::swap(reduced[static_cast<std::size_t>(rank)], reduced[static_cast<std::size_t>(row)]);
    for (std::int64_t other = 0; other < m; ++other)
      if (other != rank && reduced[static_cast<std::size_t>(other)][static_cast<std::size_t>(col)])
        for (std::int64_t c = 0; c < n; ++c) reduced[static_cast<std::size_t>(other)][static_cast<std::size_t>(c)] ^= reduced[static_cast<std::size_t>(rank)][static_cast<std::size_t>(c)];
    pivot_column.push_back(col);
    ++rank;
  }
  std::vector<bool> is_pivot(static_cast<std::size_t>(n), false);
  for (std::int64_t c : pivot_column) is_pivot[static_cast<std::size_t>(c)] = true;
  LdpcCode code;
  code.length = n;
  for (std::int64_t c = 0; c < n; ++c)
    if (!is_pivot[static_cast<std::size_t>(c)]) code.info_positions.push_back(c);
  code.dimension = static_cast<std::int64_t>(code.info_positions.size());
  std::vector<double> generator(static_cast<std::size_t>(code.dimension * n), 0.0), parity_check(static_cast<std::size_t>(m * n), 0.0);
  for (std::int64_t j = 0; j < code.dimension; ++j) {
    const std::int64_t f = code.info_positions[static_cast<std::size_t>(j)];
    generator[static_cast<std::size_t>(j * n + f)] = 1.0;
    for (std::int64_t i = 0; i < rank; ++i)
      if (reduced[static_cast<std::size_t>(i)][static_cast<std::size_t>(f)]) generator[static_cast<std::size_t>(j * n + pivot_column[static_cast<std::size_t>(i)])] = 1.0;
  }
  for (std::int64_t r = 0; r < m; ++r)
    for (std::int64_t c = 0; c < n; ++c) parity_check[static_cast<std::size_t>(r * n + c)] = h[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
  code.generator = pluss::series::make(generator, {code.dimension, n}).to(ScalarType::Int64);
  code.parity_check = pluss::series::make(parity_check, {m, n}).to(ScalarType::Int64);
  return code;
}

namespace {

void check_ldpc(const LdpcCode &code, const char *who) {
  PLUSS_CHECK(code.parity_check.defined() && code.generator.defined() && code.length > 0 && code.generator.sizes() == Shape({code.dimension, code.length}) &&
                  code.parity_check.size(-1) == code.length,
              std::string("eng::comms::") + who + ": the code is not one made by gallager");
}

Series mod_two(const Series &value) { return value.sub(value.div(2.0).floor().mul(2.0)); }

}  // namespace

Series encode(const LdpcCode &code, const Series &bits) {
  check_ldpc(code, "encode");
  PLUSS_CHECK(bits.defined() && bits.dim() >= 1 && !bits.dtype().is_complex() && bits.size(-1) == code.dimension,
              "eng::comms::encode: bits must be a real [..., k] array with k = code.dimension");
  PLUSS_CHECK(bits.ge(0).all().item<bool>() && bits.le(1).all().item<bool>(), "eng::comms::encode: bits must be zeros and ones");
  return mod_two(bits.to(ScalarType::Float64).matmul(code.generator.to(ScalarType::Float64))).round().to(ScalarType::Int64);
}

LdpcResult decode(const LdpcCode &code, const Series &soft, std::int64_t max_iterations, double scale) {
  check_ldpc(code, "decode");
  PLUSS_CHECK(soft.defined() && soft.dim() >= 1 && soft.dtype().is_floating() && soft.size(-1) == code.length,
              "eng::comms::decode: soft must be a real floating [..., n] array with n = code.length");
  reject_bits_as_soft(soft, "decode");
  PLUSS_CHECK(max_iterations >= 1 && std::isfinite(scale) && scale > 0.0 && scale <= 1.0,
              "eng::comms::decode: max_iterations must be at least 1 and scale in (0, 1]");
  const ScalarType dtype = soft.scalar_type();
  const Series h = code.parity_check.to(dtype).to(soft.platform());                     // [m, n]
  const std::int64_t m = h.size(0), n = code.length;
  const Series edges = h.gt(0.5).to(dtype);
  const Series positions = pluss::series::make([&] { std::vector<double> v(static_cast<std::size_t>(n)); for (std::int64_t i = 0; i < n; ++i) v[static_cast<std::size_t>(i)] = static_cast<double>(i); return v; }()).to(dtype).to(soft.platform());
  const double big = 1.0e30;
  Series variable_to_check = soft.unsqueeze(-2).mul(edges);                           // [..., m, n]
  Series posterior = soft;
  Series done;
  std::int64_t used = 0;
  Series syndrome_ok;
  for (std::int64_t it = 1; it <= max_iterations; ++it) {
    used = it;
    const Series negative = variable_to_check.lt(0.0).to(dtype).mul(edges);
    const Series total_negative = negative.sum(-1, true);
    const Series row_sign = mod_two(total_negative).mul(-2.0).add(1.0);               // +1 for an even number of negatives
    const Series own_sign = variable_to_check.lt(0.0).to(dtype).mul(-2.0).add(1.0);
    const Series magnitude = variable_to_check.abs().mul(edges).add(edges.neg().add(1.0).mul(big));
    const Series smallest = magnitude.amin(-1, true);
    const Series where = magnitude.argmin(-1, true).to(dtype);
    const Series at_min = positions.sub(where).abs().lt(0.5).to(dtype);
    const Series second = magnitude.add(at_min.mul(big)).amin(-1, true);
    const Series others = smallest.add(at_min.mul(second.sub(smallest)));
    const Series check_to_variable = row_sign.mul(own_sign).mul(others).mul(scale).mul(edges);
    posterior = soft.add(check_to_variable.sum(-2));
    variable_to_check = posterior.unsqueeze(-2).sub(check_to_variable).mul(edges);
    const Series hard = posterior.lt(0.0).to(dtype);
    const Series syndrome = mod_two(hard.matmul(h.mT()));
    syndrome_ok = syndrome.amax(-1, false).lt(0.5);
    if (syndrome_ok.all().item<bool>()) break;
  }
  const Series codeword = posterior.lt(0.0).to(ScalarType::Int64);
  std::vector<std::int64_t> info(code.info_positions.begin(), code.info_positions.end());
  const Series bits = codeword.index_select(-1, index_series(info, soft.platform()));
  return {bits, codeword, syndrome_ok, pluss::series::make(std::vector<double>{static_cast<double>(used)}).to(ScalarType::Int64)};
}

// ---- Turbo ---------------------------------------------------------------------------------------------------------

namespace {

Trellis make_rsc_trellis(const TurboCode &code, const char *who) {
  const std::int64_t k = code.constraint_length;
  PLUSS_CHECK(k >= 2 && k <= 8, std::string("eng::comms::") + who + ": constraint_length must lie in [2, 8]");
  PLUSS_CHECK(code.feedforward > 0 && code.feedforward < (std::int64_t{1} << k) && code.feedback > 0 && code.feedback < (std::int64_t{1} << k),
              std::string("eng::comms::") + who + ": feedforward and feedback must be non-zero and fit in constraint_length bits");
  PLUSS_CHECK((code.feedback >> (k - 1)) & 1, std::string("eng::comms::") + who + ": the feedback polynomial must have its leading bit set");
  Trellis t;
  t.memory = k - 1;
  t.states = std::int64_t{1} << t.memory;
  t.outputs = 1;
  std::int64_t fb_mask = 0, ff_mask = 0;
  for (std::int64_t i = 1; i <= t.memory; ++i) {
    fb_mask |= ((code.feedback >> (k - 1 - i)) & 1) << (i - 1);
    ff_mask |= ((code.feedforward >> (k - 1 - i)) & 1) << (i - 1);
  }
  const std::int64_t ff0 = (code.feedforward >> (k - 1)) & 1;
  for (std::int64_t s = 0; s < t.states; ++s) {
    t.terminate.push_back(parity_of(s & fb_mask));
    for (std::int64_t u = 0; u < 2; ++u) {
      const std::int64_t a = u ^ parity_of(s & fb_mask);
      t.output_bits.push_back((a & ff0) ^ parity_of(s & ff_mask));
      t.next_state.push_back(((s << 1) | a) & (t.states - 1));
    }
  }
  link_trellis(t);
  return t;
}

// Runs a one-output trellis over input bits [..., T] (Int64): the parity of every step and the final state.
struct TrellisRun {
  Series parity;
  Series state;
};

TrellisRun run_trellis(const Trellis &t, const Series &inputs, const Series &start_state) {
  const Platform platform = inputs.platform();
  const Series next = index_series(t.next_state, platform), out = index_series(t.output_bits, platform);
  Series state = start_state;
  std::vector<Series> parity;
  for (std::int64_t step = 0; step < inputs.size(-1); ++step) {
    const Series index = state.mul(std::int64_t{2}).add(inputs.select(-1, step));
    const Shape shape(index.sizes().begin(), index.sizes().end());
    parity.push_back(out.index_select(0, index.reshape(Shape{index.numel()})).reshape(shape).unsqueeze(-1));
    state = next.index_select(0, index.reshape(Shape{index.numel()})).reshape(shape);
  }
  return {parity.size() == 1 ? parity[0] : parity[0].unite(std::vector<Series>(parity.begin() + 1, parity.end()), -1), state};
}

// Max-log-MAP (BCJR) extrinsic values of the information bits of a recursive systematic code: `systematic` and `parity` are
// log-likelihood ratios [..., T], `apriori` the prior information [..., T]; `terminated` says the encoder ends in state zero.
Series bcjr(const Trellis &t, const Series &systematic, const Series &parity, const Series &apriori, bool terminated) {
  const std::int64_t steps = systematic.size(-1);
  const ScalarType dtype = systematic.scalar_type();
  const Platform platform = systematic.platform();
  const std::int64_t branches = t.states * 2;
  std::vector<double> usign(static_cast<std::size_t>(branches)), psign(static_cast<std::size_t>(branches));
  for (std::int64_t b = 0; b < branches; ++b) {
    usign[static_cast<std::size_t>(b)] = (b & 1) ? -1.0 : 1.0;
    psign[static_cast<std::size_t>(b)] = 1.0 - 2.0 * static_cast<double>(t.output_bits[static_cast<std::size_t>(b)]);
  }
  const Series u_sign = pluss::series::make(usign).to(dtype).to(platform), p_sign = pluss::series::make(psign).to(dtype).to(platform);
  const Series pred_state = index_series(t.pred_state, platform), pred_branch = index_series(t.pred_branch, platform), next = index_series(t.next_state, platform);
  const Series prior = systematic.add(apriori);
  std::vector<Series> gamma(static_cast<std::size_t>(steps));
  for (std::int64_t k = 0; k < steps; ++k)
    gamma[static_cast<std::size_t>(k)] = prior.select(-1, k).unsqueeze(-1).mul(u_sign).add(parity.select(-1, k).unsqueeze(-1).mul(p_sign)).mul(0.5);
  Shape pair = Shape(systematic.sizes().begin(), systematic.sizes().end() - 1);
  pair.push_back(t.states);
  pair.push_back(2);
  const Shape lead(systematic.sizes().begin(), systematic.sizes().end() - 1);
  std::vector<double> start(static_cast<std::size_t>(t.states), -1.0e30), end(static_cast<std::size_t>(t.states), terminated ? -1.0e30 : 0.0);
  start[0] = 0.0;
  if (terminated) end[0] = 0.0;
  const Config cfg = Config{}.with_dtype(dtype).with_platform(platform);
  std::vector<Series> alpha;
  alpha.push_back(pluss::series::zeros(with(lead, t.states), cfg).add(pluss::series::make(start).to(dtype).to(platform)));
  for (std::int64_t k = 0; k < steps; ++k) {
    const Series candidates = alpha.back().index_select(-1, pred_state).add(gamma[static_cast<std::size_t>(k)].index_select(-1, pred_branch)).reshape(pair);
    const Series next_alpha = candidates.amax(-1, false);
    alpha.push_back(next_alpha.sub(next_alpha.amax(-1, true)));
  }
  std::vector<Series> beta(static_cast<std::size_t>(steps + 1));
  beta[static_cast<std::size_t>(steps)] = pluss::series::zeros(with(lead, t.states), cfg).add(pluss::series::make(end).to(dtype).to(platform));
  for (std::int64_t k = steps - 1; k >= 0; --k) {
    const Series candidates = gamma[static_cast<std::size_t>(k)].reshape(pair).add(beta[static_cast<std::size_t>(k + 1)].index_select(-1, next).reshape(pair));
    const Series previous = candidates.amax(-1, false);
    beta[static_cast<std::size_t>(k)] = previous.sub(previous.amax(-1, true));
  }
  std::vector<Series> extrinsic;
  for (std::int64_t k = 0; k < steps; ++k) {
    const Series metric = alpha[static_cast<std::size_t>(k)].unsqueeze(-1).add(gamma[static_cast<std::size_t>(k)].reshape(pair)).add(beta[static_cast<std::size_t>(k + 1)].index_select(-1, next).reshape(pair));
    const Series zero = metric.select(-1, 0).amax(-1, false), one = metric.select(-1, 1).amax(-1, false);
    extrinsic.push_back(zero.sub(one).sub(prior.select(-1, k)).unsqueeze(-1));
  }
  return extrinsic.size() == 1 ? extrinsic[0] : extrinsic[0].unite(std::vector<Series>(extrinsic.begin() + 1, extrinsic.end()), -1);
}

}  // namespace

Series shuffle(std::int64_t length, std::uint64_t seed) {
  PLUSS_CHECK(length >= 2, "eng::comms::shuffle: length must be at least 2");
  return pluss::series::randn({length}, ScalarType::Float64, seed).argsort().to(ScalarType::Int64);
}

namespace {

void check_interleaver(const Series &interleaver, std::int64_t length, const char *who) {
  PLUSS_CHECK(interleaver.defined() && interleaver.dim() == 1 && interleaver.numel() == length && !interleaver.dtype().is_floating(),
              std::string("eng::comms::") + who + ": interleaver must be an integer vector with one entry per information bit");
  const auto values = interleaver.to(Platform::CPU, ScalarType::Int64).items<std::int64_t>();
  std::vector<bool> seen(static_cast<std::size_t>(length), false);
  for (std::int64_t v : values) {
    PLUSS_CHECK(v >= 0 && v < length && !seen[static_cast<std::size_t>(v)], std::string("eng::comms::") + who + ": interleaver must be a permutation of 0 .. length - 1");
    seen[static_cast<std::size_t>(v)] = true;
  }
}

}  // namespace

Series encode(const TurboCode &code, const Series &interleaver, const Series &bits) {
  const Trellis t = make_rsc_trellis(code, "encode");
  PLUSS_CHECK(bits.defined() && bits.dim() >= 1 && !bits.dtype().is_complex() && bits.size(-1) >= 2, "eng::comms::encode: bits must be a real [..., N] array with N >= 2");
  PLUSS_CHECK(bits.ge(0).all().item<bool>() && bits.le(1).all().item<bool>(), "eng::comms::encode: bits must be zeros and ones");
  const std::int64_t n = bits.size(-1);
  check_interleaver(interleaver, n, "encode");
  const Platform platform = bits.platform();
  const Series info = bits.to(ScalarType::Int64);
  const Shape lead(bits.sizes().begin(), bits.sizes().end() - 1);
  const Series zero_state = pluss::series::zeros(lead, Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
  const TrellisRun first = run_trellis(t, info, zero_state);
  // terminate the first encoder: the input that drives the state to zero, step by step
  const Series terminate = index_series(t.terminate, platform);
  std::vector<Series> tail_sys, tail_par;
  Series state = first.state;
  const Series next = index_series(t.next_state, platform), out = index_series(t.output_bits, platform);
  for (std::int64_t step = 0; step < t.memory; ++step) {
    const Shape shape(state.sizes().begin(), state.sizes().end());
    const Series flat_state = state.reshape(Shape{state.numel()});
    const Series u = terminate.index_select(0, flat_state).reshape(shape);
    const Series index = state.mul(std::int64_t{2}).add(u);
    const Series flat_index = index.reshape(Shape{index.numel()});
    tail_sys.push_back(u.unsqueeze(-1));
    tail_par.push_back(out.index_select(0, flat_index).reshape(shape).unsqueeze(-1));
    state = next.index_select(0, flat_index).reshape(shape);
  }
  const Series second = run_trellis(t, info.index_select(-1, interleaver.to(ScalarType::Int64).to(platform)), zero_state).parity;
  auto join = [](const std::vector<Series> &v) { return v.size() == 1 ? v[0] : v[0].unite(std::vector<Series>(v.begin() + 1, v.end()), -1); };
  return info.unite({first.parity, second, join(tail_sys), join(tail_par)}, -1);
}

Series decode(const TurboCode &code, const Series &interleaver, const Series &soft, std::int64_t iterations) {
  const Trellis t = make_rsc_trellis(code, "decode");
  const std::int64_t m = t.memory;
  PLUSS_CHECK(soft.defined() && soft.dim() >= 1 && soft.dtype().is_floating() && (soft.size(-1) - 2 * m) % 3 == 0 && (soft.size(-1) - 2 * m) / 3 >= 2,
              "eng::comms::decode: soft must be a real [..., 3 N + 2 (constraint_length - 1)] array");
  reject_bits_as_soft(soft, "decode");
  PLUSS_CHECK(iterations >= 1, "eng::comms::decode: iterations must be at least 1");
  const std::int64_t n = (soft.size(-1) - 2 * m) / 3;
  check_interleaver(interleaver, n, "decode");
  const Platform platform = soft.platform();
  const Series permute = interleaver.to(ScalarType::Int64).to(platform);
  const Series inverse = permute.argsort().to(ScalarType::Int64);
  const Series sys = soft.slice(-1, 0, n), p1 = soft.slice(-1, n, 2 * n), p2 = soft.slice(-1, 2 * n, 3 * n);
  const Series tail_sys = soft.slice(-1, 3 * n, 3 * n + m), tail_p1 = soft.slice(-1, 3 * n + m, 3 * n + 2 * m);
  const Series sys1 = sys.unite({tail_sys}, -1), par1 = p1.unite({tail_p1}, -1);
  const Series sys2 = sys.index_select(-1, permute);
  const Shape lead(soft.sizes().begin(), soft.sizes().end() - 1);
  const Config cfg = Config{}.with_dtype(soft.scalar_type()).with_platform(platform);
  const Series tail_zero = pluss::series::zeros(with(lead, m), cfg);
  const double damping = 0.75;
  Series from_second = pluss::series::zeros(with(lead, n), cfg);
  Series from_first = from_second;
  for (std::int64_t it = 0; it < iterations; ++it) {
    const Series ext1 = bcjr(t, sys1, par1, from_second.unite({tail_zero}, -1), true).slice(-1, 0, n).mul(damping);
    from_first = ext1;
    const Series ext2 = bcjr(t, sys2, p2, ext1.index_select(-1, permute), false).mul(damping);
    from_second = ext2.index_select(-1, inverse);
  }
  return sys.add(from_first).add(from_second).lt(0.0).to(ScalarType::Int64);
}

// ---- Gardner timing ------------------------------------------------------------------------------------------------

namespace {

// Cubic Lagrange interpolation of a [..., L] signal at fractional positions [...] (one per frame), in samples.
Series interpolate(const Series &signal, const Series &position) {
  const std::int64_t length = signal.size(-1);
  const ScalarType real = real_type_of(signal.scalar_type());
  const Series base = position.floor().clamp_min(1.0).clamp_max(static_cast<double>(length - 3));
  const Series mu = position.sub(base).to(real);
  const Series index = base.to(ScalarType::Int64);
  auto sample = [&](std::int64_t offset) {
    const Series at = index.add(offset).unsqueeze(-1);
    if (!signal.dtype().is_complex()) return signal.gather(-1, at).squeeze(-1);
    return pluss::series::complex(signal.real().gather(-1, at).squeeze(-1), signal.imag().gather(-1, at).squeeze(-1), signal.scalar_type());
  };
  const Series c0 = mu.mul(mu.sub(1.0)).mul(mu.sub(2.0)).mul(-1.0 / 6.0);
  const Series c1 = mu.add(1.0).mul(mu.sub(1.0)).mul(mu.sub(2.0)).mul(0.5);
  const Series c2 = mu.add(1.0).mul(mu).mul(mu.sub(2.0)).mul(-0.5);
  const Series c3 = mu.add(1.0).mul(mu).mul(mu.sub(1.0)).mul(1.0 / 6.0);
  return sample(-1).mul(c0).add(sample(0).mul(c1)).add(sample(1).mul(c2)).add(sample(2).mul(c3));
}

}  // namespace

TimingResult gardner(const Series &filtered, std::int64_t samples_per_symbol, double loop_bandwidth) {
  PLUSS_CHECK(samples_per_symbol >= 2, "eng::comms::gardner: samples_per_symbol must be at least 2");
  PLUSS_CHECK(std::isfinite(loop_bandwidth) && loop_bandwidth > 0.0 && loop_bandwidth < 0.5, "eng::comms::gardner: loop_bandwidth must lie in (0, 0.5)");
  PLUSS_CHECK(filtered.defined() && filtered.dim() >= 1 && filtered.size(-1) >= 8 * samples_per_symbol && (filtered.dtype().is_floating() || filtered.dtype().is_complex()),
              "eng::comms::gardner: filtered must be a real or complex [..., L] array holding at least eight symbols");
  const double zeta = 1.0 / std::sqrt(2.0);
  const double theta = loop_bandwidth / (zeta + 1.0 / (4.0 * zeta));
  const double d = 1.0 + 2.0 * zeta * theta + theta * theta;
  const double kp = 4.0 * zeta * theta / d, ki = 4.0 * theta * theta / d;
  const std::int64_t length = filtered.size(-1), sps = samples_per_symbol;
  const std::int64_t count = (length - 3 * sps) / sps;
  const ScalarType real = real_type_of(filtered.scalar_type());
  const Shape lead(filtered.sizes().begin(), filtered.sizes().end() - 1);
  const Config cfg = Config{}.with_dtype(real).with_platform(filtered.platform());
  Series position = pluss::series::zeros(lead, cfg).add(static_cast<double>(sps));
  Series integrator = pluss::series::zeros(lead, cfg);
  Series previous;
  std::vector<Series> symbols, offsets;
  for (std::int64_t n = 0; n < count; ++n) {
    const Series y = interpolate(filtered, position);
    offsets.push_back(position.unsqueeze(-1));
    symbols.push_back(y.unsqueeze(-1));
    Series adjust = pluss::series::zeros(lead, cfg);
    if (n > 0) {
      const Series middle = interpolate(filtered, position.sub(0.5 * static_cast<double>(sps)));
      const Series difference = y.sub(previous);
      const Series error = (middle.dtype().is_complex() ? middle.conj().mul(difference).real() : middle.mul(difference));
      const Series power = (y.abs().mul(y.abs()).add(previous.abs().mul(previous.abs()))).mul(0.5).add(1e-12);
      const Series normalised = error.div(power);
      integrator = integrator.add(normalised.mul(ki));
      adjust = normalised.mul(kp).add(integrator);
    }
    previous = y;
    position = position.add(static_cast<double>(sps)).sub(adjust.mul(static_cast<double>(sps)));   // a late strobe gives a positive error
  }
  auto join = [](const std::vector<Series> &v) { return v.size() == 1 ? v[0] : v[0].unite(std::vector<Series>(v.begin() + 1, v.end()), -1); };
  return {join(symbols), join(offsets)};
}

} // namespace pluss::subsp::eng::comms
