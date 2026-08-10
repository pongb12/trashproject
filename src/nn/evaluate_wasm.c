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

// Scalar NNUE inference — matches original evaluate.c scalar path exactly
// Key differences from previous version:
// 1. InputCReLU8: clamp to [0, 127<<5] then >> QUANT1_BITS (5) → int8_t
// 2. L1Affine: sparse matmul (skip zeros), result >> QUANT1_BITS (5)
// 3. L2Affine: result >> QUANT1_BITS (5)
// 4. L3Transform: dot product + OUTPUT_BIAS (no shift)
// 5. Final: L3Transform result >> QUANT2_BITS (12)

int Propagate(Accumulator* acc, const int stm) {
  // Step 1: InputCReLU8 — convert accumulator (int16_t) → int8_t
  // Clamp to [0, 127<<5] then >> QUANT1_BITS (5)
  int8_t x0[N_L1]; // N_L1 = 2*N_HIDDEN = 2048
  const int max_val = 127 << 5; // = 4064
  const int views[2] = {stm, !stm};
  for (int v = 0; v < 2; v++) {
    const int16_t* in = acc->values[views[v]];
    int8_t* out = &x0[N_HIDDEN * v];
    for (int i = 0; i < N_HIDDEN; i++) {
      int val = in[i];
      if (val < 0) val = 0;
      if (val > max_val) val = max_val;
      out[i] = val >> QUANT1_BITS;
    }
  }

  // Step 2: L1Affine — x0 (int8_t, N_L1=2048) → dest (int32_t, N_L2=16)
  // Sparse matmul: skip zeros, result >> QUANT1_BITS (5)
  int32_t dest[N_L3]; // N_L3 > N_L2, reuse
  for (int i = 0; i < N_L2; i++)
    dest[i] = L1_BIASES[i];
  for (int i = 0; i < N_L1; i++) {
    if (!x0[i]) continue;
    for (int j = 0; j < N_L2; j++)
      dest[j] += x0[i] * L1_WEIGHTS[j * N_L1 + i];
  }
  for (int i = 0; i < N_L2; i++)
    dest[i] = dest[i] >> QUANT1_BITS;

  // Step 3: ReLU16 — dest (int32_t) → act (int16_t), clamp to >= 0
  int16_t act[N_L3];
  for (int i = 0; i < N_L2; i++)
    act[i] = dest[i] < 0 ? 0 : dest[i];

  // Step 4: L2Affine — act (int16_t, N_L2=16) → dest (int32_t, N_L3=32)
  // result >> QUANT1_BITS (5)
  for (int i = 0; i < N_L3; i++) {
    dest[i] = L2_BIASES[i];
    for (int j = 0; j < N_L2; j++)
      dest[i] += act[j] * L2_WEIGHTS[i * N_L2 + j];
    dest[i] = dest[i] >> QUANT1_BITS;
  }

  // Step 5: ReLU16 again
  for (int i = 0; i < N_L3; i++)
    act[i] = dest[i] < 0 ? 0 : dest[i];

  // Step 6: L3Transform — act (int16_t, N_L3=32) → int32_t
  int32_t result = OUTPUT_BIAS;
  for (int i = 0; i < N_L3; i++)
    result += act[i] * OUTPUT_WEIGHTS[i];

  // Final: >> QUANT2_BITS (12)
  return result >> QUANT2_BITS;
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
