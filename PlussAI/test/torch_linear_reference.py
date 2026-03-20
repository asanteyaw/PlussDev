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


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate deterministic Torch loss trajectory CSV for Pluss parity test.")
    parser.add_argument("--out", type=Path, required=True, help="Output CSV path.")
    parser.add_argument("--epochs", type=int, default=32)
    parser.add_argument(
        "--optimizer",
        type=str,
        default="adam",
        choices=["adam", "adamw", "sgd", "rmsprop", "adagrad", "nadam"],
        help="Optimizer family for the reference trace.",
    )
    args = parser.parse_args()

    torch.manual_seed(20260318)
    torch.set_num_threads(1)

    x, y = make_data()
    model = torch.nn.Linear(2, 1, bias=True, dtype=torch.float32)
    with torch.no_grad():
        model.weight.copy_(torch.tensor([[0.10, -0.20]], dtype=torch.float32))
        model.bias.copy_(torch.tensor([0.05], dtype=torch.float32))

    if args.optimizer == "adam":
        optimizer = torch.optim.Adam(model.parameters(), lr=5e-2)
    elif args.optimizer == "adamw":
        optimizer = torch.optim.AdamW(model.parameters(), lr=5e-2, weight_decay=1e-2)
    elif args.optimizer == "sgd":
        optimizer = torch.optim.SGD(model.parameters(), lr=1e-1, momentum=0.9, nesterov=True)
    elif args.optimizer == "rmsprop":
        optimizer = torch.optim.RMSprop(model.parameters(), lr=1e-2, momentum=0.5, alpha=0.99, eps=1e-8)
    elif args.optimizer == "adagrad":
        optimizer = torch.optim.Adagrad(model.parameters(), lr=5e-2, eps=1e-10)
    elif args.optimizer == "nadam":
        optimizer = torch.optim.NAdam(model.parameters(), lr=5e-2)
    else:
        raise RuntimeError(f"unsupported optimizer '{args.optimizer}'")
    criterion = torch.nn.MSELoss(reduction="mean")

    losses: list[float] = []
    for _ in range(args.epochs):
        optimizer.zero_grad(set_to_none=False)
        pred = model(x)
        loss = criterion(pred, y)
        loss.backward()
        optimizer.step()
        losses.append(float(loss.detach().cpu().item()))

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["epoch", "loss"])
        for idx, loss in enumerate(losses, 1):
            writer.writerow([idx, f"{loss:.12g}"])

    print(f"wrote {len(losses)} rows to {args.out}")


if __name__ == "__main__":
    main()
