#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

// Original terminal attributes
struct termios original_termios;

// Restore the original terminal settings and clear the screen
void disable_raw_mode() {
    write(STDOUT_FILENO, "\x1b[2J", 4); // Clear screen
    write(STDOUT_FILENO, "\x1b[H", 3);  // Move cursor to top-left
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

// Enable raw mode
void enable_raw_mode() {
    // Save the original terminal settings
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(disable_raw_mode); // Ensure raw mode is disabled on exit

    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON);
    raw.c_oflag &= ~(OPOST);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Clear the screen and move cursor to top-left
void clear_screen() {
    write(STDOUT_FILENO, "\x1b[2J", 4); // Clear screen
    write(STDOUT_FILENO, "\x1b[H", 3);  // Move cursor to top-left
}

// Draw a vertical line of characters on the left-hand side
void draw_vertical_line() {
    for (int i = 0; i < 24; i++) { // Assuming 24 rows for simplicity
        write(STDOUT_FILENO, "~\r\n", 3); // Write ~ and move to the next line
    }
}

int main() {
    enable_raw_mode();
    clear_screen();
    draw_vertical_line();

    char c;
    while (1) {
        if (read(STDIN_FILENO, &c, 1) == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (c == 'q') break; // Exit on 'q'
    }

    return 0;
}
