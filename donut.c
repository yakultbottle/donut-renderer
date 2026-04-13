#include <stdio.h>
#include <unistd.h>
// #include <stdlib.h>
#include <string.h>

void clear() {
    printf("\x1b[2J");
}

void home() {
    printf("\x1b[H");
}

int main() {
    clear();

    char *line = "Typing demonstration...\nI can even do newline?\n";
    int n = strlen(line);

    int FPS = 30;
    int t = 1000000 / FPS;

    home();
    for (int i = 0; i < n; ++i) {
        putchar(line[i]);
        fflush(stdout);
        usleep(t);
    }
}
