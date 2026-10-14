# NodeV and NodeA Implementation Plan

Status: accepted architecture; implementation not yet started.

## Decision

Computer vision and audio should not be placed wholly inside either NodeK or
Ice. They are domain families with their own input formats, transformations,
datasets, evaluation measures, and model catalogs. NodeK supplies their
differentiable foundations, while Ice supplies domain-independent training and
evaluation orchestration.

The implementation modules are:

- **NodeV**, exposed publicly as `pluss::ai::vision`
- **NodeA**, exposed publicly as `pluss::ai::audio`

`NodeV` and `NodeA` are repository and implementation names. They must not leak
into ordinary user code or public diagnostics.

## Ownership Boundaries

| Layer | Responsibility |
|---|---|
| `Series` | Numeric storage, devices, dtypes, shape operations, autodiff graph, and general mathematical primitives. |
| NodeK | Differentiable foundations shared across domains: convolution, pooling, interpolation, FFT/STFT primitives, tensor resampling, padding, normalization, and accelerated backend dispatch. |
| NodeV | Image and video codecs, datasets, transforms, augmentations, vision metrics, pretrained weights, and ready-made vision architectures. |
| NodeA | Waveform codecs, audio datasets, resampling, spectrograms, mel filter banks, MFCCs, augmentations, audio metrics, pretrained weights, and ready-made audio architectures. |
| Ice | Generic training, validation, inference, callbacks, checkpointing, progress reporting, batching integration, and deployment workflows for models from every domain. |

Ice must not decode JPEG files, parse media containers, read waveform formats,
or implement spectrogram algorithms. Conversely, NodeV and NodeA must not grow
their own optimizer loops, checkpoint engines, callback systems, or progress
reporters. A vision or audio model should use the same Ice machinery as an NLP,
forecasting, reinforcement-learning, or user-defined NodeK model.

## Public Surface

The domain entry points are independent public headers and namespaces:

```cpp
#include <pluss/ai/vision.h>
#include <pluss/ai/audio.h>
#include <pluss/ai/ice.h>

auto image = pluss::ai::vision::load("cat.jpg");
auto model = pluss::ai::vision::models::resnet18(...);
model.learn(dataset, ice_options);

auto waveform = pluss::ai::audio::load("speech.wav");
auto mel = pluss::ai::audio::mel_spectrogram(waveform, ...);
auto recognizer = pluss::ai::audio::models::conformer(...);
recognizer.learn(dataset, ice_options);
```

Factories, options, result types, and diagnostics follow the public namespace,
not the internal module name. For example, diagnostics say
`vision::load: unsupported image encoding`, never `NodeV::load`.

## Dependency Direction

```text
Series
  |
  v
NodeK
  |
  v
NodeV / NodeA
  |
  v
Ice training orchestration
```

This diagram describes capability layering, not ownership inversion. Ice may
consume a NodeV or NodeA dataset/model adapter, but NodeV and NodeA must expose
their domain objects without requiring Ice. Users must remain able to run the
same model directly through NodeK when they need a custom loop.

The dependency graph must therefore avoid cycles:

- NodeK does not depend on NodeV, NodeA, or Ice.
- NodeV and NodeA depend on `Series` and NodeK, but not on each other.
- Shared media-neutral utilities belong below the domain modules rather than
  creating a NodeV-to-NodeA or NodeA-to-NodeV dependency.
- Ice integrates with public domain contracts through adapters; domain kernels
  do not call Ice.

## Placement Rules

Use the following test when deciding where a new operation belongs:

1. If it is useful for arbitrary differentiable tensors, it belongs in
   `Series` or NodeK. Examples include convolution, padding, interpolation,
   FFT/STFT kernels, and differentiable resampling.
2. If it understands pixels, color spaces, image/video encodings, bounding
   boxes, masks, or vision-specific evaluation, it belongs in NodeV.
3. If it understands sample rates, channels, waveform encodings, spectrogram
   conventions, acoustic features, or audio-specific evaluation, it belongs
   in NodeA.
4. If it coordinates epochs, optimizers, callbacks, checkpoints, metrics,
   logging, or deployment, it belongs in Ice.

Domain transforms may compose NodeK primitives while remaining owned by their
domain. For example, NodeK owns differentiable interpolation; NodeV owns a
random resized crop that chooses image-space parameters and then invokes that
interpolation. NodeK owns FFT/STFT math; NodeA owns mel-scale projection and
audio-specific defaults.

## Existing Foundation

NodeK already provides part of the required vision foundation through its
convolution modules and accelerated `conv2d`/`conv3d` kernels, exposed through
[`include/pluss/ai/nn.h`](include/pluss/ai/nn.h). Those kernels remain in
NodeK; NodeV builds image-domain transforms and architectures over them.

Ice is already defined as a domain-independent convenience and training layer
over NodeK. Its architecture explicitly requires the core trainer to remain as
usable for image models as it is for text, forecasting, and other domains; see
[`Ice/ICE_BLUEPRINT.md`](Ice/ICE_BLUEPRINT.md).

## Initial Implementation Order

1. Create optional NodeV and NodeA CMake targets, public umbrella headers, and
   namespace-level smoke tests without adding codec dependencies to the base
   PlussAI target.
2. Complete and test the missing NodeK primitives required by both domains,
   including backward rules and CPU/backend parity.
3. Establish canonical media value contracts: image layout, color metadata,
   waveform layout, sample-rate metadata, dtype conversion, device transfer,
   and batch shape.
4. Add CPU-first codecs and deterministic transformations, keeping optional
   third-party codec support isolated behind build features.
5. Add datasets, collation, augmentation policies, and domain metrics.
6. Add small reference architectures before large catalogs: one classifier
   and detector path for NodeV, and one classifier and spectrogram-based model
   for NodeA.
7. Integrate those models with Ice through shared dataset/model adapters and
   prove that direct NodeK loops remain possible.
8. Add pretrained-weight manifests, versioning, checksum validation, examples,
   benchmarks, and complete API references only after the underlying contracts
   are stable.

Correctness and a stable ownership boundary take precedence over catalog size.
The first releases should prefer a few end-to-end, verified workflows over a
wide surface of thin wrappers.
