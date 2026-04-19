#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

point3d* rotate(float A, float B, point3d vertices[]) {
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
    {1, 0, 3}, // Front face  (Z =  HALF_CUBE_LEN)
    {5, 4, 7}, // Back face   (Z = -HALF_CUBE_LEN)
    {1, 0, 5}, // Top face    (Y =  HALF_CUBE_LEN)
    {3, 2, 7}, // Bottom face (Y = -HALF_CUBE_LEN)
    {0, 4, 2}, // Right face  (X =  HALF_CUBE_LEN)
    {5, 1, 7}  // Left face   (X = -HALF_CUBE_LEN)
};

// Drawing cube at rotation A, B, where A is around X axis and B around Z
// https://www.symbolab.com/solver/matrix-multiply-calculator/%5Cbegin%7Bpmatrix%7Dx%26y%26z%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7Dcos%5Cleft(A%5Cright)%26-sin%5Cleft(A%5Cright)%260%5C%5C%20%20%20sin%5Cleft(A%5Cright)%26cos%5Cleft(A%5Cright)%260%5C%5C%20%20%200%260%261%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7D1%260%260%5C%5C%20%20%20%20%20%200%26cos%5Cleft(B%5Cright)%26-sin%5Cleft(B%5Cright)%5C%5C%20%20%20%20%20%200%26sin%5Cleft(B%5Cright)%26cos%5Cleft(B%5Cright)%5Cend%7Bpmatrix%7D?or=input
void draw_cube(float A, float B, char buf[HEIGHT][WIDTH], float z_buf[HEIGHT][WIDTH]) {
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

        float L = cross_product.y - cross_product.z;

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

                    // luminance equation did not normalise light dir, so
                    // factor that in here along with the cross product
                    // magnitude: 9 * sqrt(2) = 12.7, to get in range 0..11
                    // after int truncation have to * 0.92. also clamp to 0, so
                    // that all dark parts of the donut are rendered at funny
                    // angles
                    int luminance_index = L > 0 ? (int)(L * 0.92) : 0;
                    buf[pix2d.y][pix2d.x] = LUMINANCE[luminance_index];
                }
            }
        }
    }
}

int main() {
    printf("\x1b[2J"); // clear screen

    char buf[HEIGHT][WIDTH];
    float z_buf[HEIGHT][WIDTH];

    float A = 0.;
    float B = 0.;

    while (1) {
        memset(buf, ' ', sizeof(buf));
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
