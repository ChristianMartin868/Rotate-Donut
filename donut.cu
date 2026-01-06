#include <cuda_runtime.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PI 3.14159265f
#define THETA_SPACING 0.07f
#define PHI_SPACING 0.02f
#define R1 1.0f
#define R2 2.0f
#define K2 5.0f
#define K1 20.0f
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 30
#define LUM_COUNT 12

__device__ __constant__ char d_luminance[LUM_COUNT + 1] = ".,-~:;=!*#$@";

static void check_cuda(cudaError_t result, const char *msg) {
  if (result != cudaSuccess) {
    fprintf(stderr, "%s: %s\n", msg, cudaGetErrorString(result));
    exit(1);
  }
}

__global__ void render_kernel(char *output, int *zbuffer, float sinA, float cosA,
                              float sinB, float cosB) {
  int t = blockIdx.x * blockDim.x + threadIdx.x;
  float theta = (float)t * THETA_SPACING;
  if (theta >= 2.0f * PI) {
    return;
  }

  float sintheta = sinf(theta);
  float costheta = cosf(theta);

  for (float phi = 0.0f; phi < 2.0f * PI; phi += PHI_SPACING) {
    float sinphi = sinf(phi);
    float cosphi = cosf(phi);

    float circlex = R2 + R1 * costheta;
    float circley = R1 * sintheta;

    float x = circlex * (cosB * cosphi + sinA * sinB * sinphi) -
              circley * cosA * sinB;
    float y = circlex * (sinB * cosphi - sinA * cosB * sinphi) +
              circley * cosA * cosB;
    float z = K2 + cosA * circlex * sinphi + circley * sinA;
    float ooz = 1.0f / z;

    int xp = (int)(SCREEN_WIDTH / 2.0f + 2.0f * K1 * ooz * x);
    int yp = (int)(SCREEN_HEIGHT / 2.0f - K1 * ooz * y);
    if (yp > 0 && yp < SCREEN_HEIGHT && xp > 0 && xp < SCREEN_WIDTH) {
      float L = cosphi * costheta * sinB - cosA * costheta * sinphi -
                sinA * sintheta +
                cosB * (cosA * sintheta - costheta * sinA * sinphi);
      int lum_index = (int)(L * 8.0f);
      if (lum_index < 0) {
        lum_index = 0;
      } else if (lum_index >= LUM_COUNT) {
        lum_index = LUM_COUNT - 1;
      }

      int idx = xp + SCREEN_WIDTH * yp;
      int ooz_i = __float_as_int(ooz);
      int prev = atomicMax(&zbuffer[idx], ooz_i);
      if (ooz_i > prev) {
        output[idx] = d_luminance[lum_index];
      }
    }
  }
}

int main(void) {
  const size_t screen_size = (size_t)SCREEN_WIDTH * SCREEN_HEIGHT;
  char output[SCREEN_WIDTH * SCREEN_HEIGHT];
  char *d_output = NULL;
  int *d_zbuffer = NULL;

  check_cuda(cudaMalloc((void **)&d_output, screen_size),
             "cudaMalloc output");
  check_cuda(cudaMalloc((void **)&d_zbuffer, screen_size * sizeof(int)),
             "cudaMalloc zbuffer");

  const int theta_steps = (int)ceilf((2.0f * PI) / THETA_SPACING);
  const dim3 block(128);
  const dim3 grid((theta_steps + block.x - 1) / block.x);

  float A = 0.0f;
  float B = 0.0f;

  printf("\x1b[2J");
  for (;;) {
    check_cuda(cudaMemset(d_output, ' ', screen_size),
               "cudaMemset output");
    check_cuda(cudaMemset(d_zbuffer, 0, screen_size * sizeof(int)),
               "cudaMemset zbuffer");

    float sinA = sinf(A);
    float cosA = cosf(A);
    float sinB = sinf(B);
    float cosB = cosf(B);

    render_kernel<<<grid, block>>>(d_output, d_zbuffer, sinA, cosA, sinB, cosB);
    check_cuda(cudaGetLastError(), "kernel launch");
    check_cuda(cudaDeviceSynchronize(), "kernel sync");
    check_cuda(cudaMemcpy(output, d_output, screen_size,
                          cudaMemcpyDeviceToHost),
               "cudaMemcpy output");

    printf("\x1b[H");
    for (size_t k = 0; k < screen_size + 1; k++) {
      putchar(k % SCREEN_WIDTH ? output[k] : 10);
    }
    fflush(stdout);

    A += 0.04f;
    B += 0.02f;
    usleep(15000);
  }

  return 0;
}
