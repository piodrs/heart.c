#include <float.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#define W 80
#define H 40
#define N 58

typedef struct {
  float x, y, z;
  float nx, ny, nz;
} P;

P pts[N * N * N];
int n;
volatile sig_atomic_t done;

void stop(int signal_number) {
  if (signal_number == SIGINT)
    done = 1;
}

float f(float x, float y, float z) {
  float a = x * x + y * y + 2.25f * z * z - 1.0f;
  float b = y * y * y;

  return a * a * a - b * (x * x + 0.1125f * z * z);
}

void build(void) {
  float h = 2.7f / (N - 1);

  for (int i = 1; i < N - 1; i++) {
    for (int j = 1; j < N - 1; j++) {
      for (int k = 1; k < N - 1; k++) {
        float x = -1.35f + i * h;
        float y = -1.35f + j * h;
        float z = -1.35f + k * h;
        float xp = f(x + h, y, z);
        float xm = f(x - h, y, z);
        float yp = f(x, y + h, z);
        float ym = f(x, y - h, z);
        float zp = f(x, y, z + h);
        float zm = f(x, y, z - h);
        float gx, gy, gz, length;

        if (f(x, y, z) > 0.0f || (xp <= 0.0f && xm <= 0.0f && yp <= 0.0f &&
                                  ym <= 0.0f && zp <= 0.0f && zm <= 0.0f))
          continue;

        gx = xp - xm;
        gy = yp - ym;
        gz = zp - zm;
        length = sqrtf(gx * gx + gy * gy + gz * gz);

        if (length <= FLT_EPSILON)
          continue;

        pts[n++] = (P){x, y, z, gx / length, gy / length, gz / length};
      }
    }
  }
}

int main(void) {
  const char sh[] = ".,:;irsXA253hMHGS#9B&@";
  char px[H][W];
  float zb[H * W];

  signal(SIGINT, stop);
  build();
  printf("\x1b[2J\x1b[?25l");

  for (float a = 0.0f; !done; a += 0.0349066f) {
    float c = cosf(a);
    float s = sinf(a);

    memset(px, ' ', sizeof px);
    for (int i = 0; i < H * W; i++) {
      zb[i] = -1e30f;
    }

    for (int i = 0; i < n; i++) {
      P p = pts[i];
      float rx = p.x * c + p.z * s;
      float rz = -p.x * s + p.z * c;
      float nx = p.nx * c + p.nz * s;
      float nz = -p.nx * s + p.nz * c;
      float distance = 4.2f - rz;
      float scale;
      float projected_x, projected_y;
      int sx, sy;

      scale = 35.0f / distance;
      projected_x = W * 0.5f + rx * scale;
      projected_y = H * 0.48f - p.y * scale * 0.52f;
      sx = projected_x;
      sy = projected_y;

      if (sx < 0 || sx >= W || sy < 0 || sy >= H || rz <= zb[sy * W + sx])
        continue;

      float diffuse =
          fmaxf(0.0f, -0.365148f * nx + 0.547723f * p.ny + 0.730297f * nz);

      float rim = 1.0f - fabsf(nz);

      float brightness =
          fminf(1.0f, 0.1f + 0.82f * diffuse + 0.16f * rim * rim);

      size_t shade = brightness * (sizeof sh - 2);

      zb[sy * W + sx] = rz;
      px[sy][sx] = sh[shade];
    }

    printf("\x1b[H");
    for (int row = 0; row < H; row++) {
      printf("%.*s\n", W, px[row]);
    }
    fflush(stdout);
    thrd_sleep(&(struct timespec){0, 33000000}, NULL);
  }

  printf("\x1b[?25h\x1b[0m\n");
  return 0;
}
