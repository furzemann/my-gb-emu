#pragma once

#include "memory.h"
#include <cstdint>

class CPU {
public:
  CPU(Memory *mem);

  // Execute one instruction
  // returns cycle count
  int step();

private:
  Memory *mmu;

  typedef int (CPU::*OpcodeFunc)();

  OpcodeFunc table[256];
  OpcodeFunc cbtable[256];

  // registers
  uint8_t A, F, B, C, D, E, H, L;

  uint16_t SP;
  uint16_t PC;

  // flags
  static constexpr uint8_t FLAG_Z = 1 << 7;
  static constexpr uint8_t FLAG_N = 1 << 6;
  static constexpr uint8_t FLAG_H = 1 << 5;
  static constexpr uint8_t FLAG_C = 1 << 4;

  //------------------------
  // register pairs
  //------------------------

  uint16_t getAF();
  uint16_t getBC();
  uint16_t getDE();
  uint16_t getHL();

  void setAF(uint16_t v);
  void setBC(uint16_t v);
  void setDE(uint16_t v);
  void setHL(uint16_t v);

  //------------------------
  // fetch
  //------------------------

  uint8_t fetch8();
  uint16_t fetch16();

  //------------------------
  // ALU
  //------------------------

  void ADD(uint8_t val);
  void ADC(uint8_t val);

  void SUB(uint8_t val);
  void SBC(uint8_t val);

  void AND(uint8_t val);
  void OR(uint8_t val);
  void XOR(uint8_t val);

  void CP(uint8_t val);

  //------------------------
  // inc / dec
  //------------------------

  void INC8(uint8_t &reg);
  void DEC8(uint8_t &reg);

  void INC16(uint16_t &reg);
  void DEC16(uint16_t &reg);

  //------------------------
  // rotate shift
  //------------------------

  void RLC(uint8_t &reg);
  void RRC(uint8_t &reg);

  void RL(uint8_t &reg);
  void RR(uint8_t &reg);

  void SLA(uint8_t &reg);
  void SRA(uint8_t &reg);
  void SRL(uint8_t &reg);

  //------------------------
  // bit operations
  //------------------------

  void BIT(int bit, uint8_t val);

  void SET(int bit, uint8_t &reg);

  void RES(int bit, uint8_t &reg);

  //------------------------
  // stack
  //------------------------

  void PUSH(uint16_t value);

  uint16_t POP();

  //------------------------
  // jumps
  //------------------------

  void JP(uint16_t addr);

  void JR(int8_t offset);

  void CALL(uint16_t addr);

  void RET();

  //------------------------
  // flags
  //------------------------

  void setZ(bool v);
  void setN(bool v);
  void setH(bool v);
  void setC(bool v);

  bool getZ();
  bool getN();
  bool getH();
  bool getC();

  //------------------------
  // opcode handlers
  //------------------------

  int NOP();

  int UNIMPLEMENTED();

  int LD_A_d8();
  int LD_B_d8();
  int LD_C_d8();

  int ADD_A_B();
  int ADD_A_C();

  int SUB_B();

  int JP_a16();

  int CALL_a16();

  int RET_OP();

  //------------------------
  // CB opcodes
  //------------------------

  int RL_C();
};
;
