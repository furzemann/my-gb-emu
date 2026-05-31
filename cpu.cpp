#include "cpu.h"
#include <cstdint>
#include <cstdlib>
#include <stdio.h>

uint8_t CPU::fetch8() { return mmu->read(PC++); }

uint16_t CPU::fetch16() {
  uint8_t lo = fetch8();
  uint8_t hi = fetch8();
  return lo | (hi << 8);
}

uint8_t CPU::readHL() { return mmu->read(getHL()); }

void CPU::writeHL(uint8_t v) { mmu->write(getHL(), v); }

CPU::CPU(Memory *mem) {
  mmu = mem;

  IME = false;
  EI_pending = false;

  // Initialize opcode tables to unimplemented
  for (int i = 0; i < 256; i++) {
    table[i] = &CPU::UNIMPLEMENTED;
    cbtable[i] = &CPU::UNIMPLEMENTED;
  }

  // Initialize register lookup table (used by generic LD dispatcher)
  regs[0] = &B;
  regs[1] = &C;
  regs[2] = &D;
  regs[3] = &E;
  regs[4] = &H;
  regs[5] = &L;
  regs[6] = nullptr; // (HL) - handled separately
  regs[7] = &A;

  // Initialize registers to post-boot ROM values
  A = 0x01;
  F = 0xB0;
  B = 0x00;
  C = 0x13;
  D = 0x00;
  E = 0xD8;
  H = 0x01;
  L = 0x4D;
  SP = 0xFFFE;
  PC = 0x0100;

  // === 0x00-0x0F ===
  table[0x00] = &CPU::NOP;
  table[0x01] = &CPU::LD_DISPATCH; // LD BC,d16
  table[0x02] = &CPU::LD_DISPATCH; // LD (BC),A
  table[0x03] = &CPU::LD_DISPATCH; // INC BC
  table[0x04] = &CPU::LD_DISPATCH; // INC B
  table[0x05] = &CPU::LD_DISPATCH; // DEC B
  table[0x06] = &CPU::LD_B_d8;
  table[0x07] = &CPU::RLCA;
  table[0x08] = &CPU::LD_DISPATCH; // LD (a16),SP
  table[0x09] = &CPU::LD_DISPATCH; // ADD HL,BC
  table[0x0A] = &CPU::LD_DISPATCH; // LD A,(BC)
  table[0x0B] = &CPU::LD_DISPATCH; // DEC BC
  table[0x0C] = &CPU::LD_DISPATCH; // INC C
  table[0x0D] = &CPU::LD_DISPATCH; // DEC C
  table[0x0E] = &CPU::LD_C_d8;
  table[0x0F] = &CPU::RRCA;

  // === 0x10-0x1F ===
  table[0x10] = &CPU::NOP;         // STOP (treated as NOP for now)
  table[0x11] = &CPU::LD_DISPATCH; // LD DE,d16
  table[0x12] = &CPU::LD_DISPATCH; // LD (DE),A
  table[0x13] = &CPU::LD_DISPATCH; // INC DE
  table[0x14] = &CPU::LD_DISPATCH; // INC D
  table[0x15] = &CPU::LD_DISPATCH; // DEC D
  table[0x16] = &CPU::LD_DISPATCH; // LD D,d8
  table[0x17] = &CPU::RLA;
  table[0x18] = &CPU::LD_DISPATCH; // JR r8
  table[0x19] = &CPU::LD_DISPATCH; // ADD HL,DE
  table[0x1A] = &CPU::LD_DISPATCH; // LD A,(DE)
  table[0x1B] = &CPU::LD_DISPATCH; // DEC DE
  table[0x1C] = &CPU::LD_DISPATCH; // INC E
  table[0x1D] = &CPU::LD_DISPATCH; // DEC E
  table[0x1E] = &CPU::LD_DISPATCH; // LD E,d8
  table[0x1F] = &CPU::RRA;

  // === 0x20-0x2F ===
  table[0x20] = &CPU::LD_DISPATCH; // JR NZ,r8
  table[0x21] = &CPU::LD_DISPATCH; // LD HL,d16
  table[0x22] = &CPU::LD_DISPATCH; // LD (HL+),A
  table[0x23] = &CPU::LD_DISPATCH; // INC HL
  table[0x24] = &CPU::LD_DISPATCH; // INC H
  table[0x25] = &CPU::LD_DISPATCH; // DEC H
  table[0x26] = &CPU::LD_DISPATCH; // LD H,d8
  table[0x27] = &CPU::LD_DISPATCH; // DAA
  table[0x28] = &CPU::LD_DISPATCH; // JR Z,r8
  table[0x29] = &CPU::LD_DISPATCH; // ADD HL,HL
  table[0x2A] = &CPU::LD_DISPATCH; // LD A,(HL+)
  table[0x2B] = &CPU::LD_DISPATCH; // DEC HL
  table[0x2C] = &CPU::LD_DISPATCH; // INC L
  table[0x2D] = &CPU::LD_DISPATCH; // DEC L
  table[0x2E] = &CPU::LD_DISPATCH; // LD L,d8
  table[0x2F] = &CPU::LD_DISPATCH; // CPL

  // === 0x30-0x3F ===
  table[0x30] = &CPU::LD_DISPATCH; // JR NC,r8
  table[0x31] = &CPU::LD_DISPATCH; // LD SP,d16
  table[0x32] = &CPU::LD_DISPATCH; // LD (HL-),A
  table[0x33] = &CPU::LD_DISPATCH; // INC SP
  table[0x34] = &CPU::LD_DISPATCH; // INC (HL)
  table[0x35] = &CPU::LD_DISPATCH; // DEC (HL)
  table[0x36] = &CPU::LD_DISPATCH; // LD (HL),d8
  table[0x37] = &CPU::LD_DISPATCH; // SCF
  table[0x38] = &CPU::LD_DISPATCH; // JR C,r8
  table[0x39] = &CPU::LD_DISPATCH; // ADD HL,SP
  table[0x3A] = &CPU::LD_DISPATCH; // LD A,(HL-)
  table[0x3B] = &CPU::LD_DISPATCH; // DEC SP
  table[0x3C] = &CPU::LD_DISPATCH; // INC A
  table[0x3D] = &CPU::LD_DISPATCH; // DEC A
  table[0x3E] = &CPU::LD_A_d8;
  table[0x3F] = &CPU::LD_DISPATCH; // CCF

  // === 0x40-0x7F: LD r,r and HALT ===
  for (int i = 0x40; i <= 0x7F; i++) {
    if (i == 0x76) {
      table[i] = &CPU::HALT_OP; // HALT (treated as NOP for now)
    } else {
      table[i] = &CPU::LD_DISPATCH;
    }
  }

  // === 0x80-0xBF: ALU operations ===
  table[0x80] = &CPU::ADD_A_B;
  table[0x81] = &CPU::ADD_A_C;
  for (int i = 0x80; i <= 0xBF; i++) {
    table[i] = &CPU::ALU_DISPATCH;
  }

  // === 0xC0-0xFF: Control flow, stack, I/O ===
  table[0xC0] = &CPU::LD_DISPATCH; // RET NZ
  table[0xC1] = &CPU::POP_BC;
  table[0xC2] = &CPU::LD_DISPATCH; // JP NZ,a16
  table[0xC3] = &CPU::JP_a16;
  table[0xC4] = &CPU::LD_DISPATCH; // CALL NZ,a16
  table[0xC5] = &CPU::PUSH_BC;
  table[0xC6] = &CPU::ADD_A_d8;
  table[0xC7] = &CPU::LD_DISPATCH; // RST 00H
  table[0xC8] = &CPU::LD_DISPATCH; // RET Z
  table[0xC9] = &CPU::RET_OP;
  table[0xCA] = &CPU::LD_DISPATCH; // JP Z,a16
  table[0xCB] = &CPU::CB_DISPATCH; // PREFIX CB
  table[0xCC] = &CPU::LD_DISPATCH; // CALL Z,a16
  table[0xCD] = &CPU::CALL_a16;
  table[0xCE] = &CPU::LD_DISPATCH; // ADC A,d8
  table[0xCF] = &CPU::LD_DISPATCH; // RST 08H

  table[0xD0] = &CPU::LD_DISPATCH; // RET NC
  table[0xD1] = &CPU::POP_DE;
  table[0xD2] = &CPU::LD_DISPATCH; // JP NC,a16
  table[0xD4] = &CPU::LD_DISPATCH; // CALL NC,a16
  table[0xD5] = &CPU::PUSH_DE;
  table[0xD6] = &CPU::SUB_d8;
  table[0xD7] = &CPU::LD_DISPATCH; // RST 10H
  table[0xD8] = &CPU::LD_DISPATCH; // RET C
  table[0xD9] = &CPU::LD_DISPATCH; // RETI
  table[0xDA] = &CPU::LD_DISPATCH; // JP C,a16
  table[0xDC] = &CPU::LD_DISPATCH; // CALL C,a16
  table[0xDE] = &CPU::LD_DISPATCH; // SBC A,d8
  table[0xDF] = &CPU::LD_DISPATCH; // RST 18H

  table[0xE0] = &CPU::LD_DISPATCH; // LDH (a8),A
  table[0xE1] = &CPU::POP_HL;
  table[0xE2] = &CPU::LD_DISPATCH; // LD (C),A
  table[0xE3] = &CPU::LD_DISPATCH; // EX (SP),HL
  table[0xE5] = &CPU::PUSH_HL;
  table[0xE6] = &CPU::AND_d8;
  table[0xE7] = &CPU::LD_DISPATCH; // RST 20H
  table[0xE8] = &CPU::LD_DISPATCH; // ADD SP,r8
  table[0xE9] = &CPU::LD_DISPATCH; // JP (HL)
  table[0xEA] = &CPU::LD_DISPATCH; // LD (a16),A
  table[0xEB] = &CPU::LD_DISPATCH; // EX DE,HL
  table[0xEE] = &CPU::XOR_d8;
  table[0xEF] = &CPU::LD_DISPATCH; // RST 28H

  table[0xF0] = &CPU::LD_DISPATCH; // LDH A,(a8)
  table[0xF1] = &CPU::POP_AF;
  table[0xF2] = &CPU::LD_DISPATCH; // LD A,(C)
  table[0xF3] = &CPU::LD_DISPATCH; // DI
  table[0xF5] = &CPU::PUSH_AF;
  table[0xF6] = &CPU::OR_d8;
  table[0xF7] = &CPU::LD_DISPATCH; // RST 30H
  table[0xF8] = &CPU::LD_DISPATCH; // LD HL,SP+r8
  table[0xF9] = &CPU::LD_DISPATCH; // LD SP,HL
  table[0xFA] = &CPU::LD_DISPATCH; // LD A,(a16)
  table[0xFB] = &CPU::LD_DISPATCH; // EI
  table[0xFE] = &CPU::CP_d8;
  table[0xFF] = &CPU::LD_DISPATCH; // RST 38H

  // === CB-prefixed opcodes ===
  // RLC, RRC, RL, RR, SLA, SRA, SWAP, SRL
  for (int i = 0x00; i <= 0x3F; i++) {
    cbtable[i] = &CPU::CB_DISPATCH;
  }

  // BIT
  for (int i = 0x40; i <= 0x7F; i++) {
    cbtable[i] = &CPU::BIT_DISPATCH;
  }

  // RES
  for (int i = 0x80; i <= 0xBF; i++) {
    cbtable[i] = &CPU::RES_DISPATCH;
  }

  // SET
  for (int i = 0xC0; i <= 0xFF; i++) {
    cbtable[i] = &CPU::SET_DISPATCH;
  }

  cbtable[0x11] = &CPU::RL_C;
}

int CPU::step() {

  // delayed EI enable
  if (justEnabledEI) {
    IME = true;
    justEnabledEI = false;
  }

  uint8_t IF = mmu->read(0xFF0F);
  uint8_t IE = mmu->read(0xFFFF);
  uint8_t pending = IF & IE;

  // HALT handling
  if (halted) {

    if (!pending)
      return 4;

    halted = false;
  }

  // interrupts
  if (IME && pending) {

    for (int i = 0; i < 5; i++) {

      if (pending & (1 << i)) {

        IME = false;

        IF &= ~(1 << i);
        mmu->write(0xFF0F, IF);

        PUSH(PC);

        PC = 0x40 + (i * 8);

        return 20;
      }
    }
  }

  uint8_t opcode = fetch8();

  int cycles = (this->*table[opcode])();

  // delayed EI scheduling
  if (EI_pending) {
    justEnabledEI = true;
    EI_pending = false;
  }

  return cycles;
}

// === Basic opcodes ===
int CPU::NOP() { return 4; }

int CPU::LD_A_d8() {
  A = fetch8();
  return 8;
}

int CPU::HALT_OP() {
  halted = true;
  return 4;
}

int CPU::LD_B_d8() {
  B = fetch8();
  return 8;
}

int CPU::LD_C_d8() {
  C = fetch8();
  return 8;
}

int CPU::ADD_A_B() {
  ADD(B);
  return 4;
}

int CPU::ADD_A_C() {
  ADD(C);
  return 4;
}

int CPU::ADD_A_d8() {
  ADD(fetch8());
  return 8;
}

int CPU::SUB_B() {
  SUB(B);
  return 4;
}

int CPU::SUB_d8() {
  SUB(fetch8());
  return 8;
}

int CPU::AND_d8() {
  AND(fetch8());
  return 8;
}

int CPU::XOR_d8() {
  XOR(fetch8());
  return 8;
}

int CPU::OR_d8() {
  OR(fetch8());
  return 8;
}

int CPU::CP_d8() {
  CP(fetch8());
  return 8;
}

int CPU::JP_a16() {
  JP(fetch16());
  return 16;
}

int CPU::CALL_a16() {
  CALL(fetch16());
  return 24;
}

int CPU::RET_OP() {
  RET();
  return 16;
}

int CPU::RL_C() {
  RL(C);
  return 8;
}

// === Stack operations ===
int CPU::PUSH_BC() {
  PUSH(getBC());
  return 16;
}

int CPU::POP_BC() {
  setBC(POP());
  return 12;
}

int CPU::PUSH_DE() {
  PUSH(getDE());
  return 16;
}

int CPU::POP_DE() {
  setDE(POP());
  return 12;
}

int CPU::PUSH_HL() {
  PUSH(getHL());
  return 16;
}

int CPU::POP_HL() {
  setHL(POP());
  return 12;
}

int CPU::PUSH_AF() {
  PUSH(getAF());
  return 16;
}

int CPU::POP_AF() {
  setAF(POP());
  return 12;
}

// === Accumulator rotate/shift ===
int CPU::RLCA() {
  bool carry = A & 0x80;
  A = (A << 1) | carry;
  setC(carry);
  setZ(false); // RLCA clears Z
  setN(false);
  setH(false);
  return 4;
}

int CPU::RRCA() {
  bool carry = A & 1;
  A = (A >> 1) | (carry << 7);
  setC(carry);
  setZ(false); // RRCA clears Z
  setN(false);
  setH(false);
  return 4;
}

int CPU::RLA() {
  bool oldCarry = getC();
  bool newCarry = A & 0x80;
  A = (A << 1) | oldCarry;
  setC(newCarry);
  setZ(false); // RLA clears Z
  setN(false);
  setH(false);
  return 4;
}

int CPU::RRA() {
  bool oldCarry = getC();
  bool newCarry = A & 1;
  A = (A >> 1) | (oldCarry << 7);
  setC(newCarry);
  setZ(false); // RRA clears Z
  setN(false);
  setH(false);
  return 4;
}

// === Generic dispatchers ===
int CPU::LD_DISPATCH() {
  uint8_t opcode = mmu->read(PC - 1);

  // LD r,r' (0x40-0x7F, excluding 0x76 HALT)
  if (opcode >= 0x40 && opcode <= 0x7F) {
    int dst = (opcode >> 3) & 7;
    int src = opcode & 7;

    if (src == 6) { // LD r,(HL)
      if (dst == 6)
        return 4; // HALT handled elsewhere
      *regs[dst] = readHL();
      return 8;
    } else if (dst == 6) { // LD (HL),r
      writeHL(*regs[src]);
      return 8;
    } else { // LD r,r'
      *regs[dst] = *regs[src];
      return 4;
    }
  }

  // LD r,d8
  if ((opcode & 0xC7) == 0x06) {
    int reg = (opcode >> 3) & 7;
    uint8_t val = fetch8();
    if (reg == 6) {
      writeHL(val);
    } else {
      *regs[reg] = val;
    }
    return (reg == 6) ? 12 : 8;
  }

  // LD rr,d16
  if ((opcode & 0xCF) == 0x01) {
    uint16_t val = fetch16();
    switch ((opcode >> 4) & 3) {
    case 0:
      setBC(val);
      break;
    case 1:
      setDE(val);
      break;
    case 2:
      setHL(val);
      break;
    case 3:
      SP = val;
      break;
    }
    return 12;
  }

  // INC r
  if ((opcode & 0xC7) == 0x04) {
    int reg = (opcode >> 3) & 7;
    if (reg == 6) {
      uint8_t val = readHL();
      INC8(val);
      writeHL(val);
      return 12;
    } else {
      INC8(*regs[reg]);
      return 4;
    }
  }

  // DEC r
  if ((opcode & 0xC7) == 0x05) {
    int reg = (opcode >> 3) & 7;
    if (reg == 6) {
      uint8_t val = readHL();
      DEC8(val);
      writeHL(val);
      return 12;
    } else {
      DEC8(*regs[reg]);
      return 4;
    }
  }

  // INC rr
  if ((opcode & 0xCF) == 0x03) {
    switch ((opcode >> 4) & 3) {
    case 0: {
      uint16_t bc = getBC();
      INC16(bc);
      setBC(bc);
      break;
    }
    case 1: {
      uint16_t de = getDE();
      INC16(de);
      setDE(de);
      break;
    }
    case 2: {
      uint16_t hl = getHL();
      INC16(hl);
      setHL(hl);
      break;
    }
    case 3:
      INC16(SP);
      break;
    }
    return 8;
  }

  // DEC rr
  if ((opcode & 0xCF) == 0x0B) {
    switch ((opcode >> 4) & 3) {
    case 0: {
      uint16_t bc = getBC();
      DEC16(bc);
      setBC(bc);
      break;
    }
    case 1: {
      uint16_t de = getDE();
      DEC16(de);
      setDE(de);
      break;
    }
    case 2: {
      uint16_t hl = getHL();
      DEC16(hl);
      setHL(hl);
      break;
    }
    case 3:
      DEC16(SP);
      break;
    }
    return 8;
  }

  // Additional LD variants
  switch (opcode) {
  case 0x02:
    mmu->write(getBC(), A);
    return 8; // LD (BC),A
  case 0x12:
    mmu->write(getDE(), A);
    return 8; // LD (DE),A
  case 0x22:
    writeHL(A);
    {
      uint16_t hl = getHL();
      hl++;
      setHL(hl);
    }
    return 8; // LD (HL+),A
  case 0x32:
    writeHL(A);
    {
      uint16_t hl = getHL();
      hl--;
      setHL(hl);
    }
    return 8; // LD (HL-),A

  case 0x0A:
    A = mmu->read(getBC());
    return 8; // LD A,(BC)
  case 0x1A:
    A = mmu->read(getDE());
    return 8; // LD A,(DE)
  case 0x2A:
    A = readHL();
    {
      uint16_t hl = getHL();
      hl++;
      setHL(hl);
    }
    return 8; // LD A,(HL+)
  case 0x3A:
    A = readHL();
    {
      uint16_t hl = getHL();
      hl--;
      setHL(hl);
    }
    return 8; // LD A,(HL-)

  case 0x08: { // LD (a16),SP
    uint16_t addr = fetch16();
    mmu->write(addr, SP & 0xFF);
    mmu->write(addr + 1, (SP >> 8) & 0xFF);
    return 20;
  }

  case 0xE0:
    mmu->write(0xFF00 + fetch8(), A);
    return 12; // LDH (a8),A
  case 0xF0:
    A = mmu->read(0xFF00 + fetch8());
    return 12; // LDH A,(a8)
  case 0xE2:
    mmu->write(0xFF00 + C, A);
    return 8; // LD (C),A
  case 0xF2:
    A = mmu->read(0xFF00 + C);
    return 8; // LD A,(C)
  case 0xEA:
    mmu->write(fetch16(), A);
    return 16; // LD (a16),A
  case 0xFA:
    A = mmu->read(fetch16());
    return 16; // LD A,(a16)

  case 0xF9:
    SP = getHL();
    return 8;  // LD SP,HL
  case 0xF8: { // LD HL,SP+r8
    int8_t offset = (int8_t)fetch8();
    uint16_t result = SP + offset;
    setZ(false);
    setN(false);
    setH(((SP & 0xF) + (offset & 0xF)) > 0xF);
    setC(((SP & 0xFF) + (offset & 0xFF)) > 0xFF);
    setHL(result);
    return 12;
  }
  }

  // Jump instructions
  switch (opcode) {
  case 0x18:
    JR((int8_t)fetch8());
    return 12; // JR r8
  case 0x20: { // JR NZ,r8
    int8_t offset = (int8_t)fetch8();
    if (!getZ()) {
      JR(offset);
      return 12;
    }
    return 8;
  }
  case 0x28: { // JR Z,r8
    int8_t offset = (int8_t)fetch8();
    if (getZ()) {
      JR(offset);
      return 12;
    }
    return 8;
  }
  case 0x30: { // JR NC,r8
    int8_t offset = (int8_t)fetch8();
    if (!getC()) {
      JR(offset);
      return 12;
    }
    return 8;
  }
  case 0x38: { // JR C,r8
    int8_t offset = (int8_t)fetch8();
    if (getC()) {
      JR(offset);
      return 12;
    }
    return 8;
  }

  case 0xC2: { // JP NZ,a16
    uint16_t addr = fetch16();
    if (!getZ()) {
      JP(addr);
      return 16;
    }
    return 12;
  }
  case 0xCA: { // JP Z,a16
    uint16_t addr = fetch16();
    if (getZ()) {
      JP(addr);
      return 16;
    }
    return 12;
  }
  case 0xD2: { // JP NC,a16
    uint16_t addr = fetch16();
    if (!getC()) {
      JP(addr);
      return 16;
    }
    return 12;
  }
  case 0xDA: { // JP C,a16
    uint16_t addr = fetch16();
    if (getC()) {
      JP(addr);
      return 16;
    }
    return 12;
  }

  case 0xE9: {
    uint16_t addr = getHL();
    PC = addr;
    if (PC >= 0xFF00) {
      printf("JP (HL) set PC to %04X\n", PC);
    }
  }
    return 4; // JP (HL)
  }

  // Call/Return instructions
  switch (opcode) {
  case 0xC4: { // CALL NZ,a16
    uint16_t addr = fetch16();
    if (!getZ()) {
      CALL(addr);
      return 24;
    }
    return 12;
  }
  case 0xCC: { // CALL Z,a16
    uint16_t addr = fetch16();
    if (getZ()) {
      CALL(addr);
      return 24;
    }
    return 12;
  }
  case 0xD4: { // CALL NC,a16
    uint16_t addr = fetch16();
    if (!getC()) {
      CALL(addr);
      return 24;
    }
    return 12;
  }
  case 0xDC: { // CALL C,a16
    uint16_t addr = fetch16();
    if (getC()) {
      CALL(addr);
      return 24;
    }
    return 12;
  }

  case 0xC0:
    if (!getZ()) {
      RET();
      return 20;
    }
    return 8; // RET NZ
  case 0xC8:
    if (getZ()) {
      RET();
      return 20;
    }
    return 8; // RET Z
  case 0xD0:
    if (!getC()) {
      RET();
      return 20;
    }
    return 8; // RET NC
  case 0xD8:
    if (getC()) {
      RET();
      return 20;
    }
    return 8; // RET C
  case 0xD9:
    RET(); /* enable interrupts */
    IME = true;
    return 16; // RETI
  }

  // RST instructions
  if ((opcode & 0xC7) == 0xC7) {
    uint16_t addr = opcode & 0x38;
    CALL(addr);
    return 16;
  }

  // ADD HL,rr
  if ((opcode & 0xCF) == 0x09) {
    uint16_t hl = getHL();
    uint16_t val;
    switch ((opcode >> 4) & 3) {
    case 0:
      val = getBC();
      break;
    case 1:
      val = getDE();
      break;
    case 2:
      val = getHL();
      break;
    case 3:
      val = SP;
      break;
    }
    uint32_t result = hl + val;
    setN(false);
    setH(((hl & 0xFFF) + (val & 0xFFF)) > 0xFFF);
    setC(result > 0xFFFF);
    setHL(result & 0xFFFF);
    return 8;
  }

  // Misc instructions
  switch (opcode) {
  case 0xE3: { // EX (SP),HL
    uint16_t tmp = getHL();
    uint8_t lo = mmu->read(SP);
    uint8_t hi = mmu->read(SP + 1);
    uint16_t mem = lo | (hi << 8);
    mmu->write(SP, tmp & 0xFF);
    mmu->write(SP + 1, (tmp >> 8) & 0xFF);
    setHL(mem);
    return 16;
  }
  case 0xEB: { // EX DE,HL
    uint16_t de = getDE();
    uint16_t hl = getHL();
    setDE(hl);
    setHL(de);
    return 4;
  }
  case 0x27: { // DAA
    uint16_t a = A;
    if (!getN()) {
      if (getC() || a > 0x99) {
        a += 0x60;
        setC(true);
      }
      if (getH() || (a & 0x0F) > 0x09) {
        a += 0x06;
      }
    } else {
      if (getC()) {
        a -= 0x60;
      }
      if (getH()) {
        a -= 0x06;
      }
    }
    A = a & 0xFF;
    setZ(A == 0);
    setH(false);
    return 4;
  }

  case 0x2F:
    A = ~A;
    setN(true);
    setH(true);
    return 4; // CPL
  case 0x37:
    setN(false);
    setH(false);
    setC(true);
    return 4; // SCF
  case 0x3F:
    setN(false);
    setH(false);
    setC(!getC());
    return 4; // CCF

  case 0xE8: { // ADD SP,r8
    int8_t offset = (int8_t)fetch8();
    setZ(false);
    setN(false);
    setH(((SP & 0xF) + (offset & 0xF)) > 0xF);
    setC(((SP & 0xFF) + (offset & 0xFF)) > 0xFF);
    SP += offset;
    return 16;
  }

  case 0xF3: /* disable interrupts */
    IME = false;
    return 4; // DI
  case 0xFB:  /* enable interrupts */
    // EI takes effect after next instruction
    EI_pending = true;
    return 4; // EI
  }

  return UNIMPLEMENTED();
}

int CPU::ALU_DISPATCH() {
  uint8_t opcode = mmu->read(PC - 1);
  int op = (opcode >> 3) & 7;
  int src = opcode & 7;

  uint8_t val;
  int cycles;

  if (src == 6) {
    val = readHL();
    cycles = 8;
  } else {
    val = *regs[src];
    cycles = 4;
  }

  switch (op) {
  case 0:
    ADD(val);
    break;
  case 1:
    ADC(val);
    break;
  case 2:
    SUB(val);
    break;
  case 3:
    SBC(val);
    break;
  case 4:
    AND(val);
    break;
  case 5:
    XOR(val);
    break;
  case 6:
    OR(val);
    break;
  case 7:
    CP(val);
    break;
  }

  return cycles;
}

int CPU::PREFIX_CB() {

  uint8_t opcode = fetch8();

  return (this->*cbtable[opcode])();
}

int CPU::CB_DISPATCH() {

  uint8_t opcode = fetch8();

  int op = (opcode >> 3) & 7;
  int reg = opcode & 7;

  uint8_t *target = (reg == 6) ? nullptr : regs[reg];

  uint8_t val = (reg == 6) ? readHL() : *target;

  int cycles = (reg == 6) ? 16 : 8;

  switch (op) {

  case 0:
    RLC(val);
    break;
  case 1:
    RRC(val);
    break;
  case 2:
    RL(val);
    break;
  case 3:
    RR(val);
    break;
  case 4:
    SLA(val);
    break;
  case 5:
    SRA(val);
    break;

  case 6: // SWAP
    val = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);

    setZ(val == 0);
    setN(false);
    setH(false);
    setC(false);
    break;

  case 7:
    SRL(val);
    break;
  }

  if (reg == 6)
    writeHL(val);
  else
    *target = val;

  return cycles;
}

int CPU::BIT_DISPATCH() {
  uint8_t opcode = mmu->read(PC - 1);
  int bit = (opcode >> 3) & 7;
  int reg = opcode & 7;

  uint8_t val = (reg == 6) ? readHL() : *regs[reg];
  BIT(bit, val);

  return (reg == 6) ? 12 : 8;
}

int CPU::RES_DISPATCH() {
  uint8_t opcode = mmu->read(PC - 1);
  int bit = (opcode >> 3) & 7;
  int reg = opcode & 7;

  if (reg == 6) {
    uint8_t val = readHL();
    RES(bit, val);
    writeHL(val);
    return 16;
  } else {
    RES(bit, *regs[reg]);
    return 8;
  }
}

int CPU::SET_DISPATCH() {
  uint8_t opcode = mmu->read(PC - 1);
  int bit = (opcode >> 3) & 7;
  int reg = opcode & 7;

  if (reg == 6) {
    uint8_t val = readHL();
    SET(bit, val);
    writeHL(val);
    return 16;
  } else {
    SET(bit, *regs[reg]);
    return 8;
  }
}

int CPU::UNIMPLEMENTED() {
  uint16_t addr = PC - 1;
  uint8_t op = mmu->read(addr);
  exit(1);
  return 0;
}

int CPU::LD_r_r(int dst, int src) {
  if (dst == 6 && src == 6) {
    // (HL) -> (HL) no-op
    return 4;
  }

  if (src == 6) {
    // LD r,(HL)
    *regs[dst] = readHL();
    return 8;
  }

  if (dst == 6) {
    // LD (HL),r
    writeHL(*regs[src]);
    return 8;
  }

  // LD r,r'
  *regs[dst] = *regs[src];
  return 4;
}

// === Register pair operations ===
void CPU::setAF(uint16_t v) {
  A = (v >> 8) & 0xFF;
  F = v & 0xF0;
}

void CPU::setBC(uint16_t v) {
  B = (v >> 8) & 0xFF;
  C = v & 0xFF;
}

void CPU::setDE(uint16_t v) {
  D = (v >> 8) & 0xFF;
  E = v & 0xFF;
}

void CPU::setHL(uint16_t v) {
  H = (v >> 8) & 0xFF;
  L = v & 0xFF;
}

uint16_t CPU::getAF() { return (A << 8) | F; }
uint16_t CPU::getBC() { return (B << 8) | C; }
uint16_t CPU::getDE() { return (D << 8) | E; }
uint16_t CPU::getHL() { return (H << 8) | L; }

// === Flag operations ===
void CPU::setZ(bool v) {
  if (v)
    F |= (1 << 7);
  else
    F &= ~(1 << 7);
}

void CPU::setN(bool v) {
  if (v)
    F |= (1 << 6);
  else
    F &= ~(1 << 6);
}

void CPU::setH(bool v) {
  if (v)
    F |= (1 << 5);
  else
    F &= ~(1 << 5);
}

void CPU::setC(bool v) {
  if (v)
    F |= (1 << 4);
  else
    F &= ~(1 << 4);
}

bool CPU::getZ() { return F & (1 << 7); }
bool CPU::getN() { return F & (1 << 6); }
bool CPU::getH() { return F & (1 << 5); }
bool CPU::getC() { return F & (1 << 4); }

// === ALU operations ===
void CPU::ADD(uint8_t val) {
  uint16_t result = A + val;
  setZ((result & 0xFF) == 0);
  setN(false);
  setH(((A & 0xF) + (val & 0xF)) > 0xF);
  setC(result > 0xFF);
  A = result & 0xFF;
}

void CPU::ADC(uint8_t val) {
  int carry = getC() ? 1 : 0;
  uint16_t result = A + val + carry;
  setZ((result & 0xFF) == 0);
  setN(false);
  setH(((A & 0xF) + (val & 0xF) + carry) > 0xF);
  setC(result > 0xFF);
  A = result & 0xFF;
}

void CPU::SBC(uint8_t val) {
  int carry = getC() ? 1 : 0;
  uint16_t result = A - val - carry;
  setZ((result & 0xFF) == 0);
  setN(true);
  setH((A & 0xF) < ((val & 0xF) + carry));
  setC(A < (val + carry));
  A = result & 0xFF;
}

void CPU::SUB(uint8_t val) {
  uint16_t result = A - val;
  setZ((result & 0xFF) == 0);
  setN(true);
  setH((A & 0xF) < (val & 0xF));
  setC(A < val);
  A = result & 0xFF;
}

void CPU::AND(uint8_t val) {
  uint16_t result = A & val;
  setZ((result & 0xFF) == 0);
  setN(false);
  setH(true);
  setC(false);
  A = result & 0xFF;
}

void CPU::OR(uint8_t val) {
  uint16_t result = A | val;
  setZ((result & 0xFF) == 0);
  setN(false);
  setH(false);
  setC(false);
  A = result & 0xFF;
}

void CPU::XOR(uint8_t val) {
  uint16_t result = A ^ val;
  setZ((result & 0xFF) == 0);
  setN(false);
  setH(false);
  setC(false);
  A = result & 0xFF;
}

void CPU::CP(uint8_t val) {
  setZ(A == val);
  setN(true);
  setH((A & 0xF) < (val & 0xF));
  setC(A < val);
}

// === Increment/Decrement ===
void CPU::INC8(uint8_t &reg) {
  setH((reg & 0xF) == 0xF);
  reg++;
  setZ(reg == 0);
  setN(false);
}

void CPU::DEC8(uint8_t &reg) {
  setH((reg & 0xF) == 0);
  reg--;
  setZ(reg == 0);
  setN(true);
}

void CPU::INC16(uint16_t &reg) { reg++; }
void CPU::DEC16(uint16_t &reg) { reg--; }

// === Bit operations ===
void CPU::BIT(int bit, uint8_t val) {
  setZ(!(val & (1 << bit)));
  setN(false);
  setH(true);
}

void CPU::SET(int bit, uint8_t &reg) { reg |= (1 << bit); }
void CPU::RES(int bit, uint8_t &reg) { reg &= ~(1 << bit); }

// === Stack operations ===
void CPU::PUSH(uint16_t val) {
  SP--;
  mmu->write(SP, (val >> 8) & 0xFF);
  SP--;
  mmu->write(SP, val & 0xFF);
}

uint16_t CPU::POP() {
  uint8_t lo = mmu->read(SP++);
  uint8_t hi = mmu->read(SP++);
  return lo | (hi << 8);
}

void CPU::CALL(uint16_t addr) {
  PUSH(PC);
  PC = addr;
}

void CPU::RET() {
  uint16_t val = POP();
  PC = val;
}

// === Jump operations ===
void CPU::JP(uint16_t addr) { PC = addr; }
void CPU::JR(int8_t offset) { PC += offset; }

// === Rotation/Shift operations ===
void CPU::RLC(uint8_t &reg) {
  bool carry = reg & 0x80;
  reg = (reg << 1) | carry;
  setC(carry);
  setZ(reg == 0);
  setN(false);
  setH(false);
}

void CPU::RL(uint8_t &reg) {
  bool oldCarry = getC();
  bool newCarry = reg & 0x80;
  reg = (reg << 1) | oldCarry;
  setC(newCarry);
  setZ(reg == 0);
  setN(false);
  setH(false);
}

void CPU::RRC(uint8_t &reg) {
  bool carry = reg & 1;
  reg = (reg >> 1) | (carry << 7);
  setC(carry);
  setZ(reg == 0);
  setN(false);
  setH(false);
}

void CPU::RR(uint8_t &reg) {
  bool oldCarry = getC();
  bool newCarry = reg & 1;
  reg = (reg >> 1) | (oldCarry << 7);
  setC(newCarry);
  setZ(reg == 0);
  setN(false);
  setH(false);
}

void CPU::SLA(uint8_t &reg) {
  bool carry = reg & 0x80;
  reg <<= 1;
  setC(carry);
  setZ(reg == 0);
  setN(false);
  setH(false);
}

void CPU::SRA(uint8_t &reg) {
  bool carry = reg & 1;
  bool msb = reg & 0x80;
  reg >>= 1;
  reg |= msb;
  setC(carry);
  setZ(reg == 0);
  setN(false);
  setH(false);
}

void CPU::SRL(uint8_t &reg) {
  bool carry = reg & 1;
  reg >>= 1;
  setC(carry);
  setZ(reg == 0);
  setN(false);
  setH(false);
}
