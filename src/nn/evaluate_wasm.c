// WASM-only NNUE evaluation — scalar path only
// Uses proper CopyData from original evaluate.c to load network weights
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "evaluate.h"
#include "../board.h"
#include "../bits.h"
#include "accumulator.h"

// Quantization
#define QUANT1_BITS 5
#define QUANT2_BITS 12

// Network weights — defined here, used by accumulator.c via extern in accumulator.h
int16_t INPUT_WEIGHTS[N_FEATURES * N_HIDDEN] __attribute__((aligned(16)));
int16_t INPUT_BIASES[N_HIDDEN] __attribute__((aligned(16)));
int8_t L1_WEIGHTS[N_L1 * N_L2] __attribute__((aligned(16)));
int32_t L1_BIASES[N_L2] __attribute__((aligned(16)));
int16_t L2_WEIGHTS[N_L2 * N_L3] __attribute__((aligned(16)));
int32_t L2_BIASES[N_L3] __attribute__((aligned(16)));
int16_t OUTPUT_WEIGHTS[N_L3 * N_OUTPUT] __attribute__((aligned(16)));
int32_t OUTPUT_BIAS;

uint16_t LOOKUP_INDICES[256][8] __attribute__((aligned(16)));

// Network size (matches original evaluate.c NETWORK_SIZE)
static const size_t NETWORK_SIZE = N_FEATURES * N_HIDDEN * sizeof(int16_t) +
                                   N_HIDDEN * sizeof(int16_t) +
                                   N_L1 * N_L2 * sizeof(int8_t) +
                                   N_L2 * sizeof(int32_t) +
                                   N_L2 * N_L3 * sizeof(int16_t) +
                                   N_L3 * sizeof(int32_t) +
                                   N_L3 * N_OUTPUT * sizeof(int16_t) +
                                   sizeof(int32_t);

// Copy network data from raw bytes into weight arrays
static void CopyData(const unsigned char* in) {
  size_t offset = 0;

  memcpy(INPUT_WEIGHTS, &in[offset], N_FEATURES * N_HIDDEN * sizeof(int16_t));
  offset += N_FEATURES * N_HIDDEN * sizeof(int16_t);
  memcpy(INPUT_BIASES, &in[offset], N_HIDDEN * sizeof(int16_t));
  offset += N_HIDDEN * sizeof(int16_t);

  memcpy(L1_WEIGHTS, &in[offset], N_L1 * N_L2 * sizeof(int8_t));
  offset += N_L1 * N_L2 * sizeof(int8_t);
  memcpy(L1_BIASES, &in[offset], N_L2 * sizeof(int32_t));
  offset += N_L2 * sizeof(int32_t);

  memcpy(L2_WEIGHTS, &in[offset], N_L2 * N_L3 * sizeof(int16_t));
  offset += N_L2 * N_L3 * sizeof(int16_t);
  memcpy(L2_BIASES, &in[offset], N_L3 * sizeof(int32_t));
  offset += N_L3 * sizeof(int32_t);

  memcpy(OUTPUT_WEIGHTS, &in[offset], N_L3 * N_OUTPUT * sizeof(int16_t));
  offset += N_L3 * N_OUTPUT * sizeof(int16_t);
  memcpy(&OUTPUT_BIAS, &in[offset], sizeof(int32_t));
}

static void InitLookupIndices() {
  for (size_t i = 0; i < 256; i++) {
    uint64_t j = i;
    uint64_t k = 0;
    while (j) {
      int sq = __builtin_ctzll(j);
      j &= j - 1;
      LOOKUP_INDICES[i][k++] = sq;
    }
  }
}

// Scalar NNUE inference (matches original Propagate scalar path)
int Propagate(Accumulator* acc, const int stm) {
  int16_t* output = stm == WHITE ? acc->values[WHITE] : acc->values[BLACK];

  // L1: input (N_L1=2048) → L2 (N_L2=16) — CReLU8 + matmul
  int32_t l2_out[N_L2];
  for (int i = 0; i < N_L2; i++) {
    int32_t sum = L1_BIASES[i];
    for (int j = 0; j < N_L1; j++) {
      int16_t val = output[j];
      // CReLU8: clamp to [0, 127]
      if (val < 0) val = 0;
      else if (val > 127) val = 127;
      sum += val * L1_WEIGHTS[i * N_L1 + j];
    }
    l2_out[i] = sum;
  }

  // ReLU16 for L2
  for (int i = 0; i < N_L2; i++) {
    if (l2_out[i] < 0) l2_out[i] = 0;
  }

  // L2: (N_L2=16) → L3 (N_L3=32)
  int32_t l3_out[N_L3];
  for (int i = 0; i < N_L3; i++) {
    int32_t sum = L2_BIASES[i];
    for (int j = 0; j < N_L2; j++) {
      sum += l2_out[j] * L2_WEIGHTS[i * N_L2 + j];
    }
    l3_out[i] = sum;
  }

  // ReLU16 for L3
  for (int i = 0; i < N_L3; i++) {
    if (l3_out[i] < 0) l3_out[i] = 0;
  }

  // Output: (N_L3=32) → 1
  int32_t out = OUTPUT_BIAS;
  for (int i = 0; i < N_L3; i++) {
    out += l3_out[i] * OUTPUT_WEIGHTS[i];
  }

  return out >> QUANT2_BITS;
}

int Predict(Board* board) {
  ResetAccumulator(board->accumulators, board, WHITE);
  ResetAccumulator(board->accumulators, board, BLACK);

  return board->stm == WHITE ? Propagate(board->accumulators, WHITE) : Propagate(board->accumulators, BLACK);
}

void LoadDefaultNN() {
  InitLookupIndices();

  // Load from embedded file in virtual filesystem
  char* path = EVALFILE;
  FILE* fin = fopen(path, "rb");
  if (fin == NULL) {
    printf("info string Unable to read NNUE file at %s\n", path);
    return;
  }

  uint8_t* data = malloc(NETWORK_SIZE);
  if (fread(data, sizeof(uint8_t), NETWORK_SIZE, fin) != NETWORK_SIZE) {
    printf("info string Error reading NNUE file at %s\n", path);
    fclose(fin);
    free(data);
    return;
  }

  CopyData(data);
  fclose(fin);
  free(data);
}

int LoadNetwork(char* path) {
  InitLookupIndices();
  FILE* fin = fopen(path, "rb");
  if (fin == NULL) {
    printf("info string Unable to read file at %s\n", path);
    return 0;
  }

  uint8_t* data = malloc(NETWORK_SIZE);
  if (fread(data, sizeof(uint8_t), NETWORK_SIZE, fin) != NETWORK_SIZE) {
    printf("info string Error reading file at %s\n", path);
    fclose(fin);
    free(data);
    return 0;
  }

  CopyData(data);
  fclose(fin);
  free(data);
  return 1;
}
