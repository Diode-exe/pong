section .multiboot
align 4
    dd 0x1BADB002            ; Magic number
    dd 0x00000004            ; Flag bit 2: Request graphics mode information
    dd - (0x1BADB002 + 0x00000004) ; Checksum

    ; --- ADDRESS FIELDS PLACEHOLDERS ---
    ; These 5 fields must be present as padding so the graphics fields 
    ; line up at the exact memory offsets GRUB expects.
    dd 0                     ; header_addr
    dd 0                     ; load_addr
    dd 0                     ; load_end_addr
    dd 0                     ; bss_end_addr
    dd 0                     ; entry_addr

    ; --- GRAPHICS LAYOUT PARAMETERS ---
    dd 0                     ; Mode type (0 = linear graphics mode)
    dd 320                   ; Width
    dd 200                   ; Height
    dd 8                     ; Depth (8-bit color index palette)

section .text
extern kernel_main
global _start
_start:
    cli
    call kernel_main
    hlt