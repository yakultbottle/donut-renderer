#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <wchar.h>
#include <locale.h>

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

#define LUMINANCE (wchar_t[]){L'░', L'▒', L'▓', L'█'}


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

void render_buf(wchar_t buf[HEIGHT][WIDTH]) {
    wprintf(L"\x1b[H"); // return cursor to home, top left
    wprintf(L"\x1b[38;2;223;140;67m");
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            putwchar(buf[i][j]);
        }
        putwchar('\n');
    }
    fflush(stdout);
}

int in_bounds(point2d pix) {
    return pix.x > 0 && pix.x < WIDTH && pix.y > 0 && pix.y < HEIGHT;
}

// Drawing donut at rotation A, B, where A is around X axis and B around Z
// https://www.symbolab.com/solver/matrix-multiply-calculator/%5Cbegin%7Bpmatrix%7Dn%2Br%5Ccdot%20cos%5Cleft(X%5Cright)%26r%5Ccdot%20sin%5Cleft(X%5Cright)%260%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7Dcos%5Cleft(Y%5Cright)%260%26sin%5Cleft(Y%5Cright)%5C%5C%20%20%20%200%261%260%5C%5C%20%20%20%20-sin%5Cleft(Y%5Cright)%260%26cos%5Cleft(Y%5Cright)%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7D1%260%260%5C%5C%20%20%20%20%200%26cos%5Cleft(A%5Cright)%26-sin%5Cleft(A%5Cright)%5C%5C%20%20%20%20%200%26sin%5Cleft(A%5Cright)%26cos%5Cleft(A%5Cright)%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7Dcos%5Cleft(B%5Cright)%26-sin%5Cleft(B%5Cright)%260%5C%5C%20%20%20sin%5Cleft(B%5Cright)%26cos%5Cleft(B%5Cright)%260%5C%5C%20%20%200%260%261%5Cend%7Bpmatrix%7D
void draw_donut(float A, float B, wchar_t buf[HEIGHT][WIDTH], float z[HEIGHT][WIDTH]) {
    float cosA = cos(A);
    float cosB = cos(B);
    float sinA = sin(A);
    float sinB = sin(B);

    for (float X = 0.; X < 6.28; X += dX) {
        float cosX = cos(X);
        float sinX = sin(X);
        float r2r1cos = R2 + R1 * cosX;
        float sinXcosA = sinX * cosA;

        for (float Y = 0.; Y < 6.28; Y += dY) {
            float cosY = cos(Y);
            float sinY = sin(Y);
            float sinYcosA = sinY * cosA;
            float supalong = R1 * sinXcosA + sinY * sinA * r2r1cos;

            point3d pix3d = (point3d){
                cosY * cosB * r2r1cos + sinB * supalong,
                cosB * supalong - cosY * sinB * r2r1cos,
                sinYcosA * r2r1cos - R1 * sinX * sinA,
            };

            float inverse_z = 1. / (pix3d.z + dz);

            point2d pix2d = (point2d){
                WIDTH / 2 + (int)(pix3d.x * ZOOM * 2 * inverse_z),
                HEIGHT / 2 - (int)(pix3d.y * ZOOM * inverse_z),
            };

            // inverse_z being smaller means it is closer to screen, render this pixel
            if (in_bounds(pix2d) && inverse_z > z[pix2d.y][pix2d.x]) {
                // Luminance derivation
                // https://www.symbolab.com/solver/matrix-multiply-calculator/%5Cbegin%7Bpmatrix%7Dcos%5Cleft(X%5Cright)%26sin%5Cleft(X%5Cright)%260%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7Dcos%5Cleft(Y%5Cright)%260%26sin%5Cleft(Y%5Cright)%5C%5C%20%20%20%20%20%200%261%260%5C%5C%20%20%20%20%20%20-sin%5Cleft(Y%5Cright)%260%26cos%5Cleft(Y%5Cright)%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7D1%260%260%5C%5C%20%20%20%20%20%20%200%26cos%5Cleft(A%5Cright)%26-sin%5Cleft(A%5Cright)%5C%5C%20%20%20%20%20%20%200%26sin%5Cleft(A%5Cright)%26cos%5Cleft(A%5Cright)%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7Dcos%5Cleft(B%5Cright)%26-sin%5Cleft(B%5Cright)%260%5C%5C%20%20%20%20%20sin%5Cleft(B%5Cright)%26cos%5Cleft(B%5Cright)%260%5C%5C%20%20%20%20%200%260%261%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7D0%261%26-1%5Cend%7Bpmatrix%7D?or=input
                float L = sinX * cosA * cosB + cosX * sinY * sinA * cosB - cosX * cosY * sinB - cosX * sinYcosA + sinX * sinA;

                z[pix2d.y][pix2d.x] = inverse_z;
                // luminance equation did not normalise light dir, so factor that in here:
                // sqrt(2) * 2.2 = 4.2, now in range 0..4 after int truncation
                // also clamp to 0, so that all dark parts of the donut are rendered at funny angles
                int luminance_index = L > 0 ? (int)(L * 2.2) : 0;
                buf[pix2d.y][pix2d.x] = LUMINANCE[luminance_index];
            }
        }
    }
}

void cleanup() {
    wprintf(L"\x1b[?25h");   // show cursor
    wprintf(L"\x1b[?1049l"); // close alternate screen buffer
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);

    setlocale(LC_ALL, "");

    wprintf(L"\x1b[?1049h"); // open alternate screen buffer
    wprintf(L"\x1b[?25l");   // hide cursor

    wchar_t buf[HEIGHT][WIDTH];
    float z[HEIGHT][WIDTH];

    float A = 0.;
    float B = 0.;

    while (1) {
        wmemset((wchar_t*)buf, L' ', sizeof(buf) / sizeof(wchar_t));
        memset(z, 0, sizeof(z));
        draw_donut(A, B, buf, z);

        render_buf(buf);
        usleep(quanta);

        A += dA;
        B += dB;

        if (A > 6.28) A = 0.;
        if (B > 6.28) B = 0.;
    }
}
