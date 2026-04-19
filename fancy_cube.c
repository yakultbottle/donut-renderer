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

#define HALF_CUBE_LEN 1.5
#define z_offset 5
#define ZOOM (HEIGHT * z_offset * 3) / (8 * HALF_CUBE_LEN * 2)

#define dA 0.04
#define dB 0.02
// estimated when nearest cube face is to screen, it is 24 pixels long. Found
// out empirically. 1 / 24 == 0.041, so 0.03 should be fine
#define dC 0.03

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

// Order is botleft, topright, botleft
int faces[6][4] = {
    {5, 4, 7}, // Front face  (Z = -HALF_CUBE_LEN)
    {0, 1, 2}, // Back face   (Z =  HALF_CUBE_LEN)
    {1, 0, 5}, // Top face    (Y =  HALF_CUBE_LEN)
    {7, 6, 3}, // Bottom face (Y = -HALF_CUBE_LEN)
    {4, 0, 6}, // Right face  (X =  HALF_CUBE_LEN)
    {1, 5, 3}  // Left face   (X = -HALF_CUBE_LEN)
};

void render_buf(wchar_t buf[HEIGHT][WIDTH]) {
    wprintf(L"\x1b[H"); // return cursor to home, top left
    wprintf(L"\x1b[38;2;0;255;255m");
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

point3d *rotate(float A, float B, point3d vertices[]) {
    float cosA = cos(A);
    float cosB = cos(B);
    float sinA = sin(A);
    float sinB = sin(B);

    point3d *rotated_vertices = malloc(8 * sizeof(point3d));

    for (int i = 0; i < 8; ++i) {
        rotated_vertices[i] = (point3d){
            vertices[i].x * cosA + vertices[i].y * sinA,
            vertices[i].z * sinB + cosB * (vertices[i].y * cosA - vertices[i].x * sinA),
            vertices[i].z * cosB - sinB * (vertices[i].y * cosA - vertices[i].x * sinA),
        };
    }

    return rotated_vertices;
}

// Drawing cube at rotation A, B, where A is around X axis and B around Z
// https://www.symbolab.com/solver/matrix-multiply-calculator/%5Cbegin%7Bpmatrix%7Dx%26y%26z%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7Dcos%5Cleft(A%5Cright)%26-sin%5Cleft(A%5Cright)%260%5C%5C%20%20%20sin%5Cleft(A%5Cright)%26cos%5Cleft(A%5Cright)%260%5C%5C%20%20%200%260%261%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7D1%260%260%5C%5C%20%20%20%20%20%200%26cos%5Cleft(B%5Cright)%26-sin%5Cleft(B%5Cright)%5C%5C%20%20%20%20%20%200%26sin%5Cleft(B%5Cright)%26cos%5Cleft(B%5Cright)%5Cend%7Bpmatrix%7D?or=input
void draw_cube(float A, float B, wchar_t buf[HEIGHT][WIDTH], float z_buf[HEIGHT][WIDTH]) {
    point3d *rot_vertices = rotate(A, B, vertices);

    for (int face = 0; face < 6; ++face) {
        point3d TL = rot_vertices[faces[face][0]];
        point3d TR = rot_vertices[faces[face][1]];
        point3d BL = rot_vertices[faces[face][2]];

        point3d TL_TR = (point3d){
            TR.x - TL.x,
            TR.y - TL.y,
            TR.z - TL.z,
        };

        point3d TL_BL = (point3d){
            BL.x - TL.x,
            BL.y - TL.y,
            BL.z - TL.z,
        };

        point3d cross_product = (point3d){
            TL_BL.y * TL_TR.z - TL_BL.z * TL_TR.y,
            TL_BL.z * TL_TR.x - TL_BL.x * TL_TR.z,
            TL_BL.x * TL_TR.y - TL_BL.y * TL_TR.x,
        };

        if (cross_product.z < 0) {
            continue;
        }

        // Light dir (0, 1, -1) not normalised: sqrt(2)
        // Cross produce magnitude: 9
        // 1 / (9 * sqrt(2)) = 0.07856 to normalise
        float angle_intensity = -(cross_product.y - cross_product.z) * 0.07856;

        for (float u = 0.; u <= 1.0; u += dC) {
            for (float v = 0.; v <= 1.0; v += dC) {
                float x = TL.x + (TR.x - TL.x) * u + (BL.x - TL.x) * v;
                float y = TL.y + (TR.y - TL.y) * u + (BL.y - TL.y) * v;
                float z = TL.z + (TR.z - TL.z) * u + (BL.z - TL.z) * v;

                float inverse_z = 1. / (z + z_offset);
                point2d pix2d = (point2d){
                    WIDTH / 2 + (int)(x * ZOOM * 2 * inverse_z),
                    HEIGHT / 2 - (int)(y * ZOOM * inverse_z),
                };

                // inverse_z being smaller means it is closer to screen, render this pixel
                if (in_bounds(pix2d) && inverse_z > z_buf[pix2d.y][pix2d.x]) {
                    z_buf[pix2d.y][pix2d.x] = inverse_z;

                    float dist_squared = x * x + (y - 3) * (y - 3) + (z + 3) * (z + 3);
                    // 50 is a random number I found. Sorry I'm bad at math
                    float dist_intensity = 1.0 - (dist_squared / 50.);
                    float L = angle_intensity * dist_intensity;

                    // clamp to 0, so that all dark parts of the donut are
                    // rendered at funny angles
                    int luminance_index = L <= 0 ? 0 : L > 12 ? 11 : (int)(L * 4);
                    buf[pix2d.y][pix2d.x] = LUMINANCE[luminance_index];
                }
            }
        }
    }

    free(rot_vertices);
}

void cleanup() {
    wprintf(L"\x1b[?25h");   // show cursor
    wprintf(L"\x1b[?1049l"); // close alternate screen buffer
    wprintf(L"\x1b[0m");     // reset all text colour
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);

    setlocale(LC_ALL, "");

    wprintf(L"\x1b[?1049h"); // open alternate screen buffer
    wprintf(L"\x1b[?25l");   // hide cursor

    wchar_t buf[HEIGHT][WIDTH];
    float z_buf[HEIGHT][WIDTH];

    float A = 0.;
    float B = 0.;

    while (1) {
        wmemset((wchar_t*)buf, L' ', sizeof(buf) / sizeof(wchar_t));
        memset(z_buf, 0, sizeof(z_buf));
        draw_cube(A, B, buf, z_buf);

        render_buf(buf);
        usleep(quanta);

        A += dA;
        B += dB;

        if (A > 6.28) A = 0.;
        if (B > 6.28) B = 0.;
    }
}
