#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

// Terminal settings
struct termios original_termios;

// Cursor position and screen size
int cursor_row = 1, cursor_col = 1;
int screen_rows, screen_cols;

// Modes
typedef enum { CONTROL_MODE, EDITOR_MODE } Mode;
Mode current_mode = CONTROL_MODE;

// Text buffer for the editor
#define MAX_LINES 1024
#define MAX_LINE_LENGTH 1024
char *lines[MAX_LINES]; // Array of lines
int total_lines = 0;

// File name
char *filename = NULL;

// Restore the original terminal settings
void disable_raw_mode() {
    write(STDOUT_FILENO, "\x1b[2J", 4); // Clear screen
    write(STDOUT_FILENO, "\x1b[H", 3);  // Move cursor to top-left
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

// Enable raw mode
void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(disable_raw_mode);

    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON);
    raw.c_oflag &= ~(OPOST);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Clear the screen and move cursor to top-left
void clear_screen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

// Get the terminal size
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

// Move the cursor
void move_cursor(int row, int col) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
    write(STDOUT_FILENO, buf, strlen(buf));
}

// Load a file into the buffer
void load_file(const char *fname) {
    FILE *file = fopen(fname, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        lines[total_lines] = strdup(line);
        total_lines++;
    }
    fclose(file);
}

// Save the buffer to the file
void save_file(const char *fname) {
    FILE *file = fopen(fname, "w");
    if (!file) {
        perror("Failed to save file");
        return;
    }

    for (int i = 0; i < total_lines; i++) {
        if (lines[i]) {
            fputs(lines[i], file);
        }
    }
    fclose(file);
    write(STDOUT_FILENO, "File saved.\r\n", 13);
}

// Draw the screen with the text buffer
void draw_screen() {
    clear_screen();
    for (int i = 0; i < screen_rows - 1 && i < total_lines; i++) {
        write(STDOUT_FILENO, lines[i], strlen(lines[i]));
    }
    move_cursor(cursor_row, cursor_col);
}

// Handle key input in editor mode
void editor_input(char c) {
    if (cursor_row - 1 >= total_lines) {
        lines[cursor_row - 1] = calloc(MAX_LINE_LENGTH, sizeof(char));
        total_lines++;
    }

    int line_len = strlen(lines[cursor_row - 1]);
    if (line_len < MAX_LINE_LENGTH - 1) {
        lines[cursor_row - 1][line_len] = c;
        lines[cursor_row - 1][line_len + 1] = '\0';
        cursor_col++;
    }
    draw_screen();
}

// Handle control commands
void process_control_command(char c) {
    if (c == ':') { // Wait for next input
        char cmd[3];
        if (read(STDIN_FILENO, cmd, 2) == 2) {
            if (strncmp(cmd, "w", 1) == 0) { // Save
                save_file(filename);
            } else if (strncmp(cmd, "wq", 2) == 0) { // Save and quit
                save_file(filename);
                exit(0);
            }
        }
    }
}

// Handle input
void process_input() {
    char c;
    while (1) {
        if (read(STDIN_FILENO, &c, 1) == -1) continue;

        if (current_mode == CONTROL_MODE) {
            if (c == 'q') break;
            if (c == '\x1b') { // ESC key
                char seq[2];
                if (read(STDIN_FILENO, seq, 1) == 1 && seq[0] == 'i') {
                    current_mode = EDITOR_MODE; // Switch to editor mode
                }
            } else {
                process_control_command(c);
            }
        } else if (current_mode == EDITOR_MODE) {
            if (c == '\x1b') { // ESC key to switch back to control mode
                current_mode = CONTROL_MODE;
            } else {
                editor_input(c);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    enable_raw_mode();
    get_terminal_size();

    if (argc > 1) {
        filename = argv[1];
        load_file(filename);
    }

    draw_screen();
    process_input();

    return 0;
}
