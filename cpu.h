#pragma once

#include "memory.h"
#include <cstdint>

class CPU {
public:
  CPU(Memory *mem);

  int step();

private:
  Memory *mmu;

  using OpcodeFunc = int (CPU::*)();

  OpcodeFunc table[256];
  OpcodeFunc cbtable[256];

  //------------------------
  // registers
  //------------------------

  uint8_t A, F, B, C, D, E, H, L;

  uint16_t SP;
  uint16_t PC;
  bool justEnabledEI = false;
  bool halted = false;
  //------------------------
  // register lookup
  //------------------------

  uint8_t *regs[8];

  // used by generated opcode dispatch
  int loadDst[256];
  int loadSrc[256];

  //------------------------
  // flags
  //------------------------

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
  // memory helpers
  //------------------------

  uint8_t readHL();
  void writeHL(uint8_t v);

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
  // inc/dec
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
  // accumulator versions
  //------------------------

  int RLCA();
  int RRCA();
  int RLA();
  int RRA();

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
  // generic dispatchers
  //------------------------

  int LD_r_r(int dst, int src);

  int LD_DISPATCH();

  int ALU_DISPATCH();

  int CB_DISPATCH();

  int BIT_DISPATCH();

  int RES_DISPATCH();

  int SET_DISPATCH();

  //------------------------
  // interrupts
  //------------------------

  bool IME;
  bool EI_pending;

  void serviceInterrupt();

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

  int RL_C();

  //------------------------
  // immediates
  //------------------------

  int ADD_A_d8();

  int SUB_d8();

  int AND_d8();

  int XOR_d8();

  int OR_d8();

  int CP_d8();

  //------------------------
  // stack pair ops
  //------------------------

  int PUSH_BC();
  int POP_BC();

  int PUSH_DE();
  int POP_DE();

  int PUSH_HL();
  int POP_HL();

  int PUSH_AF();
  int POP_AF();

  int HALT_OP();
};
