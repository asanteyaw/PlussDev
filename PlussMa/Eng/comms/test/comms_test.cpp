#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace comms = pluss::subsp::eng::comms;
using pluss::Series;

namespace {

void expect(bool value, const std::string &message) {
  if (!value) throw std::runtime_error(message);
}

void expect_error(const std::function<void()> &call, const std::string &message) {
  try {
    call();
  } catch (const pluss::Error &) {
    return;
  }
  throw std::runtime_error(message);
}

std::vector<std::complex<double>> points_of(const comms::Constellation &c) {
  const Series re = c.points.real().to(pluss::ScalarType::Float64), im = c.points.imag().to(pluss::ScalarType::Float64);
  std::vector<std::complex<double>> out;
  const auto r = re.items<double>(), i = im.items<double>();
  for (std::size_t k = 0; k < r.size(); ++k) out.emplace_back(r[k], i[k]);
  return out;
}

int popcount(std::int64_t v) {
  int n = 0;
  for (; v; v >>= 1) n += static_cast<int>(v & 1);
  return n;
}

Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}

void constellation_checks() {
  for (std::int64_t order : {2, 4, 8, 16}) {
    const auto c = comms::psk(order);
    const auto p = points_of(c);
    double energy = 0.0;
    for (auto v : p) energy += std::norm(v);
    expect(std::abs(energy / static_cast<double>(order) - 1.0) < 1e-12 && c.bits == std::log2(order), "PSK energy and bits");
    // Gray: points adjacent on the circle have labels one bit apart
    std::vector<std::int64_t> by_angle(static_cast<std::size_t>(order));
    for (std::int64_t label = 0; label < order; ++label) {
      double angle = std::arg(p[static_cast<std::size_t>(label)]);
      if (angle < -1e-9) angle += 2 * M_PI;
      by_angle[static_cast<std::size_t>(std::llround(angle / (2 * M_PI / order)) % order)] = label;
    }
    for (std::int64_t k = 0; k < order; ++k)
      expect(popcount(by_angle[static_cast<std::size_t>(k)] ^ by_angle[static_cast<std::size_t>((k + 1) % order)]) == 1, "PSK Gray property");
  }
  for (std::int64_t order : {4, 16, 64, 256}) {
    const auto c = comms::qam(order);
    const auto p = points_of(c);
    double energy = 0.0;
    for (auto v : p) energy += std::norm(v);
    expect(std::abs(energy / static_cast<double>(order) - 1.0) < 1e-12, "QAM energy");
    // nearest neighbours (smallest distance) differ by one bit
    double dmin = 1e9;
    for (std::size_t a = 0; a < p.size(); ++a)
      for (std::size_t b = a + 1; b < p.size(); ++b) dmin = std::min(dmin, std::abs(p[a] - p[b]));
    for (std::size_t a = 0; a < p.size(); ++a)
      for (std::size_t b = a + 1; b < p.size(); ++b)
        if (std::abs(std::abs(p[a] - p[b]) - dmin) < 1e-9) expect(popcount(static_cast<std::int64_t>(a ^ b)) == 1, "QAM Gray property");
  }
  // natural labelling: label k at angle 2 pi k / M
  const auto natural = points_of(comms::psk(8, {.gray = false}));
  expect(std::abs(natural[3] - std::polar(1.0, 2 * M_PI * 3 / 8)) < 1e-12, "natural PSK labels");
  const auto rotated = points_of(comms::psk(4, {.gray = true, .phase = M_PI / 4}));
  expect(std::abs(std::arg(rotated[0]) - M_PI / 4) < 1e-12, "PSK phase");
  expect_error([] { comms::psk(6); }, "non power-of-two PSK accepted");
  expect_error([] { comms::qam(8); }, "non-square QAM accepted");
  expect_error([] { comms::qam(2); }, "binary QAM accepted");
  const auto c32 = comms::psk(4, {.dtype = pluss::ScalarType::Complex64});
  expect(c32.points.scalar_type() == pluss::ScalarType::Complex64, "Complex64 constellation");
}

void mapping_checks() {
  for (std::int64_t order : {2, 4, 16, 64}) {
    const auto c = order == 2 || order == 4 ? comms::psk(order) : comms::qam(order);
    const std::int64_t n = c.bits * 500;
    const auto bits = random_bits(n, 11);
    const auto sym = comms::modulate(c, bits);
    expect(sym.numel() == 500 && sym.dtype().is_complex(), "modulate shape");
    const auto back = comms::demod(c, sym);
    expect(comms::ber(bits, back).item<double>() == 0.0, "noiseless round trip");
    expect(comms::to_bits(comms::to_labels(bits, c.bits), c.bits).sub(bits).abs().amax().item<double>() == 0.0, "label packing");
  }
  // batch
  const auto c = comms::qam(16);
  const auto bits = random_bits(2 * 4 * 50, 5).reshape({2, 200});
  const auto sym = comms::modulate(c, bits);
  expect(sym.sizes() == std::vector<std::int64_t>{2, 50}, "batched modulate shape");
  expect(comms::demod(c, sym).sizes() == std::vector<std::int64_t>{2, 200}, "batched demodulate shape");
  expect_error([&] { comms::modulate(c, random_bits(7, 1)); }, "ragged bits accepted");
  expect_error([&] { comms::to_labels(pluss::series::make(std::vector<double>{0, 2, 1, 0}), 2); }, "non-binary bits accepted");
  expect_error([&] { comms::symbols(c, pluss::series::make(std::vector<double>{16})); }, "label outside accepted");
}

void llr_checks() {
  const auto c = comms::qam(16);
  const auto bits = random_bits(4 * 2000, 3);
  const auto sym = comms::modulate(c, bits);
  const double n0 = comms::n0(8.0, c);
  const auto rx = comms::noise(sym, n0, 99);
  const auto soft = comms::llr(c, rx, n0);
  expect(soft.numel() == bits.numel(), "llr shape");
  // the sign of the LLR is the hard decision (positive favours zero)
  const auto hard = comms::demod(c, rx);
  const auto from_llr = soft.lt(0.0).to(pluss::ScalarType::Int64);
  expect(comms::ber(hard, from_llr).item<double>() < 1e-3, "llr sign agrees with the hard decision");
  // noiseless: the LLR magnitude is large and has the right sign
  const auto clean = comms::llr(c, sym, 0.1);
  expect(comms::ber(bits, clean.lt(0.0).to(pluss::ScalarType::Int64)).item<double>() == 0.0, "noiseless llr");
  expect_error([&] { comms::llr(c, rx, 0.0); }, "zero noise variance accepted");
}

void pulse_checks() {
  const auto taps = comms::rrc(0.35, 8, 10);
  expect(taps.numel() == 81, "rrc length");
  expect(std::abs(taps.mul(taps).sum().item<double>() - 1.0) < 1e-12, "rrc unit energy");
  const auto h = taps.items<double>();
  for (std::size_t i = 0; i < h.size(); ++i) expect(std::abs(h[i] - h[h.size() - 1 - i]) < 1e-12, "rrc symmetric");
  // the matched pair is Nyquist: the raised-cosine response is zero at the other symbol instants
  const auto rc = pluss::subsp::eng::signal::conv(taps, taps);
  const auto r = rc.items<double>();
  const std::size_t mid = r.size() / 2;
  expect(std::abs(r[mid] - 1.0) < 1e-12, "raised cosine peak");
  for (int k = 1; k <= 6; ++k) expect(std::abs(r[mid + static_cast<std::size_t>(8 * k)]) < 1e-2, "no intersymbol interference");
  // a noiseless chain recovers the symbols
  const auto c = comms::qam(16);
  const auto bits = random_bits(4 * 64, 21);
  const auto sym = comms::modulate(c, bits);
  const auto wave = comms::shape(sym, taps, 8);
  expect(wave.size(-1) == 63 * 8 + 81, "shape length");
  const auto rx = comms::match(wave, taps, 8, 64);
  expect(comms::evm(rx, sym).item<double>() < 3e-2, "matched filter recovers the symbols");
  expect(comms::demod(c, rx).sub(bits).abs().amax().item<double>() == 0.0, "shaped chain bits");
  // batch of waveforms
  const auto batch = comms::shape(sym.reshape({2, 32}), taps, 8);
  expect(batch.sizes() == std::vector<std::int64_t>{2, 31 * 8 + 81}, "batched shape");
  expect_error([] { comms::rrc(1.5, 8, 10); }, "rolloff above one accepted");
  expect_error([&] { comms::match(wave, taps, 8, 200); }, "too many symbols accepted");
  // rolloff zero is the sinc pulse
  expect(std::abs(comms::rrc(0.0, 4, 8).mul(comms::rrc(0.0, 4, 8)).sum().item<double>() - 1.0) < 1e-12, "rolloff zero");
}

void channel_and_metric_checks() {
  const auto c = comms::psk(4);
  const auto sym = comms::symbols(c, pluss::series::zeros({20000}, pluss::Config{}.with_dtype(pluss::ScalarType::Int64)));
  const auto noisy = comms::noise(sym, 0.25, 7);
  const auto noise = noisy.sub(sym);
  const double power = noise.abs().mul(noise.abs()).mean().item<double>();
  expect(std::abs(power - 0.25) < 0.01, "noise variance");
  expect(std::abs(noise.real().mul(noise.real()).mean().item<double>() - 0.125) < 0.006, "noise split equally");
  expect(comms::noise(sym, 0.25, 7).sub(noisy).abs().amax().item<double>() == 0.0, "noise reproducible from the seed");
  expect(comms::noise(sym, 0.25, 8).sub(noisy).abs().amax().item<double>() > 0.0, "different seeds differ");
  const double evm = comms::evm(noisy, sym).item<double>();
  expect(std::abs(evm - 0.5) < 0.02, "evm is the noise-to-signal amplitude ratio");
  expect(std::abs(comms::n0(0.0, c) - 0.5) < 1e-12, "noise variance at 0 dB");
  const auto a = pluss::series::make(std::vector<double>{0, 1, 1, 0}), b = pluss::series::make(std::vector<double>{0, 1, 0, 1});
  expect(std::abs(comms::ber(a, b).item<double>() - 0.5) < 1e-12, "bit error rate");
  const auto ser = comms::ser(a.reshape({2, 2}), b.reshape({2, 2}));
  expect(ser.sizes() == std::vector<std::int64_t>{2} && std::abs(ser.items<double>()[1] - 1.0) < 1e-12, "batched symbol error rate");
  expect_error([&] { comms::ber(a, b.slice(0, 0, 3)); }, "ragged error rate accepted");
  // Float32 signals keep their dtype
  const auto s32 = comms::psk(4, {.dtype = pluss::ScalarType::Complex64});
  expect(comms::noise(comms::symbols(s32, pluss::series::zeros({10}, pluss::Config{}.with_dtype(pluss::ScalarType::Int64))), 0.1, 3).scalar_type() ==
             pluss::ScalarType::Complex64, "complex64 awgn");
}

void ber_theory_checks() {
  // simulated bit error rate against theory at a few Eb/N0 values
  struct Case { comms::Constellation c; double ebn0; double tolerance; };
  const std::vector<Case> cases = {{comms::psk(2), 4.0, 0.15}, {comms::psk(4), 6.0, 0.15}, {comms::qam(16), 8.0, 0.15}, {comms::qam(64), 12.0, 0.2},
                                   {comms::psk(8), 10.0, 0.2}};
  std::uint64_t seed = 100;
  for (const auto &t : cases) {
    const std::int64_t symbols = 60000;
    const auto bits = random_bits(symbols * t.c.bits, seed++);
    const auto rx = comms::noise(comms::modulate(t.c, bits), comms::n0(t.ebn0, t.c), seed++);
    const double measured = comms::ber(bits, comms::demod(t.c, rx)).item<double>();
    const double theory = comms::theory(t.c, pluss::series::make(std::vector<double>{t.ebn0})).items<double>()[0];
    expect(std::abs(measured / theory - 1.0) < t.tolerance, "simulated BER matches theory for order " + std::to_string(t.c.order));
  }
  // exact BPSK value at 0 dB: Q(sqrt 2)
  const double q = 0.5 * std::erfc(1.0);
  expect(std::abs(comms::theory(comms::psk(2), pluss::series::make(std::vector<double>{0.0})).items<double>()[0] - q) < 1e-12, "BPSK theory");
  const auto curve = comms::theory(comms::qam(16), pluss::series::make(std::vector<double>{0, 4, 8, 12}));
  const auto v = curve.items<double>();
  for (int i = 1; i < 4; ++i) expect(v[static_cast<std::size_t>(i)] < v[static_cast<std::size_t>(i - 1)], "theory decreases with Eb/N0");
}


Series soft_of(const Series &bits) { return pluss::series::make(std::vector<double>{1.0}).mul(1.0).add(bits.to(pluss::ScalarType::Float64).mul(-2.0)); }

void coding_checks() {
  // the textbook (7, 5) code: input 1 0 1 1 and its two-bit tail give 11 10 00 01 01 11
  comms::ConvolutionalCode small{3, {07, 05}};
  const auto word = comms::encode(small, pluss::series::make(std::vector<std::int64_t>{1, 0, 1, 1}));
  const std::vector<std::int64_t> expected{1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1};
  expect(word.items<std::int64_t>() == expected, "(7, 5) codeword");
  expect(comms::decode(small, soft_of(word)).items<std::int64_t>() == std::vector<std::int64_t>{1, 0, 1, 1}, "noiseless viterbi (7, 5)");
  // the K = 7 rate-1/2 code: round trip, batch, and hard errors within the correcting power
  comms::ConvolutionalCode strong;
  const auto bits = random_bits(2 * 300, 41).reshape({2, 300});
  const auto coded = comms::encode(strong, bits);
  expect(coded.sizes() == std::vector<std::int64_t>{2, (300 + 6) * 2}, "coded length");
  expect(comms::decode(strong, soft_of(coded)).sub(bits).abs().amax().item<double>() == 0.0, "noiseless viterbi K = 7");
  // flip a few coded bits: the code corrects them (free distance 10)
  auto flipped = coded.to(pluss::ScalarType::Float64);
  std::vector<double> mask(static_cast<std::size_t>(coded.size(-1)), 0.0);
  mask[10] = mask[100] = mask[300] = mask[500] = 1.0;
  flipped = flipped.sub(pluss::series::make(mask).mul(2.0).mul(flipped.sub(0.5)));
  expect(comms::decode(strong, soft_of(flipped)).sub(bits).abs().amax().item<double>() == 0.0, "viterbi corrects isolated errors");
  // coding gain: BPSK at Eb/N0 = 4 dB, rate 1/2 so each coded bit has half the energy per bit
  const auto info = random_bits(20000, 55);
  const auto bpsk = comms::psk(2);
  const auto sent = comms::encode(strong, info);
  const double n0 = comms::n0(4.0, bpsk) / 0.5;
  const auto rx = comms::noise(comms::modulate(bpsk, sent), n0, 56);
  const auto decoded = comms::decode(strong, comms::llr(bpsk, rx, n0));
  const double coded_ber = comms::ber(info, decoded).item<double>();
  const auto raw_rx = comms::noise(comms::modulate(bpsk, info), comms::n0(4.0, bpsk), 57);
  const double raw_ber = comms::ber(info, comms::demod(bpsk, raw_rx)).item<double>();
  expect(coded_ber < 0.1 * raw_ber, "the code beats uncoded BPSK at the same Eb/N0");
  expect_error([&] { comms::encode(comms::ConvolutionalCode{1, {1, 1}}, info); }, "constraint length one accepted");
  expect_error([&] { comms::encode(comms::ConvolutionalCode{3, {07}}, info); }, "single generator accepted");
  expect_error([&] { comms::encode(comms::ConvolutionalCode{3, {07, 011}}, info); }, "oversized generator accepted");
  expect_error([&] { comms::decode(strong, pluss::series::zeros({5}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))); }, "ragged codeword accepted");
}

void sync_checks() {
  const auto c = comms::qam(16);
  const auto taps = comms::rrc(0.35, 8, 12);
  const auto symbols = comms::modulate(c, random_bits(4 * 400, 71));
  const auto wave = comms::shape(symbols, taps, 8);
  // timing: a fractional delay is recovered, then removed
  for (double d : {0.0, 1.37, 2.9, -2.4}) {
    const auto late = comms::delay(wave, pluss::series::make(std::vector<double>{d}).select(0, 0));
    const double estimate = comms::timing(late, 8).item<double>();
    expect(std::abs(estimate - d) < 0.25, "timing estimate for delay " + std::to_string(d));
  }
  const auto late = comms::delay(wave, pluss::series::make(std::vector<double>{2.37}).select(0, 0));
  const double est = comms::timing(late, 8).item<double>();
  const auto fixed = comms::delay(late, pluss::series::make(std::vector<double>{-est}).select(0, 0));
  const auto rx = comms::match(fixed, taps, 8, 380);
  expect(comms::evm(rx.slice(-1, 10, 370), symbols.slice(-1, 10, 370)).item<double>() < 0.06, "timing recovery restores the symbols");
  expect(comms::delay(wave, pluss::series::make(std::vector<double>{0.0}).select(0, 0)).sub(wave).abs().amax().item<double>() < 1e-12, "zero delay");
  // carrier: frequency offset estimate and phase-locked loop on QPSK
  const auto qpsk = comms::psk(4);
  const auto data = comms::modulate(qpsk, random_bits(2 * 3000, 81));
  const auto offset = comms::derot(data, pluss::series::make(std::vector<double>{-0.0031}).select(0, 0), pluss::series::make(std::vector<double>{-0.4}).select(0, 0));
  const double f = comms::cfo(offset, 4).item<double>();
  expect(std::abs(f - 0.0031) < 2e-4, "frequency offset estimate");
  const auto noisy = comms::noise(offset, 0.01, 5);
  const auto locked = comms::pll(noisy, 4, 0.02);
  expect(locked.symbols.sizes() == data.sizes() && locked.phase.sizes() == data.sizes(), "carrier result shapes");
  // after lock the symbols sit on the constellation up to a multiple of 90 degrees
  const auto tail = locked.symbols.slice(-1, 1500, 3000);
  const auto angle = tail.imag().atan2(tail.real()).sub(M_PI / 4.0);
  const auto wrapped = angle.sub(angle.div(M_PI / 2.0).round().mul(M_PI / 2.0));
  (void)wrapped;
  const auto points = comms::detect(qpsk, tail);
  const auto detected = comms::symbols(qpsk, points);
  const auto fixed_rotation = detected.mul(tail.conj()).mean();   // a constant rotation of 0, 90, 180 or 270 degrees
  expect(std::abs(std::abs(fixed_rotation.abs().item<double>()) - 1.0) < 0.1, "the loop locks to the constellation");
  expect_error([&] { comms::timing(wave, 2); }, "two samples per symbol accepted");
  expect_error([&] { comms::pll(noisy, 3, 0.02); }, "order three accepted");
  expect_error([&] { comms::pll(noisy, 4, 0.0); }, "zero bandwidth accepted");
}


void soft_guard_checks() {
  const comms::ConvolutionalCode strong;
  const auto word = comms::encode(strong, random_bits(40, 3));
  const auto as_bits = word.to(pluss::ScalarType::Float64);   // zeros and ones are bits, not log-likelihood ratios
  expect_error([&] { comms::decode(strong, as_bits); }, "bits as soft values accepted by the Viterbi decoder");
  const auto ldpc = comms::gallager(600, 3, 6, 1);
  const auto ldpc_bits = comms::encode(ldpc, random_bits(ldpc.dimension, 4)).to(pluss::ScalarType::Float64);
  expect_error([&] { comms::decode(ldpc, ldpc_bits); }, "bits as soft values accepted by the LDPC decoder");
  const comms::TurboCode turbo;
  const auto pi = comms::shuffle(40, 1);
  const auto turbo_bits = comms::encode(turbo, pi, random_bits(40, 5)).to(pluss::ScalarType::Float64);
  expect_error([&] { comms::decode(turbo, pi, turbo_bits, 2); }, "bits as soft values accepted by the turbo decoder");
  // hard decisions of an all-zero word are all +1 and must still be accepted
  const auto zeros = comms::encode(strong, pluss::series::zeros({40}, pluss::Config{}.with_dtype(pluss::ScalarType::Int64)));
  expect(comms::decode(strong, soft_of(zeros)).abs().amax().item<double>() == 0.0, "an all-zero word as +1 hard decisions decodes");
}

void ldpc_checks() {
  const auto code = comms::gallager(1200, 3, 6, 1);
  expect(code.length == 1200 && code.dimension >= 600 && code.generator.sizes() == std::vector<std::int64_t>{code.dimension, 1200}, "ldpc shape");
  const auto syndrome = code.generator.to(pluss::ScalarType::Float64).matmul(code.parity_check.to(pluss::ScalarType::Float64).mT());
  expect(syndrome.sub(syndrome.div(2.0).floor().mul(2.0)).abs().amax().item<double>() == 0.0, "generator and parity checks agree");
  expect(code.parity_check.to(pluss::ScalarType::Float64).sum(-1).sub(6.0).abs().amax().item<double>() == 0.0, "regular row weight");
  const auto bits = random_bits(code.dimension * 2, 31).reshape({2, code.dimension});
  const auto word = comms::encode(code, bits);
  expect(word.sizes() == std::vector<std::int64_t>{2, 1200}, "ldpc codeword shape");
  const auto noiseless = comms::decode(code, pluss::series::make(std::vector<double>{1.0}).add(word.to(pluss::ScalarType::Float64).mul(-2.0)).mul(5.0));
  expect(noiseless.converged.all().item<bool>() && noiseless.bits.sub(bits).abs().amax().item<double>() == 0.0, "noiseless ldpc");
  // coding gain: BPSK at Eb/N0 = 3 dB, rate k / n
  const double rate = static_cast<double>(code.dimension) / 1200.0;
  const auto bpsk = comms::psk(2);
  const auto info = random_bits(code.dimension * 4, 32).reshape({4, code.dimension});
  const double n0 = comms::n0(3.0, bpsk) / rate;
  const auto rx = comms::noise(comms::modulate(bpsk, comms::encode(code, info)), n0, 33);
  const auto out = comms::decode(code, comms::llr(bpsk, rx, n0), 60);
  const double coded = comms::ber(info, out.bits).mean().item<double>();
  const auto raw = comms::noise(comms::modulate(bpsk, info), comms::n0(3.0, bpsk), 34);
  const double uncoded = comms::ber(info, comms::demod(bpsk, raw)).mean().item<double>();
  expect(coded < 0.2 * uncoded, "ldpc beats uncoded BPSK");
  expect_error([&] { comms::gallager(1000, 3, 6, 1); }, "length not a multiple accepted");
  expect_error([&] { comms::gallager(1200, 1, 6, 1); }, "column weight one accepted");
  expect_error([&] { comms::encode(code, random_bits(5, 1)); }, "wrong message length accepted");
  expect_error([&] { comms::decode(code, random_bits(5, 1).to(pluss::ScalarType::Float64)); }, "wrong soft length accepted");
}

void turbo_checks() {
  const comms::TurboCode code;
  const std::int64_t n = 400;
  const auto pi = comms::shuffle(n, 5);
  {
    auto values = pi.items<std::int64_t>();
    std::sort(values.begin(), values.end());
    bool permutation = static_cast<std::int64_t>(values.size()) == n;
    for (std::int64_t i = 0; i < n && permutation; ++i) permutation = values[static_cast<std::size_t>(i)] == i;
    expect(permutation, "interleaver is a permutation");
  }
  const auto bits = random_bits(2 * n, 61).reshape({2, n});
  const auto word = comms::encode(code, pi, bits);
  expect(word.sizes() == std::vector<std::int64_t>{2, 3 * n + 6}, "turbo codeword length");
  expect(word.slice(-1, 0, n).sub(bits).abs().amax().item<double>() == 0.0, "systematic part");
  const auto soft = pluss::series::make(std::vector<double>{1.0}).add(word.to(pluss::ScalarType::Float64).mul(-2.0)).mul(4.0);
  expect(comms::decode(code, pi, soft, 4).sub(bits).abs().amax().item<double>() == 0.0, "noiseless turbo");
  // gain: BPSK at Eb/N0 = 1.5 dB, rate about 1/3
  const auto bpsk = comms::psk(2);
  const auto info = random_bits(n * 10, 62).reshape({10, n});
  const double rate = static_cast<double>(n) / static_cast<double>(3 * n + 6);
  const double n0 = comms::n0(1.5, bpsk) / rate;
  const auto rx = comms::noise(comms::modulate(bpsk, comms::encode(code, pi, info)), n0, 63);
  const auto decoded = comms::decode(code, pi, comms::llr(bpsk, rx, n0), 8);
  const double coded = comms::ber(info, decoded).mean().item<double>();
  const double uncoded = comms::ber(info, comms::demod(bpsk, comms::noise(comms::modulate(bpsk, info), comms::n0(1.5, bpsk), 64))).mean().item<double>();
  expect(coded < 0.2 * uncoded, "turbo beats uncoded BPSK");
  const auto one_pass = comms::ber(info, comms::decode(code, pi, comms::llr(bpsk, rx, n0), 1)).mean().item<double>();
  expect(coded <= one_pass, "more iterations do not hurt");
  expect_error([&] { comms::encode(code, pi.slice(0, 0, 10), bits); }, "short interleaver accepted");
  expect_error([&] { comms::encode(comms::TurboCode{4, 013, 05}, pi, bits); }, "feedback without leading bit accepted");
  expect_error([&] { comms::decode(code, pi, soft.slice(-1, 0, 100), 2); }, "ragged turbo input accepted");
}

void timing_loop_checks() {
  const auto qpsk = comms::psk(4);
  const auto taps = comms::rrc(0.35, 4, 12);
  const auto symbols = comms::modulate(qpsk, random_bits(2 * 800, 91));
  for (double offset : {0.0, 1.7, -1.1}) {
    const auto late = comms::delay(comms::shape(symbols, taps, 4), pluss::series::make(std::vector<double>{offset}).select(0, 0));
    const auto filtered = comms::matched(late, taps);
    const auto got = comms::gardner(filtered, 4, 0.02);
    expect(got.symbols.size(-1) > 700 && got.offset.sizes() == got.symbols.sizes(), "timing result shapes");
    // the recovered stream is a shifted copy of the sent one: find the lag that matches
    double best = 1e9;
    for (int lag = 0; lag < 30; ++lag) {
      const int count = 500;
      const auto a = got.symbols.slice(-1, 200 + lag, 200 + lag + count);
      best = std::min(best, comms::evm(a, symbols.slice(-1, 200, 200 + count)).item<double>());
    }
    expect(best < 0.15, "gardner locks (offset " + std::to_string(offset) + ", evm " + std::to_string(best) + ")");
  }
  // the matched filter and match agree
  const auto wave = comms::shape(symbols, taps, 4);
  expect(comms::match(wave, taps, 4, 100).sub(comms::matched(wave, taps).slice(-1, 48, 48 + 400).slice(-1, 0, 400, 4)).abs().amax().item<double>() < 1e-12, "match is matched_filter sampled");
  expect_error([&] { comms::gardner(wave, 1, 0.02); }, "one sample per symbol accepted");
  expect_error([&] { comms::gardner(wave, 4, 0.0); }, "zero bandwidth accepted");
  expect_error([&] { comms::gardner(wave.slice(-1, 0, 20), 4, 0.02); }, "short waveform accepted");
}

void ofdm_checks() {
  const auto c = comms::qam(16);
  const std::int64_t k = 16, cp = 4, frames = 5;
  const auto bits = random_bits(4 * k * frames, 77);
  const auto sym = comms::modulate(c, bits);
  const auto tx = comms::to_tones(sym, k, cp);
  expect(tx.size(-1) == frames * (k + cp), "ofdm length");
  expect(comms::evm(comms::from_tones(tx, k, cp), sym).item<double>() < 1e-12, "ofdm round trip");
  const double e_sym = sym.abs().mul(sym.abs()).mean().item<double>();
  const auto body = tx.reshape({frames, k + cp}).slice(-1, cp, k + cp);
  expect(std::abs(body.abs().mul(body.abs()).mean().item<double>() - e_sym) < 1e-12, "unitary ofdm keeps the energy per sample");
  // a multipath channel no longer than the prefix becomes one complex gain per subcarrier
  const auto taps = pluss::series::complex(pluss::series::make(std::vector<double>{1.0, 0.0, 0.4, 0.0}), pluss::series::make(std::vector<double>{0.0, 0.3, 0.0, -0.2}), pluss::ScalarType::Complex128);
  const auto rx = pluss::subsp::eng::signal::conv(tx, taps).slice(-1, 0, tx.size(-1));
  const auto got = comms::from_tones(rx, k, cp).reshape({frames, k});
  const auto padded = pluss::series::complex(pluss::series::make(std::vector<double>{1, 0, 0.4, 0}).unite({pluss::series::zeros({k - 4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))}, 0),
                                              pluss::series::make(std::vector<double>{0, 0.3, 0, -0.2}).unite({pluss::series::zeros({k - 4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))}, 0), pluss::ScalarType::Complex128);
  const auto response = padded.fft(k, -1, "backward");
  const auto equalised = got.div(response);
  expect(comms::evm(equalised, sym.reshape({frames, k})).amax().item<double>() < 1e-12, "one-tap equalisation after a multipath channel");
  expect_error([&] { comms::to_tones(sym, 7, 1); }, "ragged ofdm blocks accepted");
  expect_error([&] { comms::to_tones(sym, k, k); }, "prefix as long as the block accepted");
  expect_error([&] { comms::from_tones(tx.slice(-1, 0, 19), k, cp); }, "ragged ofdm signal accepted");
}

} // namespace

int main() {
  constellation_checks();
  mapping_checks();
  llr_checks();
  pulse_checks();
  channel_and_metric_checks();
  ber_theory_checks();
  ofdm_checks();
  coding_checks();
  soft_guard_checks();
  ldpc_checks();
  turbo_checks();
  timing_loop_checks();
  sync_checks();
  std::cout << "Comms checks passed\n";
}
