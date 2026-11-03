/*
 * 8086 FAT16 MBR for a CF image moved between different BIOS geometries.
 * Load the first primary partition using its LBA start and INT 13h AH=08
 * geometry. Patch the in-memory FAT boot sector's geometry before ELKS runs.
 * Only the loaded copy is patched; no sector is written by this boot code.
 * Copyright (c) 2026 Alex Moir and contributors. GPL-2.0-or-later.
 */
.code16
.arch i8086, nojumps
.text
.global _start
_start:
    cli
    xor %ax,%ax
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%ss
    mov $0x7c00,%sp
    mov $0x7c00,%si
    mov $0x0600,%di
    mov $256,%cx
    cld
    rep movsw
    mov $0x0060,%ax
    push %ax
    mov $relocated,%ax
    push %ax
    lret
relocated:
    mov %cs,%ax
    mov %ax,%ds
    mov %dl,drive
    sti
    /* TGA-compatible 80-column text, even if XUB used a 40-column menu. */
    mov $3,%ax
    int $0x10
    mov drive,%dl
    xor %di,%di
    mov $8,%ah
    push %ds
    push %es
    int $0x13
    pop %es
    pop %ds
    jc error
    xor %ax,%ax
    mov %cl,%al
    and $63,%al
    jz error
    mov %ax,sectors
    xor %ax,%ax
    mov %dh,%al
    inc %ax
    cmp $255,%ax
    ja error
    mov %ax,heads
    mulw sectors
    mov %ax,cylinder_sectors
retry:
    mov partition+8,%ax
    mov partition+10,%dx
    divw cylinder_sectors
    cmp $1023,%ax
    ja error
    mov %al,%ch
    mov %ah,%cl
    ror %cl
    ror %cl
    and $0xc0,%cl
    mov %dx,%ax
    xor %dx,%dx
    divw sectors
    mov %al,%dh
    or %dl,%cl
    inc %cl
    mov drive,%dl
    mov $0x7c00,%bx
    mov $0x0201,%ax
    push %ds
    push %es
    int $0x13
    pop %es
    pop %ds
    jnc loaded
    xor %ax,%ax
    mov drive,%dl
    int $0x13
    decb retries
    jnz retry
    jmp error
loaded:
    cmpw $0xaa55,%es:0x7dfe
    jne error
    mov sectors,%ax
    mov %ax,%es:0x7c18
    mov heads,%ax
    mov %ax,%es:0x7c1a
    mov drive,%dl
    xor %ax,%ax
    mov %ax,%ds
    ljmp $0,$0x7c00
error:
    mov $message,%si
    cld
print:
    lodsb
    test %al,%al
    jz halt
    mov $0x0e,%ah
    mov $7,%bx
    int $0x10
    jmp print
halt:
    sti
    hlt
    jmp halt
drive: .byte 0
retries: .byte 3
sectors: .word 0
heads: .word 0
cylinder_sectors: .word 0
message: .asciz "ELKS CF: BIOS disk read failed"
.org 446
partition: .space 64
.word 0xaa55
