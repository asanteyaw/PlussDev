#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path

import torch


def make_data() -> tuple[torch.Tensor, torch.Tensor]:
    n = 16
    x = torch.empty((n, 3), dtype=torch.float32)
    y = torch.empty((n, 1), dtype=torch.float32)
    for i in range(n):
        a = float((i % 7) - 3) / 3.0
        b = float(((i * 3) % 11) - 5) / 4.0
        c = float(((i * 5) % 13) - 6) / 5.0
        x[i, 0] = a
        x[i, 1] = b
        x[i, 2] = c
        y[i, 0] = (0.7 * a) - (1.1 * b) + (0.4 * c) + 0.2
    return x, y


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
        description="Generate deterministic Torch custom-objective loss trajectory CSV for Pluss parity test."
    )
    parser.add_argument("--out", type=Path, required=True, help="Output CSV path.")
    parser.add_argument("--epochs", type=int, default=30)
    parser.add_argument("--device", type=str, default="cpu", choices=["cpu", "mps", "cuda"])
    args = parser.parse_args()

    torch.manual_seed(20260318)
    torch.set_num_threads(1)
    device = resolve_device(args.device)

    x, y = make_data()
    x = x.to(device)
    y = y.to(device)
    w = torch.nn.Parameter(torch.tensor([0.2, -0.1, 0.05], dtype=torch.float32, device=device))
    optimizer = torch.optim.Adam([w], lr=2e-2)
    idx = torch.tensor([2, 0], dtype=torch.int64, device=device)

    losses: list[float] = []
    for _ in range(args.epochs):
        optimizer.zero_grad(set_to_none=False)
        pred = x.matmul(w.view(3, 1)).reshape(x.shape[0], 1)
        resid = pred - y
        mse = (resid.square()).mean()
        reg1 = (torch.sin(w).square()).mean() * 0.10
        reg2 = w.abs().mean() * 0.05
        reg3 = (w.index_select(0, idx).square()).sum() * 0.01
        loss = mse + reg1 + reg2 + reg3
        loss.backward()
        optimizer.step()
        losses.append(float(loss.detach().to("cpu").item()))

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["epoch", "loss"])
        for idx_epoch, loss in enumerate(losses, 1):
            writer.writerow([idx_epoch, f"{loss:.12g}"])

    print(f"wrote {len(losses)} rows to {args.out}")


if __name__ == "__main__":
    main()
