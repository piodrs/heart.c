#include <float.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#define AX_IMPLEMENTATION
#include "./thirdparty/axiom/axiom.h"

#define HEART_WIDTH 80
#define HEART_HEIGHT 40
#define HEART_GRID_SIZE 58

typedef struct {
    float x, y, z;
    float nx, ny, nz;
} Heart_Point;

typedef struct {
    Ax_Array points;
} Heart_Model;

volatile sig_atomic_t heart_interrupted = 0;

void heart_handle_signal(int signal_number)
{
    if (signal_number == SIGINT) heart_interrupted = 1;
}

float heart_field(float x, float y, float z)
{
    float shape = x * x + y * y + 2.25f * z * z - 1.0f;
    float y_cubed = y * y * y;

    return shape * shape * shape - y_cubed * (x * x + 0.1125f * z * z);
}

void heart_build(Heart_Model *model)
{
    float step = 2.7f / (HEART_GRID_SIZE - 1);

    for (size_t i = 1; i < HEART_GRID_SIZE - 1; ++i) {
        for (size_t j = 1; j < HEART_GRID_SIZE - 1; ++j) {
            for (size_t k = 1; k < HEART_GRID_SIZE - 1; ++k) {
                float x = -1.35f + i * step;
                float y = -1.35f + j * step;
                float z = -1.35f + k * step;

                float x_positive = heart_field(x + step, y, z);
                float x_negative = heart_field(x - step, y, z);
                float y_positive = heart_field(x, y + step, z);
                float y_negative = heart_field(x, y - step, z);
                float z_positive = heart_field(x, y, z + step);
                float z_negative = heart_field(x, y, z - step);
                
                float normal_x, normal_y, normal_z, normal_length;

                if (heart_field(x, y, z) > 0.0f ||
                    (x_positive <= 0.0f && x_negative <= 0.0f &&
                     y_positive <= 0.0f && y_negative <= 0.0f &&
                     z_positive <= 0.0f && z_negative <= 0.0f)) {
                    continue;
                }

                normal_x = x_positive - x_negative;
                normal_y = y_positive - y_negative;
                normal_z = z_positive - z_negative;
                normal_length = sqrtf(normal_x * normal_x + normal_y * normal_y + normal_z * normal_z);

                if (normal_length <= FLT_EPSILON) continue;

                ax_array_push_value(&model->points, Heart_Point, ((Heart_Point) {
                    .x = x,
                    .y = y,
                    .z = z,
                    .nx = normal_x / normal_length,
                    .ny = normal_y / normal_length,
                    .nz = normal_z / normal_length,
                }));
            }
        }
    }
}

int main(void)
{
    const char shades[] = ".,:;irsXA253hMHGS#9B&@";
    char pixels[HEART_HEIGHT][HEART_WIDTH];
    float depth_buffer[HEART_HEIGHT * HEART_WIDTH];

    Heart_Model heart = {
        .points = ax_array(Heart_Point),
    };

    signal(SIGINT, heart_handle_signal);
    heart_build(&heart);
    printf("\x1b[2J\x1b[?25l");

    for (float angle = 0.0f; !heart_interrupted; angle += 0.0349066f) {
        float cosine = cosf(angle);
        float sine = sinf(angle);

        memset(pixels, ' ', sizeof(pixels));
        for (size_t i = 0; i < HEART_HEIGHT * HEART_WIDTH; ++i) {
            depth_buffer[i] = -1e30f;
        }

        for (size_t i = 0; i < heart.points.length; ++i) {
            Heart_Point point = *ax_array_at_as(&heart.points, i, Heart_Point);

            float rotated_x = point.x * cosine + point.z * sine;
            float rotated_z = -point.x * sine + point.z * cosine;
            
            float normal_x = point.nx * cosine + point.nz * sine;
            float normal_z = -point.nx * sine + point.nz * cosine;
            
            float distance = 4.2f - rotated_z;
            float scale = 35.0f / distance;
            
            float projected_x = HEART_WIDTH * 0.5f + rotated_x * scale;
            float projected_y = HEART_HEIGHT * 0.48f - point.y * scale * 0.52f;

            int screen_x = projected_x;
            int screen_y = projected_y;

            if (screen_x < 0 || screen_x >= HEART_WIDTH ||
                screen_y < 0 || screen_y >= HEART_HEIGHT ||
                rotated_z <= depth_buffer[screen_y * HEART_WIDTH + screen_x]) {
                continue;
            }

            float diffuse = fmaxf(0.0f, -0.365148f * normal_x + 0.547723f * point.ny + 0.730297f * normal_z);
            float rim = 1.0f - fabsf(normal_z);
            float brightness = fminf(1.0f, 0.1f + 0.82f * diffuse + 0.16f * rim * rim);
            
            size_t shade = brightness * (sizeof(shades) - 2);
            size_t pixel = screen_y * HEART_WIDTH + screen_x;

            depth_buffer[pixel] = rotated_z;
            pixels[screen_y][screen_x] = shades[shade];
        }

        printf("\x1b[H");
        for (size_t row = 0; row < HEART_HEIGHT; ++row) {
            printf("%.*s\n", HEART_WIDTH, pixels[row]);
        }
        fflush(stdout);
        thrd_sleep(&(struct timespec) {0, 33000000}, NULL);
    }

    ax_array_release(&heart.points);
    printf("\x1b[?25h\x1b[0m\n");
    return 0;
}
