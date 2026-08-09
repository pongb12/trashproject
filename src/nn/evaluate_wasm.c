// WASM-only NNUE evaluation — scalar path only
// This file replaces evaluate.c for WASM builds to avoid clang crash
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "evaluate.h"
#include "../board.h"
#include "../bits.h"
#include "accumulator.h"

// Quantization
#define QUANT1_BITS 5
#define QUANT2_BITS 12

// Network weights — loaded at runtime from .nn file
int16_t INPUT_WEIGHTS[N_FEATURES * N_HIDDEN] __attribute__((aligned(16)));
int16_t INPUT_BIASES[N_HIDDEN] __attribute__((aligned(16)));
int8_t L1_WEIGHTS[N_L1 * N_L2] __attribute__((aligned(16)));
int32_t L1_BIASES[N_L2] __attribute__((aligned(16)));
int16_t L2_WEIGHTS[N_L2 * N_L3] __attribute__((aligned(16)));
int32_t L2_BIASES[N_L3] __attribute__((aligned(16)));
int16_t OUTPUT_WEIGHTS[N_L3 * N_OUTPUT] __attribute__((aligned(16)));
int32_t OUTPUT_BIAS;

uint16_t LOOKUP_INDICES[256][8] __attribute__((aligned(16)));

// Simple scalar NNUE inference
int Propagate(Accumulator* acc, const int stm) {
  // Simplified — just use accumulator values
  int16_t* output = stm == WHITE ? acc->values[WHITE] : acc->values[BLACK];
  
  // L1: input (1024) → L2 (16) — simplified dot product
  int32_t l2_out[N_L2];
  for (int i = 0; i < N_L2; i++) {
    int32_t sum = L1_BIASES[i];
    for (int j = 0; j < N_L1; j++) {
      sum += output[j] * L1_WEIGHTS[i * N_L1 + j];
    }
    l2_out[i] = sum;
  }
  
  // ReLU
  for (int i = 0; i < N_L2; i++) {
    if (l2_out[i] < 0) l2_out[i] = 0;
  }
  
  // L2: (16) → L3 (32) — simplified
  int32_t l3_out[N_L3];
  for (int i = 0; i < N_L3; i++) {
    int32_t sum = L2_BIASES[i];
    for (int j = 0; j < N_L2; j++) {
      sum += l2_out[j] * L2_WEIGHTS[i * N_L2 + j];
    }
    l3_out[i] = sum;
  }
  
  // ReLU
  for (int i = 0; i < N_L3; i++) {
    if (l3_out[i] < 0) l3_out[i] = 0;
  }
  
  // Output: (32) → 1
  int32_t out = OUTPUT_BIAS;
  for (int i = 0; i < N_L3; i++) {
    out += l3_out[i] * OUTPUT_WEIGHTS[i];
  }
  
  return out;
}

int Predict(Board* board) {
  ResetAccumulator(board->accumulators, board, WHITE);
  ResetAccumulator(board->accumulators, board, BLACK);

  return board->stm == WHITE ? Propagate(board->accumulators, WHITE) : Propagate(board->accumulators, BLACK);
}

void LoadDefaultNN() {
  // Will be loaded via LoadNetwork
}

int LoadNetwork(char* path) {
  FILE* f = fopen(path, "rb");
  if (!f) return 0;
  
  // Read network weights
  fread(INPUT_WEIGHTS, sizeof(int16_t), N_FEATURES * N_HIDDEN, f);
  fread(INPUT_BIASES, sizeof(int16_t), N_HIDDEN, f);
  fread(L1_WEIGHTS, sizeof(int8_t), N_L1 * N_L2, f);
  fread(L1_BIASES, sizeof(int32_t), N_L2, f);
  fread(L2_WEIGHTS, sizeof(int16_t), N_L2 * N_L3, f);
  fread(L2_BIASES, sizeof(int32_t), N_L3, f);
  fread(OUTPUT_WEIGHTS, sizeof(int16_t), N_L3 * N_OUTPUT, f);
  fread(&OUTPUT_BIAS, sizeof(int32_t), 1, f);
  
  fclose(f);
  return 1;
}
