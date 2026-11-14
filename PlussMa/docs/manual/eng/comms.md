<!-- generated from comms.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Communications

A digital link maps bits to symbols, shapes them into a waveform, passes them through a noisy channel and decides what was sent. The functions work on complete blocks of data and are vectorized over frames.

1. **How do bits become symbols and back, and how well does a link perform?** PSK and square QAM constellations with Gray labelling, hard decisions and soft likelihood ratios, noise at a chosen Eb/N0, and the bit error rate against theory.

2. **How are symbols put on a band-limited waveform?** Root-raised-cosine pulse shaping and matched filtering, and the whole chain checked against theory.

3. **How does a link survive multipath, noise and an unknown clock?** OFDM with a cyclic prefix, which turns a multipath channel into one gain per subcarrier; convolutional coding with soft Viterbi decoding; and timing and carrier recovery.

Chapters, in reading order:

- [Communications 1: Modulation and Detection](comms/comms_01_modulation.md)

Every chapter follows the same format: a mental model, complete programs, and output produced by those programs.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

comms/comms_01_modulation
```
<!-- apiref:toctree:end -->
