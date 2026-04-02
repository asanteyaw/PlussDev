#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path

import torch


def make_data() -> tuple[torch.Tensor, torch.Tensor]:
    batch = 24
    length = 15
    x = torch.empty((batch, 1, length), dtype=torch.float32)
    for b in range(batch):
        for t in range(length):
            z = (0.17 * float(t)) + (0.05 * float(b))
            x[b, 0, t] = torch.sin(torch.tensor(z, dtype=torch.float32)) + (
                0.2 * torch.cos(torch.tensor(2.0 * z, dtype=torch.float32))
            )
    kernel = torch.tensor([[[0.25, -0.1, 0.35]]], dtype=torch.float32)
    bias = torch.tensor([0.05], dtype=torch.float32)
    y = torch.nn.functional.conv1d(x, kernel, bias=bias, stride=1, padding=1, dilation=1, groups=1)
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
        description="Generate deterministic Torch Conv1d loss trajectory CSV for Pluss parity test."
    )
    parser.add_argument("--out", type=Path, required=True, help="Output CSV path.")
    parser.add_argument("--epochs", type=int, default=28)
    parser.add_argument("--device", type=str, default="cpu", choices=["cpu", "mps", "cuda"])
    args = parser.parse_args()

    torch.manual_seed(20260318)
    torch.set_num_threads(1)
    device = resolve_device(args.device)

    x, y = make_data()
    x = x.to(device)
    y = y.to(device)
    model = torch.nn.Conv1d(
        in_channels=1,
        out_channels=1,
        kernel_size=3,
        stride=1,
        padding=1,
        dilation=1,
        groups=1,
        bias=True,
    ).to(device=device, dtype=torch.float32)
    model.train(True)
    with torch.no_grad():
        for p in model.parameters():
            p.fill_(0.02)

    optimizer = torch.optim.Adam(model.parameters(), lr=1e-2)
    criterion = torch.nn.MSELoss(reduction="mean")

    losses: list[float] = []
    for _ in range(args.epochs):
        optimizer.zero_grad(set_to_none=False)
        pred = model(x)
        loss = criterion(pred, y)
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
