#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define WIDTH 80
#define HEIGHT 22
#define R1 1
#define R2 2
#define dz 5
#define ZOOM (HEIGHT * dz * 3) / (8 * (R1 + R2))
#define dA 0.04
#define dB 0.02
#define dX 0.07
#define dY 0.02

void clear() {
    printf("\x1b[2J");
}

void home() {
    printf("\x1b[H");
}

void render_buf(char buf[HEIGHT][WIDTH]) {
    home();
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            putchar(buf[i][j]);
        }
        putchar('\n');
    }
    fflush(stdout);
}

typedef struct {
    float x;
    float y;
    float z;
} point3d;

typedef struct {
    int x;
    int y;
} point2d;

point2d project(point3d pix) {
    return (point2d){
        WIDTH / 2 + (int)(pix.x * ZOOM * 2 / (pix.z + dz)),
        HEIGHT / 2 - (int)(pix.y * ZOOM / (pix.z + dz)),
    };
}

int in_bounds(point2d pix) {
    return pix.x > 0 && pix.x < WIDTH && pix.y > 0 && pix.y < HEIGHT;
}

// Drawing donut at rotation A, B, where A is around X axis and B around Z
// https://www.symbolab.com/solver/matrix-multiply-calculator/%5Cbegin%7Bpmatrix%7Dn%2Br%5Ccdot%20cos%5Cleft(X%5Cright)%26r%5Ccdot%20sin%5Cleft(X%5Cright)%260%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7Dcos%5Cleft(Y%5Cright)%260%26sin%5Cleft(Y%5Cright)%5C%5C%20%20%20%200%261%260%5C%5C%20%20%20%20-sin%5Cleft(Y%5Cright)%260%26cos%5Cleft(Y%5Cright)%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7D1%260%260%5C%5C%20%20%20%20%200%26cos%5Cleft(A%5Cright)%26-sin%5Cleft(A%5Cright)%5C%5C%20%20%20%20%200%26sin%5Cleft(A%5Cright)%26cos%5Cleft(A%5Cright)%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7Dcos%5Cleft(B%5Cright)%26-sin%5Cleft(B%5Cright)%260%5C%5C%20%20%20sin%5Cleft(B%5Cright)%26cos%5Cleft(B%5Cright)%260%5C%5C%20%20%200%260%261%5Cend%7Bpmatrix%7D
void draw_donut(float A, float B, char buf[HEIGHT][WIDTH]) {
    float cosA = cos(A);
    float cosB = cos(B);
    float sinA = sin(A);
    float sinB = sin(B);

    for (float Y = 0.; Y < 6.28; Y += dY) {
        float cosY = cos(Y);
        float sinY = sin(Y);

        for (float X = 0.; X < 6.28; X += dX) {
            float cosX = cos(X);
            float sinX = sin(X);

            point3d pix3d = (point3d){
                cosY * cosB * (R2 + R1 * cosX) +  sinB * (R1 * sinX * cosA + sinY * sinA * (R2 + R1 * cosX)),
                cosB * (R1 * sinX * cosA + sinY * sinA * (R2 + R1 * cosX)) - cosY * sinB * (R2 + R1 * cosX),
                sinY * cosA * (R2 + R1 * cosX) - R1 * sinX * sinA,
            };

            point2d pix2d = project(pix3d);
            if (in_bounds(pix2d)) {
                buf[pix2d.y][pix2d.x] = '#';
            }
        }
    }
}

int main() {
    clear();

    int FPS = 30;
    int t = 1000000 / FPS;

    char buf[HEIGHT][WIDTH];

    float A = 0.;
    float B = 0.;

    while (1) {
        home();

        memset(buf, ' ', sizeof(buf));
        draw_donut(A, B, buf);

        render_buf(buf);
        usleep(t);

        A += dA;
        B += dB;

        if (A > 6.28) A = 0.;
        if (B > 6.28) B = 0.;
    }
}
