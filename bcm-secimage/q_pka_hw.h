/*******************************************************************
*
* Copyright 2006
* Broadcom Corporation
* 16215 Alton Parkway
* PO Box 57013
* Irvine CA 92619-7013
*
* Broadcom provides the current code only to licensees who
* may have the non-exclusive right to use or modify this
* code according to the license.
* This program may be not sold, resold or disseminated in
* any form without the explicit consent from Broadcom Corp
*
*******************************************************************/
/*!
******************************************************************
* \file
*   q_pka_hw.h
*
* \brief
*   header file for PKA hardware access functions
*
* \author
*   Charles Qi (x2-8439)
*
* \date
*   Originated: 12/15/2006
*   Updated: 10/24/2007
*
* \version
*   0.2
*
******************************************************************
*/

#ifndef _Q_PKA_HW_H_
#define _Q_PKA_HW_H_


/* define all LIRs */
#define PKA_EOS     0x80000000

#define PKA_NULL    0x000

/* 64-bit */
#define PKA_LIR_A   0x100

/* 128-bit */
#define PKA_LIR_B   0x200

/* 256-bit */
#define PKA_LIR_C   0x300

/* 512-bit */
#define PKA_LIR_D   0x400

/* 768-bit */
#define PKA_LIR_E   0x500

/* 1024-bit */
#define PKA_LIR_F   0x600

/* 1536-bit */
#define PKA_LIR_G   0x700

/* 2048-bit */
#define PKA_LIR_H   0x800

/* 3072-bit */
#define PKA_LIR_I   0x900

/* 4096-bit */
#define PKA_LIR_J   0xA00

/* 6144-bit */
#define PKA_LIR_K   0xB00

/* 8192-bit */
#define PKA_LIR_L   0xC00

/* 12288-bit */
#define PKA_LIR_M   0xD00

/* 16384-bit */
#define PKA_LIR_N   0xE00

#define PKA_LIR(type, index) \
  ((type) | (index))

/* define Opcodes */
#define PKA_LAST_OP     0x80000000

#define PKA_OP_MTLIR    0x01000000
#define PKA_OP_MFLIR    0x02000000
#define PKA_OP_MTLIRI   0x03000000
#define PKA_OP_MFLIRI   0x04000000
#define PKA_OP_CLIR     0x05000000
#define PKA_OP_SLIR     0x06000000
#define PKA_OP_NLIR     0x07000000
#define PKA_OP_MOVE     0x08000000
#define PKA_OP_RESIZE   0x09000000

#define PKA_OP_MODADD   0x10000000
#define PKA_OP_MODSUB   0x11000000
#define PKA_OP_MODREM   0x12000000
#define PKA_OP_MODMUL   0x13000000
#define PKA_OP_MODSQR   0x14000000
#define PKA_OP_MODEXP   0x15000000
#define PKA_OP_MODINV   0x16000000
#define PKA_OP_MODDIV2  0x17000000

#define PKA_OP_LCMP     0x20000000
#define PKA_OP_LADD     0x21000000
#define PKA_OP_LSUB     0x22000000
#define PKA_OP_LMUL     0x23000000
#define PKA_OP_LSQR     0x24000000
#define PKA_OP_LDIV     0x25000000
#define PKA_OP_LMUL2N   0x26000000
#define PKA_OP_LDIV2N   0x27000000
#define PKA_OP_LMOD2N   0x28000000
#define PKA_OP_SMUL     0x29000000
#define PKA_OP_PMADD    0x30000000
#define PKA_OP_PMSUB    0x31000000
#define PKA_OP_PMMUL    0x32000000

#define PKA_OP_PPSEL    0x40000000

typedef struct opcode {
 uint32_t op1;
 uint32_t op2;
 uint32_t *ptr;
} opcode_t;

void q_pka_hw_rst ();
/*!< reset PKA hardware 
*/

q_status_t q_pka_zeroize_mem (q_lip_ctx_t *ctx);
/*!< Zeroize PKA internal memory using a data loading sequence
    \param ctx QLIP context pointer
*/

uint32_t q_pka_sel_LIR (q_size_t size);
/*!< Determine the type of LIR to use from the data size.
    This function returns a LIR enumeration.
    \param size data size in 32-bit words
*/

uint32_t q_pka_LIR_size (uint32_t lir);
/*!< return the size of the LIR register from the enumeration.
    \param lir the LIR register enumeration
*/

uint32_t q_pka_hw_rd_status ();
/*!< read PKA hardware status register and return the value.
*/

void q_pka_hw_wr_status (uint32_t status);
/*!< write PKA hardware status register with input value.
    \param status the updated value to be written
*/

void q_pka_hw_write_sequence (int count, opcode_t *sequence);
/*!< write opcode sequence to PKA hardware data register.
    \param count the number of opcodes to be written
    \param sequence pointer to the opcode sequence
*/

void q_pka_hw_read_LIR (int size, uint32_t *data);
/*!< read PKA hardware data register to retrieve computation result.
    \param size the size of the data to be read
    \param data the pointer to data memory to store the read value.
*/

#define PACK_OP1(last_op, opcode, param0, param1) \
        ((last_op) | (opcode) | ((param0) << 12) | param1)

#define PACK_OP2(param0, param1) \
        (((param0) << 12) | (param1))

/* address mapped PKA registers */
#define PKA_HW_BASE_ADDR      0x01020000
#define PKA_HW_REG_CTLSTAT    (PKA_HW_BASE_ADDR+0x0)
#define PKA_HW_REG_DIN        (PKA_HW_BASE_ADDR+0x4)
#define PKA_HW_REG_DOUT       (PKA_HW_BASE_ADDR+0x8)
#define PKA_HW_REG_ACC_CTL    (PKA_HW_BASE_ADDR+0xc)

#define PKA_STAT_PPSEL_FAILED 0x00000200
#define PKA_STAT_CARRY        0x00000100
#define PKA_CTL_RST           0x00000080
#define PKA_STAT_ERROR        0x00000008
#define PKA_STAT_BUSY         0x00000004
#define PKA_STAT_DONE         0x00000002
#define PKA_CTL_EN            0x00000001

#define PKA_CTL_ESCAPE_LOC    16
#define PKA_CTL_ESCAPE_MASK   0x7
      
#define PKA_HW_RESULT         0xE00E0000
#define PKA_HW_SUCCESS        0xBABEBABE
#define PKA_HW_FAIL           0xDEADBEEF

#endif 
