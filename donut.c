#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define WIDTH 80
#define HEIGHT 20
#define dz 10

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

void draw_outline(char buf[HEIGHT][WIDTH]) {
    for (int i = 1; i < HEIGHT - 1; ++i) {
        buf[i][0] = '|';
        buf[i][WIDTH - 1] = '|';
    }

    for (int j = 1; j < WIDTH - 1; ++j) {
        buf[0][j] = '-';
        buf[HEIGHT - 1][j] = '-';
    }

    buf[0][0] = '+';
    buf[HEIGHT - 1][0] = '+';
    buf[0][WIDTH - 1] = '+';
    buf[HEIGHT - 1][WIDTH - 1] = '+';
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

#define ZOOM 15.0
point2d project(point3d pix) {
    return (point2d){
        (int) WIDTH / 2 + pix.x * ZOOM / (pix.z + dz),
        (int) HEIGHT / 2 - pix.y * ZOOM / (pix.z + dz),
    };
}

// Rotation around y axis and x axis
point3d rotate(point3d pix, float A) {
    return (point3d) {
        // pix.x * cos(A) - pix.z * sin(A),
        // pix.y,
        // pix.x * sin(A) + pix.z * cos(A),
        pix.x * cos(A) - pix.z * sin(A),
        pix.y * cos(A) + sin(A) * (pix.x * sin(A) + pix.z * cos(A)),
        -pix.y * sin(A) + cos(A) * (pix.x * sin(A) + pix.z * cos(A))
    };
}

int in_bounds(point2d pix) {
    return pix.x > 0 && pix.x < WIDTH && pix.y > 0 && pix.y < HEIGHT;
}

int main() {
    clear();

    int FPS = 60;
    int t = 1000000 / FPS;

    char buf[HEIGHT][WIDTH];
    memset(buf, ' ', sizeof(buf));

    // point3d pix3d = {WIDTH - 1, HEIGHT - 1, 0};
    #define CUBE_LENGTH 3
    point3d pixels3d[] = {
        {CUBE_LENGTH, CUBE_LENGTH, CUBE_LENGTH},
        {CUBE_LENGTH, -CUBE_LENGTH, CUBE_LENGTH},
        {-CUBE_LENGTH, CUBE_LENGTH, CUBE_LENGTH},
        {-CUBE_LENGTH, -CUBE_LENGTH, CUBE_LENGTH},
        {CUBE_LENGTH, CUBE_LENGTH, -CUBE_LENGTH},
        {CUBE_LENGTH, -CUBE_LENGTH, -CUBE_LENGTH},
        {-CUBE_LENGTH, CUBE_LENGTH, -CUBE_LENGTH},
        {-CUBE_LENGTH, -CUBE_LENGTH, -CUBE_LENGTH},
    };
    point2d pix2d;

    float A = 0;
    float d_theta = 6.28 / FPS / 10;
    while (1) {
        home();
        memset(buf, ' ', sizeof(buf));
        draw_outline(buf);

        for (int i = 0; i < 8; ++i) {
            pix2d = project(rotate(pixels3d[i], A));

            if (in_bounds(pix2d)) {
                buf[pix2d.y][pix2d.x] = '#';
            }
        }

        A += d_theta;
        if (A > 6.28) A = 0;

        render_buf(buf);
        usleep(t);
    }
}
