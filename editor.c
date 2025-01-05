#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

// Original terminal attributes
struct termios original_termios;

// Cursor position
int cursor_row = 1, cursor_col = 1; // Start at top-left (1,1)

// Terminal size
int screen_rows, screen_cols;

// Restore the original terminal settings
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

// Get the size of the terminal
void get_terminal_size() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        screen_rows = 24; // Default rows
        screen_cols = 80; // Default columns
    } else {
        screen_rows = ws.ws_row;
        screen_cols = ws.ws_col;
    }
}

// Move the cursor to the specified position
void move_cursor(int row, int col) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
    write(STDOUT_FILENO, buf, strlen(buf));
}

// Handle user input to move the cursor
void process_input(char c) {
    switch (c) {
        case 'h': // Move left
            if (cursor_col > 1) cursor_col--;
            break;
        case 'j': // Move down
            if (cursor_row < screen_rows) cursor_row++;
            break;
        case 'k': // Move up
            if (cursor_row > 1) cursor_row--;
            break;
        case 'l': // Move right
            if (cursor_col < screen_cols) cursor_col++;
            break;
    }
    move_cursor(cursor_row, cursor_col); // Update cursor position
}

// Draw the vertical line of ~ characters
void draw_vertical_line() {
    for (int i = 0; i < screen_rows; i++) {
        write(STDOUT_FILENO, "~\r\n", 3); // Write ~ and move to the next line
    }
}

int main() {
    enable_raw_mode();
    get_terminal_size();
    clear_screen();
    draw_vertical_line();
    move_cursor(cursor_row, cursor_col);

    char c;
    while (1) {
        if (read(STDIN_FILENO, &c, 1) == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (c == 'q') break; // Exit on 'q'
        process_input(c);    // Handle cursor movement
    }

    return 0;
}
