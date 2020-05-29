.section "text.fastcode"

.global qspi_memcpy_src_unaligned_asm

/*
 * C Equivalent code:
 * void qspi_memcpy_src_unaligned(u8_t *dst_addr, u8_t *src_addr, u32_t size)
 * {
 *     register u32_t reg[11];
 *     u32_t i, reg_n_shift, reg_n_plus_1_shift;
 *
 *     // If src is unaligned load the registers and shift them to align
 *     // with the dest address
 *     if (src_addr & 0x3) {
 *         reg_n_shift = (src_addr) & 0x3;
 *         reg_n_plus_1_shift = ~reg_n_shift + 1;
 *         reg_n_shift <<= 3;
 *         reg_n_plus_1_shift <<= 3;
 *         for (; size ; size -= 32) {
 *             load_multiple_regs(&regs, 11, src_addr & ~0x3);
 *             src_addr += 32;
 *             for (i = 0; i < 10; i++) {
 *                 regs[i] << reg_n_shift;
 *                 regs[i] |= regs[i+1] >> reg_n_plus_1_shift;
 *             }
 *
 *             if (size < 32)
 *                 break;
 *             store_multiple_regs(&regs, , dst_addr);
 *             dst_addr += 32;
 *         }
 *        if (size == 0) {
 *             return;
 *         }
 *     } else {
 *         for (; size; size -= 32) {
 *             load_multiple_regs(&regs, 10, src_addr);
 *             src_addr += 32;
 *
 *             if (size < 32)
 *                 break;
 *
 *             store_multiple_regs(&regs, 10, dst_addr);
 *             dst_addr += 32;
 *         }
 *         if (size == 0) {
 *             return;
 *         }
 *     }
 *
 *     // Transfer the last n bytes (when n == size % 32)
 *     load_multiple_regs(&regs, 10, src_addr);
 *     switch (size) {
 *         case 36: store_multiple_regs(&regs, 9, dst_addr); return;
 *         case 32: store_multiple_regs(&regs, 8, dst_addr); return;
 *         case 28: store_multiple_regs(&regs, 7, dst_addr); return;
 *         case 24: store_multiple_regs(&regs, 6, dst_addr); return;
 *         case 20: store_multiple_regs(&regs, 5, dst_addr); return;
 *         case 16: store_multiple_regs(&regs, 4, dst_addr); return;
 *         case 12: store_multiple_regs(&regs, 3, dst_addr); return;
 *         case 8: store_multiple_regs(&regs, 2, dst_addr); return;
 *         case 4: store_multiple_regs(&regs, 1, dst_addr); return;
 *         default: return;
 *     }
 * }
 */

qspi_memcpy_src_unaligned_asm:
	push {r4-r12, r14}

	/* Check src addr alignment */
	tst r1, #0x3
	beq _src_aligned
	b _src_unaligned

_src_aligned:
	/* Load 32 bytes */
	ldmia r1!, {r3-r10}

	/* Store min(32, remaining bytes) */
	cmp r2, #32
	/* Branch to the code that will store the last size % 32 bytes */
	blt _store_mod_32_bytes
	stmia r0!, {r3-r10}
	sub r2, r2, #32
	beq _done
	b _src_aligned

_src_unaligned:

	/* Load 32 bytes from a unaligned address */
	push {r2}
	/* Save src % 4 in r12 */
	and r12, r1, #0x3
	/* Save 4 - (src % 4) in r2 */
	eor r2, r12, #0x3
	add r2, r2, #1
	bic r2, #0x4

	/* Round down src_addr to 4 byte aligned value */
	bic r14, r1, #0x3

	/* Load 36 bytes from (src_addr & ~0x3)
	 * This will ensure the 32 bytes needed
	 * are in regs r3 - r11
	 */
	ldmia r14, {r3-r11}

	/* Now, shift the data in the registers (assuming little endian mode) */
	lsl r12, r12, #3/* # of bits R[n] should be shifted right (LE mode) */
	lsl r2, r2, #3 	/* # of bits R[n+1] should be shifted left (LE mode) */

	/* r14 will be the temp register */
	lsr r3, r3, r12
	mov r14, r4
	lsl r14, r14, r2
	orr r3, r3, r14

	lsr r4, r4, r12
	mov r14, r5
	lsl r14, r14, r2
	orr r4, r4, r14

	lsr r5, r5, r12
	mov r14, r6
	lsl r14, r14, r2
	orr r5, r5, r14

	lsr r6, r6, r12
	mov r14, r7
	lsl r14, r14, r2
	orr r6, r6, r14

	lsr r7, r7, r12
	mov r14, r8
	lsl r14, r14, r2
	orr r7, r7, r14

	lsr r8, r8, r12
	mov r14, r9
	lsl r14, r14, r2
	orr r8, r8, r14

	lsr r9, r9, r12
	mov r14, r10
	lsl r14, r14, r2
	orr r9, r9, r14

	lsr r10, r10, r12
	mov r14, r11
	lsl r14, r14, r2
	orr r10, r10, r14

	pop {r2}
	add r1, r1, #32

	/* Store min(32, remaining bytes) */
	cmp r2, #32
	/* Branch to the code that will store the last size % 32 bytes */
	blt _store_mod_32_bytes
	stmia r0!, {r3-r10}
	sub r2, r2, #32
	beq _done
	b _src_unaligned
_done:
	pop {r4-r12, r14}
	bx lr

_store_mod_32_bytes:
	cmp r2, #4
	beq _copy_4

	cmp r2, #8
	beq _copy_8

	cmp r2, #12
	beq _copy_12

	cmp r2, #16
	beq _copy_16

	cmp r2, #20
	beq _copy_20

	cmp r2, #24
	beq _copy_24

	cmp r2, #28
	beq _copy_28

	b _done

_copy_4:
	stmia r0!, {r3}
	beq _done

_copy_8:
	stmia r0!, {r3-r4}
	beq _done

_copy_12:
	stmia r0!, {r3-r5}
	beq _done

_copy_16:
	stmia r0!, {r3-r6}
	beq _done

_copy_20:
	stmia r0!, {r3-r7}
	beq _done

_copy_24:
	stmia r0!, {r3-r8}
	beq _done

_copy_28:
	stmia r0!, {r3-r9}
	beq _done
