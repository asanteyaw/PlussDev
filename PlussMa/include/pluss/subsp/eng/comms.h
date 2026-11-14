#pragma once

#include "pluss/codem/codem.h"

#include <cstdint>
#include <vector>

namespace pluss::subsp::eng::comms {

// A set of complex symbols with their bit labels. `points[i]` is the symbol sent for the integer label `i`, whose
// binary digits (most significant first) are the `bits` bits it carries. The average symbol energy is one.
struct Constellation {
  Series points;
  std::int64_t bits{};
  std::int64_t order{};
  bool phase_shift_keying{};
};

struct ConstellationOptions {
  bool gray{true};
  double phase{0.0};
  ScalarType dtype{ScalarType::Complex128};
};

// M-PSK for M a power of two: point k sits at angle 2 pi k / M + phase. Gray labelling makes neighbours differ by one bit.
Constellation psk(std::int64_t order, const ConstellationOptions &options = {});

// Square M-QAM for M a power of four: the high half of the label chooses the in-phase level and the low half the
// quadrature level, each Gray-coded when `gray` is set.
Constellation qam(std::int64_t order, const ConstellationOptions &options = {});

// Bits [..., N] (zeros and ones, N a multiple of `bits`) to symbols [..., N / bits].
Series modulate(const Constellation &constellation, const Series &bits);

// Labels [...] (integers in [0, order)) to symbols.
Series symbols(const Constellation &constellation, const Series &labels);

// Bit labels packed to and from bits: labels [..., N] to bits [..., N * bits_per_symbol] and back.
Series to_bits(const Series &labels, std::int64_t bits_per_symbol);
Series to_labels(const Series &bits, std::int64_t bits_per_symbol);

// Nearest-point decisions: the label of the closest constellation point to every received symbol, as Int64.
Series detect(const Constellation &constellation, const Series &received);

// Hard decisions as bits [..., N * bits], Int64.
Series demod(const Constellation &constellation, const Series &received);

// Max-log log-likelihood ratios log P(bit = 0) / P(bit = 1) for every bit, given the complex noise variance
// `noise_variance` = E|n|^2 per received symbol; [..., N * bits].
Series llr(const Constellation &constellation, const Series &received, double noise_variance);

// Unit-energy root-raised-cosine pulse of `span` symbols each side-to-side: span * samples_per_symbol + 1 taps.
Series rrc(double rolloff, std::int64_t samples_per_symbol, std::int64_t span, ScalarType dtype = ScalarType::Float64);

// Symbols [..., N] to a waveform [..., (N - 1) * samples_per_symbol + taps]: upsample then filter with `taps`.
Series shape(const Series &symbols, const Series &taps, std::int64_t samples_per_symbol);

// Matched filter and symbol-rate sampling: filters with the time-reversed conjugate taps and keeps `count` samples at
// the symbol instants. Undoes `shape` when `taps` is a Nyquist pulse pair such as `rrc`.
Series match(const Series &waveform, const Series &taps, std::int64_t samples_per_symbol, std::int64_t count);

// Adds white Gaussian noise of total variance `noise_variance` (complex signals split it equally between the real and
// imaginary parts). Reproducible from `seed`.
Series noise(const Series &signal, double noise_variance, std::uint64_t seed);

// N0 for unit symbol energy at Eb/N0 = `ebn0_db`: 1 / (bits * 10^(ebn0_db / 10)).
double n0(double ebn0_db, const Constellation &constellation);

// Fraction of differing bits over the last axis, [...].
Series ber(const Series &sent, const Series &received);

// Fraction of differing labels over the last axis, [...].
Series ser(const Series &sent, const Series &received);

// Root-mean-square error vector magnitude over the last axis, relative to the root-mean-square reference.
Series evm(const Series &received, const Series &reference);

// Bit error probability of Gray-coded coherent detection in white Gaussian noise at Eb/N0 [dB], elementwise: exact for
// BPSK and QPSK, the nearest-neighbour approximation otherwise.
Series theory(const Constellation &constellation, const Series &ebn0_db);

// OFDM: symbols [..., F * fft_size] are grouped into F blocks of `fft_size` subcarrier symbols, each turned into a time
// block by the unitary inverse FFT (so the energy per sample equals the energy per subcarrier symbol) and preceded by its
// last `cyclic_prefix` samples. Result [..., F * (fft_size + cyclic_prefix)].
Series to_tones(const Series &symbols, std::int64_t fft_size, std::int64_t cyclic_prefix);

// The inverse: drops each block's prefix and takes the unitary FFT. A channel whose impulse response is no longer than
// the prefix plus one multiplies subcarrier k by its frequency response H[k]. Result [..., F * fft_size].
Series from_tones(const Series &signal, std::int64_t fft_size, std::int64_t cyclic_prefix);

// Feed-forward convolutional code of rate 1 / generators.size(). `generators` are given in octal-style integers whose
// most significant of `constraint_length` bits multiplies the current input bit (the classic (7, 5) code of constraint
// length 3 is {07, 05}; the common rate-1/2 code of constraint length 7 is {0171, 0133}).
struct ConvolutionalCode {
  std::int64_t constraint_length{7};
  std::vector<std::int64_t> generators{0171, 0133};
};

// Encodes bits [..., N] and appends constraint_length - 1 zero tail bits so the encoder ends in the zero state. Result
// [..., (N + constraint_length - 1) * generators.size()], Int64, the outputs of the generators interleaved per input bit.
Series encode(const ConvolutionalCode &code, const Series &bits);

// Maximum-likelihood (Viterbi) decoding of a terminated codeword from soft values: `soft` [..., (N + K - 1) * n] holds a
// log-likelihood ratio per coded bit (positive favours zero, as `llr` returns; use +1 and -1 for hard decisions).
// Result: the N information bits [..., N], Int64.
Series decode(const ConvolutionalCode &code, const Series &soft);

// Fractional timing offset in samples of a pulse-shaped waveform [..., L] sampled `samples_per_symbol` (at least 4) times
// per symbol, by the Oerder-Meyr square-law estimator: the phase of the symbol-rate tone in |x|^2. Result [...] in
// (-samples_per_symbol / 2, samples_per_symbol / 2]; a waveform delayed by d samples gives d (modulo a symbol).
Series timing(const Series &waveform, std::int64_t samples_per_symbol);

// Delays a waveform [..., L] by `samples` (real, may be fractional or negative; [...] or a scalar) by a linear phase in
// the frequency domain. The shift is circular: pad the waveform if its ends matter.
Series delay(const Series &signal, const Series &samples);

// Carrier frequency offset in cycles per symbol of M-PSK symbols [..., N] (M = order: 2, 4 or 8) from the spectral peak
// of the M-th power, refined by parabolic interpolation. Result [...] in (-0.5 / order, 0.5 / order).
Series cfo(const Series &symbols, std::int64_t order);

// Removes a carrier offset: multiplies symbol n by exp(-j (2 pi cycles_per_symbol n + phase)); cycles_per_symbol and phase
// are [...] or scalars.
Series derot(const Series &symbols, const Series &cycles_per_symbol, const Series &phase);

struct CarrierResult {
  Series symbols;
  Series phase;
};

// Second-order phase-locked loop (Costas loop with an M-th power detector) for M-PSK symbols [..., N]: the corrected
// symbols and the tracked phase of every symbol. `loop_bandwidth` is the normalised noise bandwidth (about 0.005 to 0.05
// of the symbol rate); the damping is 1 / sqrt(2). It locks to a frequency offset within about its bandwidth and leaves
// the usual M-fold phase ambiguity: use a known preamble or differential coding to resolve it.
CarrierResult pll(const Series &symbols, std::int64_t order, double loop_bandwidth);

// ---- LDPC codes ----------------------------------------------------------------------------------------------------

// A binary low-density parity-check code: `parity_check` [m, n] (zeros and ones, Int64), a systematic `generator` [k, n]
// with generator * parity_check^T = 0 (mod 2), and the columns `info_positions` that carry the information bits.
struct LdpcCode {
  Series parity_check;
  Series generator;
  std::vector<std::int64_t> info_positions;
  std::int64_t length{};
  std::int64_t dimension{};
};

// A regular Gallager code: every column has `column_weight` ones and every row `row_weight`; `length` must be a multiple
// of `row_weight`. The parity-check matrix is reproducible from `seed`; the rate is at least 1 - column / row weight.
LdpcCode gallager(std::int64_t length, std::int64_t column_weight, std::int64_t row_weight, std::uint64_t seed);

// Systematic encoding: bits [..., k] to codewords [..., n], Int64.
Series encode(const LdpcCode &code, const Series &bits);

struct LdpcResult {
  Series bits;
  Series codeword;
  Series converged;
  Series iterations;
};

// Normalised min-sum belief propagation on log-likelihood ratios `soft` [..., n] (positive favours zero) for up to
// `max_iterations` rounds, stopping when every frame satisfies all checks. `scale` is the usual 0.75 attenuation of
// the check messages. Result: the decoded information bits [..., k], the hard-decision codeword [..., n], a Bool flag per
// frame saying whether every parity check holds, and the number of iterations used.
LdpcResult decode(const LdpcCode &code, const Series &soft, std::int64_t max_iterations = 50, double scale = 0.75);

// ---- Turbo codes ---------------------------------------------------------------------------------------------------

// Parallel concatenation of two identical recursive systematic convolutional encoders (rate about 1/3). `feedforward` and
// `feedback` are octal-style polynomials of `constraint_length` bits, most significant bit first; the common (13, 15)
// code of constraint length 4 is the default.
struct TurboCode {
  std::int64_t constraint_length{4};
  std::int64_t feedforward{013};
  std::int64_t feedback{015};
};

// A pseudo-random permutation of 0 .. length - 1 (Int64), reproducible from `seed`.
Series shuffle(std::int64_t length, std::uint64_t seed);

// Encodes bits [..., N] with an interleaver of length N: the output is [systematic N, parity 1 (N), parity 2 (N),
// then the termination of the first encoder: constraint_length - 1 systematic and as many parity bits], Int64.
Series encode(const TurboCode &code, const Series &interleaver, const Series &bits);

// Iterative max-log-MAP decoding of `soft` [..., 3 N + 2 (constraint_length - 1)] log-likelihood ratios in the layout of
// `encode` for `iterations` rounds. Result: the N information bits [..., N], Int64.
Series decode(const TurboCode &code, const Series &interleaver, const Series &soft, std::int64_t iterations = 8);

// ---- Symbol timing recovery ----------------------------------------------------------------------------------------

// Filters a waveform with the time-reversed conjugate taps (the matched filter) without sampling it; `match` is this followed
// by sampling at the symbol instants.
Series matched(const Series &waveform, const Series &taps);

struct TimingResult {
  Series symbols;
  Series offset;
};

// Gardner symbol-timing recovery on a matched-filtered waveform [..., L] with `samples_per_symbol` samples per symbol
// (at least 2): a second-order loop steers a sampling instant that is interpolated (cubic) from the waveform, using the
// Gardner error Re{conj(y(mid)) (y(n) - y(n - 1))}. `loop_bandwidth` is the normalised noise bandwidth (about 0.005 to
// 0.03). Result: the recovered symbols [..., count] and the sampling instant of each in samples [..., count]; `count`
// is as many symbols as the waveform holds.
TimingResult gardner(const Series &filtered, std::int64_t samples_per_symbol, double loop_bandwidth);

} // namespace pluss::subsp::eng::comms
