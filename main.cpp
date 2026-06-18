//--------------------
// from https://github.com/dhepper/font8x8/blob/master/font8x8_basic.h
#include "font8x8_basic.h"
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

int right_paddle_y = 90;
int left_paddle_y = 90;

int left_paddle_direction = 0; // -1 for up, 1 for down, 0 for stationary
int right_paddle_direction = 0; // -1 for up, 1 for down, 0 for stationary

int right_score = 0;
int left_score = 0;

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

void delay() {
    // 32-bit CPUs execute loops insanely fast
    for (volatile int i = 0; i < 4000000; i++);
}

// Read keys directly from the motherboard's I/O port 0x60
unsigned char read_keyboard_port() {
    unsigned char result;
    __asm__ volatile("inb $0x60, %0" : "=a"(result));
    return result;
}

void kernel_main() {
    // Clear screen
    for (int i = 0; i < 320 * 200; i++) {
        vga_memory[i] = 0x00;
    }

    while (true) {
        left_paddle_direction = 0;
        right_paddle_direction = 0;

        // Clear previous ball position
        draw_pixel(ball_x, ball_y, 0x00);

        unsigned char key = read_keyboard_port();

        // Right Paddle Up (Up Arrow)
        if (key == 0x48 && right_paddle_y > 0) {
            for(int i = 0; i < 4; i++) draw_pixel(300, right_paddle_y + 16 + i, 0x00);
            right_paddle_y -= 4;
            right_paddle_direction = -1;
        }
        // Right Paddle Down (Down Arrow)
        if (key == 0x50 && right_paddle_y < 180) {
            for(int i = 0; i < 4; i++) draw_pixel(300, right_paddle_y + i, 0x00);
            right_paddle_y += 4;
            right_paddle_direction = 1;
        }

        // Left Paddle Up (W Key)
        if (key == 0x11 && left_paddle_y > 0) {
            for(int i = 0; i < 4; i++) draw_pixel(20, left_paddle_y + 16 + i, 0x00);
            left_paddle_y -= 4;
            left_paddle_direction = -1;
        }

        // Left Paddle Down (S Key)
        if (key == 0x1F && left_paddle_y < 180) {
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
        ball_x += ball_dir_x;
        ball_y += ball_dir_y;

        // if (ball_x >= 310) ball_dir_x = -1;
        // if (ball_x <= 10)  ball_dir_x = 1;

        if (ball_y >= right_paddle_y && ball_y <= right_paddle_y + 20 && ball_x >= 299 && ball_x <= 310) {
            ball_dir_x = -1;
            if (right_paddle_direction != 0) {
                ball_dir_y = right_paddle_direction; // Add vertical spin based on paddle movement
            }
        }

        if (ball_y >= left_paddle_y && ball_y <= left_paddle_y + 20 && ball_x >= 10 && ball_x <= 21) {
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
            left_score++;
        }
        if (ball_x >= 319) {
            ball_x = 160;
            ball_y = 100;
            ball_dir_x = -1;
            ball_dir_y = 1;
            right_score++;
        }

        // Render Ball
        draw_pixel(ball_x, ball_y, 0x0F);

        // Render Scores

        draw_char(140, 10, '0' + (left_score), 0x0F);
        draw_char(180, 10, '0' + (right_score), 0x0F);

        for (int i = 0; i < 200; i++) {
            vga_memory[i * 320 + 160] = 0x0F; // Draw center line
        }

        delay();
    }
}