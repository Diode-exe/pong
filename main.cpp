extern "C" {
    // This is the entry point function that multiboot.asm calls
    void kernel_main();
}

// 32-bit flat memory address for VGA memory (no segments or far pointers)
volatile unsigned char* const vga_memory = (volatile unsigned char*)0xA0000;

// Game State Variables (Standard 32-bit ints)
int ball_x = 10;
int ball_y = 50;
int ball_dir = 1;

int right_paddle_y = 90;
int left_paddle_y = 90;

int left_paddle_direction = 0; // -1 for up, 1 for down, 0 for stationary
int right_paddle_direction = 0; // -1 for up, 1 for down, 0 for stationary

void draw_pixel(int x, int y, unsigned char color) {
    if (x >= 0 && x < 320 && y >= 0 && y < 200) {
        vga_memory[y * 320 + x] = color;
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
        ball_x += ball_dir;

        // if (ball_x >= 310) ball_dir = -1;
        // if (ball_x <= 10)  ball_dir = 1;

        if (ball_y >= right_paddle_y && ball_y <= right_paddle_y + 20 && ball_x >= 299 && ball_x <= 310) {
            ball_dir = -1;
        }

        if (ball_y >= left_paddle_y && ball_y <= left_paddle_y + 20 && ball_x >= 10 && ball_x <= 21) {
            ball_dir = 1;
        }

        // Render Ball
        draw_pixel(ball_x, ball_y, 0x0F);

        delay();
    }
}