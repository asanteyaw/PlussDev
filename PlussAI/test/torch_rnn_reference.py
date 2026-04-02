#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path

import torch


def make_sequence_data() -> tuple[torch.Tensor, torch.Tensor]:
    batch = 20
    steps = 18
    seq = torch.empty((batch, steps, 1), dtype=torch.float32)
    for b in range(batch):
        for t in range(steps):
            x = (0.11 * float(t)) + (0.09 * float(b))
            seq[b, t, 0] = torch.sin(torch.tensor(x, dtype=torch.float32)) + (
                0.05 * torch.cos(torch.tensor(3.0 * x, dtype=torch.float32))
            )
    features = seq[:, :17, :]
    target = seq[:, 1:, :]
    return features, target


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


class TinyRNNParityRegressor(torch.nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.rnn = torch.nn.RNN(
            input_size=1,
            hidden_size=10,
            num_layers=1,
            nonlinearity="tanh",
            bias=True,
            batch_first=True,
            dropout=0.0,
            bidirectional=False,
        )
        self.head = torch.nn.Linear(10, 1, bias=True)

        with torch.no_grad():
            for p in self.parameters():
                p.fill_(0.02)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        seq, _ = self.rnn(x)
        return self.head(seq)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate deterministic Torch RNN loss trajectory CSV for Pluss parity test."
    )
    parser.add_argument("--out", type=Path, required=True, help="Output CSV path.")
    parser.add_argument("--epochs", type=int, default=34)
    parser.add_argument("--device", type=str, default="cpu", choices=["cpu", "mps", "cuda"])
    args = parser.parse_args()

    torch.manual_seed(20260318)
    torch.set_num_threads(1)
    device = resolve_device(args.device)

    x, y = make_sequence_data()
    x = x.to(device)
    y = y.to(device)
    model = TinyRNNParityRegressor().to(device=device, dtype=torch.float32)
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
