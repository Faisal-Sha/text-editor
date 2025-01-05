#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

// Original terminal attributes
struct termios original_termios;

// Restore the original terminal settings
void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

// Enable raw mode
void enable_raw_mode() {
    // Save the original terminal settings
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(disable_raw_mode); // Ensure raw mode is disabled on exit

    struct termios raw = original_termios;
    // Turn off canonical mode and echo
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    // Disable Ctrl-S and Ctrl-Q
    raw.c_iflag &= ~(IXON);
    // Disable output processing
    raw.c_oflag &= ~(OPOST);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    // Enable raw mode
    enable_raw_mode();

    printf("Press 'q' to quit.\n");

    char c;
    while (1) {
        // Read one character at a time
        if (read(STDIN_FILENO, &c, 1) == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (c == 'q') break; // Exit on 'q'
    }

    return 0;
}
