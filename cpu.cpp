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

CPU::CPU(Memory *mem) {
  mmu = mem;

  for (int i = 0; i < 256; i++) {
    table[i] = &CPU::UNIMPLEMENTED;
    cbtable[i] = &CPU::UNIMPLEMENTED;
  }
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

  table[0x00] = &CPU::NOP;

  table[0x0E] = &CPU::LD_C_d8;
  table[0x3E] = &CPU::LD_A_d8;
  table[0x06] = &CPU::LD_B_d8;

  table[0x80] = &CPU::ADD_A_B;
  table[0x81] = &CPU::ADD_A_C;

  table[0x90] = &CPU::SUB_B;

  table[0xC3] = &CPU::JP_a16;

  table[0xCD] = &CPU::CALL_a16;

  table[0xC9] = &CPU::RET_OP;

  cbtable[0x11] = &CPU::RL_C;
}

int CPU::step() {
  uint8_t opcode = fetch8();

  if (opcode == 0xCB) {
    uint8_t cb = fetch8();

    return (this->*cbtable[cb])();
  }

  return (this->*table[opcode])();
}

int CPU::NOP() { return 4; }

int CPU::LD_A_d8() {
  A = fetch8();

  return 8;
}

int CPU::ADD_A_B() {
  ADD(B);

  return 4;
}

int CPU::SUB_B() {
  SUB(B);

  return 4;
}

int CPU::JP_a16() {
  JP(fetch16());

  return 16;
}

int CPU::CALL_a16() {
  CALL(fetch16());

  return 24;
}

int CPU::LD_C_d8() {
  C = fetch8();
  return 8;
}

int CPU::RET_OP() {
  RET();

  return 16;
}

int CPU::RL_C() {
  RL(C);

  return 8;
}

int CPU::UNIMPLEMENTED() {
  printf("Unknown opcode %02X at %04X\n", mmu->read(PC - 1), PC - 1);

  exit(1);

  return 0;
}

// Setting / Getting files
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
// SETTING FLAGS
void CPU::setZ(bool v) {
  if (v)
    F |= (1 << 7); // set bit 7
  else
    F &= ~(1 << 7); // clear bit 7
}

void CPU::setN(bool v) {
  if (v)
    F |= (1 << 6); // set bit 7
  else
    F &= ~(1 << 6); // clear bit 7
}

void CPU::setH(bool v) {
  if (v)
    F |= (1 << 5); // set bit 7
  else
    F &= ~(1 << 5); // clear bit 7
}

void CPU::setC(bool v) {
  if (v)
    F |= (1 << 4); // set bit 7
  else
    F &= ~(1 << 4); // clear bit 7
}

bool CPU::getZ() { return F & (1 << 7); };
bool CPU::getN() { return F & (1 << 6); };
bool CPU::getH() { return F & (1 << 5); };
bool CPU::getC() { return F & (1 << 4); };

// ALU FUNCTIONS
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
  setC(A < val + carry);
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

// Increments & Decrements
void CPU::INC8(uint8_t &reg) {
  setH((reg & 0xF) == 0xF);
  reg++;
  setZ(reg == 0);
  setN(false);
}

void CPU::DEC8(uint8_t &reg) {
  setH((reg & 0xF) == 0xF);
  reg--;
  setZ(reg == 0);
  setN(false);
}

void CPU::INC16(uint16_t &reg) { reg++; }

void CPU::DEC16(uint16_t &reg) { reg--; }

// BITS
void CPU::BIT(int bit, uint8_t val) {
  setZ(!(val & (1 << bit)));
  setN(false);
  setH(true);
}

void CPU::SET(int bit, uint8_t &reg) { reg |= (1 << bit); }

void CPU::RES(int bit, uint8_t &reg) { reg &= ~(1 << bit); }

// STACK OPS
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

void CPU::RET() { PC = POP(); }

// JUMPS
void CPU::JP(uint16_t addr) { PC = addr; };
void CPU::JR(int8_t offset) { PC += (int8_t)offset; };

// ROTATIONS OPS
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
