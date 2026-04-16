#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define WIDTH 80
#define HEIGHT 22

#define HALF_CUBE_LEN 1.5
#define dz 5
#define ZOOM (HEIGHT * dz * 3) / (8 * HALF_CUBE_LEN * 2)

#define dA 0.04
#define dB 0.02

#define LUMINANCE ".,-~:;=!*#$@"

#define FPS 30
#define quanta 1000000 / FPS

typedef struct {
    float x;
    float y;
    float z;
} point3d;

typedef struct {
    int x;
    int y;
} point2d;

void render_buf(char buf[HEIGHT][WIDTH]) {
    printf("\x1b[H"); // return cursor to home, top left
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            putchar(buf[i][j]);
        }
        putchar('\n');
    }
    fflush(stdout);
}

int in_bounds(point2d pix) {
    return pix.x > 0 && pix.x < WIDTH && pix.y > 0 && pix.y < HEIGHT;
}

point3d vertices[8] = {
    (point3d){HALF_CUBE_LEN, HALF_CUBE_LEN, HALF_CUBE_LEN},
    (point3d){-HALF_CUBE_LEN, HALF_CUBE_LEN, HALF_CUBE_LEN},
    (point3d){HALF_CUBE_LEN, -HALF_CUBE_LEN, HALF_CUBE_LEN},
    (point3d){-HALF_CUBE_LEN, -HALF_CUBE_LEN, HALF_CUBE_LEN},
    (point3d){HALF_CUBE_LEN, HALF_CUBE_LEN, -HALF_CUBE_LEN},
    (point3d){-HALF_CUBE_LEN, HALF_CUBE_LEN, -HALF_CUBE_LEN},
    (point3d){HALF_CUBE_LEN, -HALF_CUBE_LEN, -HALF_CUBE_LEN},
    (point3d){-HALF_CUBE_LEN, -HALF_CUBE_LEN, -HALF_CUBE_LEN},
};

// Drawing cube at rotation A, B, where A is around X axis and B around Z
// https://www.symbolab.com/solver/matrix-multiply-calculator/%5Cbegin%7Bpmatrix%7Dx%26y%26z%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7Dcos%5Cleft(A%5Cright)%26-sin%5Cleft(A%5Cright)%260%5C%5C%20%20%20sin%5Cleft(A%5Cright)%26cos%5Cleft(A%5Cright)%260%5C%5C%20%20%200%260%261%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7D1%260%260%5C%5C%20%20%20%20%20%200%26cos%5Cleft(B%5Cright)%26-sin%5Cleft(B%5Cright)%5C%5C%20%20%20%20%20%200%26sin%5Cleft(B%5Cright)%26cos%5Cleft(B%5Cright)%5Cend%7Bpmatrix%7D?or=input
void draw_cube(float A, float B, char buf[HEIGHT][WIDTH], float z[HEIGHT][WIDTH]) {
    float cosA = cos(A);
    float cosB = cos(B);
    float sinA = sin(A);
    float sinB = sin(B);

    for (int i = 0; i < 8; ++i) {
        point3d pix3d = {
            vertices[i].x * cosA + vertices[i].y * sinA,
            vertices[i].z * sinB + cosB * (vertices[i].y * cosA - vertices[i].x * sinA),
            vertices[i].z * cosB - sinB * (vertices[i].y * cosA - vertices[i].x * sinA),
        };

        float inverse_z = 1. / (pix3d.z + dz);

        point2d pix2d = (point2d){
            WIDTH / 2 + (int)(pix3d.x * ZOOM * 2 * inverse_z),
            HEIGHT / 2 - (int)(pix3d.y * ZOOM * inverse_z),
        };

        // inverse_z being smaller means it is closer to screen, render this pixel
        if (in_bounds(pix2d) && inverse_z > z[pix2d.y][pix2d.x]) {
            // buf[pix2d.y][pix2d.x] = '#';

            // Luminance derivation
            // float L = sinX * cosA * cosB + cosX * sinY * sinA * cosB - cosX * cosY * sinB - cosX * sinYcosA + sinX * sinA;

            // z[pix2d.y][pix2d.x] = inverse_z;
            // luminance equation did not normalise light dir, so factor that in here:
            // 8 * sqrt(2) = 11.3, now in range 0..11 after int truncation
            // also clamp to 0, so that all dark parts of the donut are rendered at funny angles
            // int luminance_index = L > 0 ? (int)(L * 8) : 0;
            // buf[pix2d.y][pix2d.x] = LUMINANCE[luminance_index];
        }
    }
}

int main() {
    printf("\x1b[2J"); // clear screen

    char buf[HEIGHT][WIDTH];
    float z[HEIGHT][WIDTH];

    float A = 0.;
    float B = 0.;

    while (1) {
        memset(buf, ' ', sizeof(buf));
        memset(z, 0, sizeof(z));
        draw_cube(A, B, buf, z);

        render_buf(buf);
        usleep(quanta);

        A += dA;
        B += dB;

        if (A > 6.28) A = 0.;
        if (B > 6.28) B = 0.;
    }
}
