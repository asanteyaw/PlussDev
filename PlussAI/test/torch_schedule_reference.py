#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path

import torch


def make_data() -> tuple[torch.Tensor, torch.Tensor]:
    n = 256
    x = torch.empty((n, 2), dtype=torch.float32)
    y = torch.empty((n, 1), dtype=torch.float32)
    for i in range(n):
        x0 = float((i % 17) - 8) / 8.0
        x1 = float(((i * 7) % 19) - 9) / 9.0
        x[i, 0] = x0
        x[i, 1] = x1
        y[i, 0] = (1.75 * x0) - (2.25 * x1) + 0.35
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


class ScheduleParityRegressor(torch.nn.Module):
    def __init__(self) -> None:
        super().__init__()
        torch.manual_seed(41)
        self.encoder = torch.nn.Linear(2, 12, bias=True)
        self.activation = torch.nn.Tanh()
        torch.manual_seed(43)
        self.head = torch.nn.Linear(12, 1, bias=True)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.head(self.activation(self.encoder(x)))


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate deterministic Torch schedule-like loss trajectory CSV for Pluss parity test."
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
    model = ScheduleParityRegressor().to(device=device, dtype=torch.float32)
    model.train(True)

    optimizer = torch.optim.Adam(model.parameters(), lr=2e-2)
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
