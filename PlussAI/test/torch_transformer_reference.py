#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path

import torch


def make_data() -> tuple[torch.Tensor, torch.Tensor]:
    batch = 8
    seq = 6
    d_model = 8
    x = torch.empty((batch, seq, d_model), dtype=torch.float32)
    for b in range(batch):
        for t in range(seq):
            for d in range(d_model):
                z = (0.09 * float(t)) + (0.07 * float(d)) + (0.03 * float(b))
                x[b, t, d] = torch.sin(torch.tensor(z, dtype=torch.float32)) + (
                    0.15 * torch.cos(torch.tensor(1.7 * z, dtype=torch.float32))
                )
    return x, x


def resolve_device(name: str) -> torch.device:
    if name == "cpu":
        return torch.device("cpu")
    if name == "mps":
        if not torch.backends.mps.is_available():
            raise RuntimeError("requested --device mps but torch MPS backend is unavailable")
        return torch.device("mps")
    if name == "cuda":
        if not torch.cuda.is_available():
            raise RuntimeError("requested --device cuda but torch CUDA backend is unavailable")
        return torch.device("cuda")
    raise RuntimeError(f"unsupported device '{name}'")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate deterministic Torch Transformer loss trajectory CSV for Pluss parity test."
    )
    parser.add_argument("--out", type=Path, required=True, help="Output CSV path.")
    parser.add_argument("--epochs", type=int, default=20)
    parser.add_argument("--device", type=str, default="cpu", choices=["cpu", "mps", "cuda"])
    args = parser.parse_args()

    torch.manual_seed(20260318)
    torch.set_num_threads(1)
    device = resolve_device(args.device)

    src, tgt = make_data()
    src = src.to(device)
    tgt = tgt.to(device)
    model = torch.nn.Transformer(
        d_model=8,
        nhead=2,
        num_encoder_layers=1,
        num_decoder_layers=1,
        dim_feedforward=16,
        dropout=0.0,
        activation="relu",
        custom_encoder=None,
        custom_decoder=None,
        layer_norm_eps=1e-5,
        batch_first=True,
        norm_first=False,
        bias=True,
    ).to(device=device, dtype=torch.float32)
    model.train(True)
    with torch.no_grad():
        for p in model.parameters():
            p.fill_(0.02)

    optimizer = torch.optim.Adam(model.parameters(), lr=5e-3)
    criterion = torch.nn.MSELoss(reduction="mean")

    losses: list[float] = []
    for _ in range(args.epochs):
        optimizer.zero_grad(set_to_none=False)
        pred = model(src, tgt)
        loss = criterion(pred, tgt)
        loss.backward()
        optimizer.step()
        losses.append(float(loss.detach().to("cpu").item()))

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["epoch", "loss"])
        for idx, loss in enumerate(losses, 1):
            writer.writerow([idx, f"{loss:.12g}"])

    print(f"wrote {len(losses)} rows to {args.out}")


if __name__ == "__main__":
    main()
