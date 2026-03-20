#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path

import torch


def make_sequence_data() -> tuple[torch.Tensor, torch.Tensor]:
    batch = 24
    steps = 20
    seq = torch.empty((batch, steps, 1), dtype=torch.float32)
    for b in range(batch):
        for t in range(steps):
            x = (0.15 * float(t)) + (0.07 * float(b))
            seq[b, t, 0] = torch.sin(torch.tensor(x, dtype=torch.float32)) + (
                0.1 * torch.cos(torch.tensor(2.0 * x, dtype=torch.float32))
            )
    features = seq[:, :19, :]
    target = seq[:, 1:, :]
    return features, target


class TinyLSTMParityRegressor(torch.nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.lstm = torch.nn.LSTM(
            input_size=1,
            hidden_size=12,
            num_layers=1,
            bias=True,
            batch_first=True,
            dropout=0.0,
            bidirectional=False,
        )
        self.head = torch.nn.Linear(12, 1, bias=True)

        with torch.no_grad():
            for p in self.parameters():
                p.fill_(0.02)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        seq, _ = self.lstm(x)
        return self.head(seq)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate deterministic Torch LSTM loss trajectory CSV for Pluss parity test."
    )
    parser.add_argument("--out", type=Path, required=True, help="Output CSV path.")
    parser.add_argument("--epochs", type=int, default=36)
    args = parser.parse_args()

    torch.manual_seed(20260318)
    torch.set_num_threads(1)

    x, y = make_sequence_data()
    model = TinyLSTMParityRegressor().to(dtype=torch.float32)
    model.train(True)

    optimizer = torch.optim.Adam(model.parameters(), lr=1e-2)
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
