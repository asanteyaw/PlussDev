#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${1:-$ROOT_DIR/torch_refs}"
DEVICE="${2:-cpu}"
PY_BIN="${PY_BIN:-/Users/yawasante/software/miniconda3/envs/cpp_env/bin/python}"

mkdir -p "$OUT_DIR"

"$PY_BIN" "$ROOT_DIR/torch_linear_reference.py" --optimizer adam --epochs 32 --device "$DEVICE" --out "$OUT_DIR/linear_adam.csv"
"$PY_BIN" "$ROOT_DIR/torch_linear_reference.py" --optimizer adamw --epochs 24 --device "$DEVICE" --out "$OUT_DIR/linear_adamw.csv"
"$PY_BIN" "$ROOT_DIR/torch_linear_reference.py" --optimizer sgd --epochs 24 --device "$DEVICE" --out "$OUT_DIR/linear_sgd.csv"
"$PY_BIN" "$ROOT_DIR/torch_linear_reference.py" --optimizer rmsprop --epochs 24 --device "$DEVICE" --out "$OUT_DIR/linear_rmsprop.csv"
"$PY_BIN" "$ROOT_DIR/torch_linear_reference.py" --optimizer adagrad --epochs 24 --device "$DEVICE" --out "$OUT_DIR/linear_adagrad.csv"
"$PY_BIN" "$ROOT_DIR/torch_linear_reference.py" --optimizer nadam --epochs 24 --device "$DEVICE" --out "$OUT_DIR/linear_nadam.csv"
"$PY_BIN" "$ROOT_DIR/torch_lstm_reference.py" --epochs 36 --device "$DEVICE" --out "$OUT_DIR/lstm.csv"
"$PY_BIN" "$ROOT_DIR/torch_rnn_reference.py" --epochs 34 --device "$DEVICE" --out "$OUT_DIR/rnn.csv"
"$PY_BIN" "$ROOT_DIR/torch_gru_reference.py" --epochs 34 --device "$DEVICE" --out "$OUT_DIR/gru.csv"
"$PY_BIN" "$ROOT_DIR/torch_schedule_reference.py" --epochs 30 --device "$DEVICE" --out "$OUT_DIR/schedule.csv"
"$PY_BIN" "$ROOT_DIR/torch_conv_reference.py" --epochs 28 --device "$DEVICE" --out "$OUT_DIR/conv.csv"
"$PY_BIN" "$ROOT_DIR/torch_transformer_reference.py" --epochs 20 --device "$DEVICE" --out "$OUT_DIR/transformer.csv"
"$PY_BIN" "$ROOT_DIR/torch_objective_reference.py" --epochs 30 --device "$DEVICE" --out "$OUT_DIR/objective.csv"

cat <<EOF
Generated Torch references in: $OUT_DIR (device=$DEVICE)

export PLUSS_TORCH_LINEAR_REF_CSV="$OUT_DIR/linear_adam.csv"
export PLUSS_TORCH_LINEAR_ADAMW_REF_CSV="$OUT_DIR/linear_adamw.csv"
export PLUSS_TORCH_LINEAR_SGD_REF_CSV="$OUT_DIR/linear_sgd.csv"
export PLUSS_TORCH_LINEAR_RMSPROP_REF_CSV="$OUT_DIR/linear_rmsprop.csv"
export PLUSS_TORCH_LINEAR_ADAGRAD_REF_CSV="$OUT_DIR/linear_adagrad.csv"
export PLUSS_TORCH_LINEAR_NADAM_REF_CSV="$OUT_DIR/linear_nadam.csv"
export PLUSS_TORCH_LSTM_REF_CSV="$OUT_DIR/lstm.csv"
export PLUSS_TORCH_RNN_REF_CSV="$OUT_DIR/rnn.csv"
export PLUSS_TORCH_GRU_REF_CSV="$OUT_DIR/gru.csv"
export PLUSS_TORCH_SCHEDULE_REF_CSV="$OUT_DIR/schedule.csv"
export PLUSS_TORCH_CONV_REF_CSV="$OUT_DIR/conv.csv"
export PLUSS_TORCH_TRANSFORMER_REF_CSV="$OUT_DIR/transformer.csv"
export PLUSS_TORCH_OBJECTIVE_REF_CSV="$OUT_DIR/objective.csv"
EOF

case "$DEVICE" in
  mps)
    cat <<EOF
export PLUSS_TORCH_MPS_LINEAR_REF_CSV="$OUT_DIR/linear_adam.csv"
export PLUSS_TORCH_MPS_LINEAR_ADAMW_REF_CSV="$OUT_DIR/linear_adamw.csv"
export PLUSS_TORCH_MPS_LINEAR_SGD_REF_CSV="$OUT_DIR/linear_sgd.csv"
export PLUSS_TORCH_MPS_LINEAR_RMSPROP_REF_CSV="$OUT_DIR/linear_rmsprop.csv"
export PLUSS_TORCH_MPS_LINEAR_ADAGRAD_REF_CSV="$OUT_DIR/linear_adagrad.csv"
export PLUSS_TORCH_MPS_LINEAR_NADAM_REF_CSV="$OUT_DIR/linear_nadam.csv"
export PLUSS_TORCH_MPS_LSTM_REF_CSV="$OUT_DIR/lstm.csv"
export PLUSS_TORCH_MPS_RNN_REF_CSV="$OUT_DIR/rnn.csv"
export PLUSS_TORCH_MPS_GRU_REF_CSV="$OUT_DIR/gru.csv"
export PLUSS_TORCH_MPS_SCHEDULE_REF_CSV="$OUT_DIR/schedule.csv"
export PLUSS_TORCH_MPS_CONV_REF_CSV="$OUT_DIR/conv.csv"
export PLUSS_TORCH_MPS_TRANSFORMER_REF_CSV="$OUT_DIR/transformer.csv"
export PLUSS_TORCH_MPS_OBJECTIVE_REF_CSV="$OUT_DIR/objective.csv"
EOF
    ;;
  cuda)
    cat <<EOF
export PLUSS_TORCH_CUDA_LINEAR_REF_CSV="$OUT_DIR/linear_adam.csv"
export PLUSS_TORCH_CUDA_LINEAR_ADAMW_REF_CSV="$OUT_DIR/linear_adamw.csv"
export PLUSS_TORCH_CUDA_LINEAR_SGD_REF_CSV="$OUT_DIR/linear_sgd.csv"
export PLUSS_TORCH_CUDA_LINEAR_RMSPROP_REF_CSV="$OUT_DIR/linear_rmsprop.csv"
export PLUSS_TORCH_CUDA_LINEAR_ADAGRAD_REF_CSV="$OUT_DIR/linear_adagrad.csv"
export PLUSS_TORCH_CUDA_LINEAR_NADAM_REF_CSV="$OUT_DIR/linear_nadam.csv"
export PLUSS_TORCH_CUDA_LSTM_REF_CSV="$OUT_DIR/lstm.csv"
export PLUSS_TORCH_CUDA_RNN_REF_CSV="$OUT_DIR/rnn.csv"
export PLUSS_TORCH_CUDA_GRU_REF_CSV="$OUT_DIR/gru.csv"
export PLUSS_TORCH_CUDA_SCHEDULE_REF_CSV="$OUT_DIR/schedule.csv"
export PLUSS_TORCH_CUDA_CONV_REF_CSV="$OUT_DIR/conv.csv"
export PLUSS_TORCH_CUDA_TRANSFORMER_REF_CSV="$OUT_DIR/transformer.csv"
export PLUSS_TORCH_CUDA_OBJECTIVE_REF_CSV="$OUT_DIR/objective.csv"
EOF
    ;;
esac
