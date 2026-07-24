//--------------------
// from https://github.com/dhepper/font8x8/blob/master/font8x8_basic.h
#include "font8x8_basic.h"
// --------------------

// --------------------
// from https://wiki.osdev.org/CMOS#Getting_Current_Date_and_Time_from_RTC
#include "rtc_utils.h"
// --------------------

extern "C" {
    // This is the entry point function that multiboot.asm calls
    void kernel_main();
}

// 32-bit flat memory address for VGA memory (no segments or far pointers)
volatile unsigned char* const vga_memory = (volatile unsigned char*)0xA0000;

// Game State Variables (Standard 32-bit ints)
int ball_x = 10;
int ball_y = 50;
int ball_dir_x = 1;
int ball_dir_y = 1;
float ball_speed = 1.0f;

int right_paddle_y = 90;
int left_paddle_y = 90;

int left_paddle_direction = 0; // -1 for up, 1 for down, 0 for stationary
int right_paddle_direction = 0; // -1 for up, 1 for down, 0 for stationary

int right_score = 0;
int left_score = 0;

bool past_start_screen = false;

bool keys[256] = {false};

bool paused = false;

void draw_pixel(int x, int y, unsigned char color) {
    if (x >= 0 && x < 320 && y >= 0 && y < 200) {
        vga_memory[y * 320 + x] = color;
    }
}

void draw_char(int x, int y, char c, unsigned char color) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (font8x8_basic[(unsigned char)c][row] & (0x80 >> col)) {
                draw_pixel(x + (7 - col), y + row, color);
            } else {
                draw_pixel(x + (7 - col), y + row, 0x00);
            }
        }
    }
}

void draw_string(int x, int y, const char* str, unsigned char color, int spacing = 0) {
    for (int i = 0; str[i] != '\0'; i++) {
        draw_char(x + i * (8 + spacing), y, str[i], color);
    }
}

void draw_start_screen() {
    draw_string(120, 80,  "PONG",                  0x0F);
    draw_string(80, 100, "First to 10 Wins!",       0x0F);
    draw_string(80, 120, "Left Paddle: W/S Keys", 0x0F);
    draw_string(80, 140, "Right Paddle: Arrow Keys", 0x0F);
    draw_string(80, 160, "Press P to Pause",            0x0F);
    draw_string(80, 180, "Press Enter To Start",  0x0F);
}

void delay_ms(unsigned short ms) {
    // 1. Enable the PIT Channel 2 gate by setting the lowest two bits of system port 0x61
    unsigned char port_61;
    __asm__ volatile("inb $0x61, %0" : "=a"(port_61));
    __asm__ volatile("outb %%al, $0x61" : : "a"((unsigned char)(port_61 | 0x01)));

    while (ms > 0) {
        // 2. Set PIT Channel 2 to Mode 0 (Interrupt on Terminal Count), Binary counter
        __asm__ volatile("outb %%al, $0x43" : : "a"((unsigned char)0xB0));

        // 3. Load the divisor for exactly 1 millisecond.
        // The PIT frequency is 1193182 Hz. For 1ms, we count down from ~1193.
        unsigned short count = 1193;
        __asm__ volatile("outb %%al, $0x42" : : "a"((unsigned char)(count & 0xFF)));        // Low byte
        __asm__ volatile("outb %%al, $0x42" : : "a"((unsigned char)((count >> 8) & 0xFF))); // High byte

        // 4. Poll the PIT status byte until the output pin goes high (bit 7 becomes 1)
        // indicating the 1ms countdown has finished.
        unsigned char status = 0;
        while (!(status & 0x80)) {
            // Send a Read-Back Command to check the status of Channel 2
            __asm__ volatile("outb %%al, $0x43" : : "a"((unsigned char)0xE8));
            __asm__ volatile("inb $0x42, %0" : "=a"(status));
        }

        ms--;
    }
}

// Read keys directly from the motherboard's I/O port 0x60
unsigned char read_keyboard_port() {
    unsigned char result;
    __asm__ volatile("inb $0x60, %0" : "=a"(result));
    return result;
}


void update_keys() {
    unsigned char sc = read_keyboard_port();
    if (sc & 0x80) {
        keys[sc & 0x7F] = false; // break code (key released)
    } else {
        keys[sc] = true;         // make code (key pressed)
    }
}

void clear_screen() {
    for (int i = 0; i < 320 * 200; i++) {
        vga_memory[i] = 0x00;
    }
}

void end_game() {
    draw_string(100, 80, "Game Over!", 0x0F);
    draw_string(80, 120, "Press Enter to Restart", 0x0F);
    while (true) {
        if (read_keyboard_port() == 0x1C) { // Enter key
            // Reset game state
            ball_x = 10;
            ball_y = 50;
            ball_dir_x = 1;
            ball_dir_y = 1;
            right_paddle_y = 90;
            left_paddle_y = 90;
            left_score = 0;
            right_score = 0;
            past_start_screen = false;
            ball_speed = 1.0f;

            clear_screen();
            break; // Exit the loop to restart the game
        }
    }
}

void game_pause() {
    draw_string(100, 80, "Game Paused", 0x0F);
    draw_string(80, 120, "Press P to Resume", 0x0F);
    while (true) {
        if (read_keyboard_port() == 0x19) { // P key
            paused = false;
            clear_screen();
            break; // Exit the loop to resume the game
        }
    }
}

void kernel_main() {
    // Clear screen
    clear_screen();

    while (!past_start_screen) {
        draw_start_screen();
        if (read_keyboard_port() == 0x1C) { // Enter key
            past_start_screen = true;
            clear_screen();
        }
    }

    while (true) {
        if (!paused) {

            left_paddle_direction = 0;
            right_paddle_direction = 0;

            // Clear previous ball position
            draw_pixel(ball_x, ball_y, 0x00);

            // unsigned char key = read_keyboard_port();
            update_keys();

            // Right Paddle Up (Up Arrow)
            if (keys[0x48] && right_paddle_y > 0) {
                for(int i = 0; i < 4; i++) draw_pixel(300, right_paddle_y + 16 + i, 0x00);
                right_paddle_y -= 4;
                right_paddle_direction = -1;
            }
            // Right Paddle Down (Down Arrow)
            if (keys[0x50] && right_paddle_y < 180) {
                for(int i = 0; i < 4; i++) draw_pixel(300, right_paddle_y + i, 0x00);
                right_paddle_y += 4;
                right_paddle_direction = 1;
            }

            // Left Paddle Up (W Key)
            if (keys[0x11] && left_paddle_y > 0) {
                for(int i = 0; i < 4; i++) draw_pixel(20, left_paddle_y + 16 + i, 0x00);
                left_paddle_y -= 4;
                left_paddle_direction = -1;
            }

            // Left Paddle Down (S Key)
            if (keys[0x1F] && left_paddle_y < 180) {
                for(int i = 0; i < 4; i++) draw_pixel(20, left_paddle_y + i, 0x00);
                left_paddle_y += 4;
                left_paddle_direction = 1;
            }

            // Render Paddles
            for (int i = 0; i < 20; i++) {
                draw_pixel(300, right_paddle_y + i, 0x0F);
                draw_pixel(20, left_paddle_y + i, 0x0F);
            }

            // Update Ball
            ball_x += ball_dir_x * ball_speed;
            ball_y += ball_dir_y * ball_speed;

            // if (ball_x >= 310) ball_dir_x = -1;
            // if (ball_x <= 10)  ball_dir_x = 1;

            if (ball_y >= right_paddle_y && ball_y <= right_paddle_y + 20 && ball_x >= 299 && ball_x <= 301) {
                ball_dir_x = -1;
                if (right_paddle_direction != 0) {
                    ball_dir_y = right_paddle_direction; // Add vertical spin based on paddle movement
                }
            }

            if (ball_y >= left_paddle_y && ball_y <= left_paddle_y + 20 && ball_x >= 19 && ball_x <= 21) {
                ball_dir_x = 1;
                if (left_paddle_direction != 0) {
                    ball_dir_y = left_paddle_direction; // Add vertical spin based on paddle movement
                }
            }

            if (ball_y <= 0 || ball_y >= 199) {
                ball_dir_y = -ball_dir_y; // Bounce off top and bottom walls
            }

            if (ball_x <= 0) {
                ball_x = 160;
                ball_y = 100;
                ball_dir_x = 1;
                ball_dir_y = 1;
                right_score++;
                ball_speed += 0.1f; // Increase ball speed after each score
            }
            if (ball_x >= 319) {
                ball_x = 160;
                ball_y = 100;
                ball_dir_x = -1;
                ball_dir_y = 1;
                left_score++;
                ball_speed += 0.1f; // Increase ball speed after each score
            }

            if (left_score >= 10 || right_score >= 10) {
                end_game();
            }

            // Render Ball
            draw_pixel(ball_x, ball_y, 0x0F);

            // Render Scores

            draw_char(140, 10, '0' + (left_score), 0x0F);
            draw_char(175, 10, '0' + (right_score), 0x0F);

            for (int i = 0; i < 200; i++) {
                vga_memory[i * 320 + 160] = 0x0F; // Draw center line
            }

            if (read_keyboard_port() == 0x19) { // P key
                paused = true;
                clear_screen();
            }

            // elegant solution
            dateTimeValues dt = read_rtc();

            draw_char(80, 180, '0' + (dt.hour / 10), 0x0F);
            draw_char(88, 180, '0' + (dt.hour % 10), 0x0F);
            draw_char(96, 180, ':', 0x0F);
            draw_char(104, 180, '0' + (dt.minute / 10), 0x0F);
            draw_char(112, 180, '0' + (dt.minute % 10), 0x0F);
            draw_char(120, 180, ':', 0x0F);
            draw_char(128, 180, '0' + (dt.second / 10), 0x0F);
            draw_char(136, 180, '0' + (dt.second % 10), 0x0F);

            draw_char(184, 180, '0' + ((dt.year / 1000) % 10), 0x0F);
            draw_char(192, 180, '0' + ((dt.year / 100) % 10), 0x0F);
            draw_char(200, 180, '0' + ((dt.year / 10) % 10), 0x0F);
            draw_char(208, 180, '0' + (dt.year % 10), 0x0F);
            draw_char(216, 180, '/', 0x0F);
            draw_char(224, 180, '0' + (dt.month / 10), 0x0F);
            draw_char(232, 180, '0' + (dt.month % 10), 0x0F);
            draw_char(240, 180, '/', 0x0F);
            draw_char(248, 180, '0' + (dt.day / 10), 0x0F);
            draw_char(256, 180, '0' + (dt.day % 10), 0x0F);

            delay_ms(16); // Approximately 60 FPS
        }

        else {
            game_pause();
            delay_ms(16); // Approximately 60 FPS
        }
    }
}
