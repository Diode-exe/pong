// compiler guards suck

#ifndef IO_BYTE_H
#define IO_BYTE_H

void out_byte(int port, int value) {
      __asm__ volatile ("outb %0, %1" : : "a"((unsigned char)value), "Nd"((unsigned short)port));
}

int in_byte(int port) {
      unsigned char ret;
      __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"((unsigned short)port));
      return ret;
}

#endif // IO_BYTE_H