/*********************************************************************************
 * Copyright (c) 2016, Troels F. Roennow
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Troels F. Rønnow
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *********************************************************************************/

#include <stack>
#include <vector>
#include <cstdio>
#include <iomanip>
#include <fstream>
#include <stdexcept>

namespace emulators {

template <std::size_t MEM_SIZE = 0x1000>
class Chip8 {
  // Defining the chip memory layout.
  union {
    uint8_t memory_[MEM_SIZE];
    struct {
      uint8_t fonts_[16 * 5];
      uint16_t stack_pointer_;
      uint16_t stack_[16];
      uint8_t V_[16], keypress_[16];
      uint64_t graphics_[32] = {0};
      uint8_t delay_timer_, sound_timer_;
      uint16_t program_counter_, index_, opcode_;
    };
  };

  bool redraw_ = false;
  uint32_t lcg_x = 1103515245;
  uint8_t Random() {
    lcg_x = lcg_x * 1103515245 + 12345;
    return (lcg_x >> 24) & 0xFF;
  }

 public:
  void Reset() {
    program_counter_ = 0x200;
    stack_pointer_ = delay_timer_ = sound_timer_ = 0;

    static uint8_t fonts[80] = {
        0xF0, 0x90, 0x90,
        0x90, 0xF0,  // 0
        0x20, 0x60, 0x20,
        0x20, 0x70,  // 1
        0xF0, 0x10, 0xF0,
        0x80, 0xF0,  // 2
        0xF0, 0x10, 0xF0,
        0x10, 0xF0,  // 3
        0x90, 0x90, 0xF0,
        0x10, 0x10,  // 4
        0xF0, 0x80, 0xF0,
        0x10, 0xF0,  // 5
        0xF0, 0x80, 0xF0,
        0x90, 0xF0,  // 6
        0xF0, 0x10, 0x20,
        0x40, 0x40,  // 7
        0xF0, 0x90, 0xF0,
        0x90, 0xF0,  // 8
        0xF0, 0x90, 0xF0,
        0x10, 0xF0,  // 9
        0xF0, 0x90, 0xF0,
        0x90, 0x90,  // A
        0xE0, 0x90, 0xE0,
        0x90, 0xE0,  // B
        0xF0, 0x80, 0x80,
        0x80, 0xF0,  // C
        0xE0, 0x90, 0x90,
        0x90, 0xE0,  // D
        0xF0, 0x80, 0xF0,
        0x80, 0xF0,  // E
        0xF0, 0x80, 0xF0,
        0x80, 0x80  // F
    };
    for (std::size_t i = 0; i < 80; ++i) {
      fonts_[i] = fonts[i];
    }
  }

  Chip8() { Reset(); }

  void DecrementTimers() {
    if (delay_timer_ > 0) {
      --delay_timer_;
    }
    if (sound_timer_ > 0) {
      --sound_timer_;
      if (sound_timer_ == 0) std::cout << "BEEP!!" << std::endl;
    }
  }

  int TestEvaluateInstruction(uint16_t pc, uint16_t opcode, bool overwrite,
                              uint16_t *expV, uint16_t index, uint16_t sp) {
    if (pc != program_counter_) {
      std::cerr << "Expected different program counter" << std::endl;
      std::cerr << "Current counter: " << program_counter_ << std::endl;
      std::cerr << "Expected counter: " << pc << std::endl;
      exit(0);
    }

    memory_[pc] = (opcode >> 8) & 0xFF;
    memory_[pc + 1] = opcode & 0xFF;

    EvaluateInstruction();
    int X = (opcode_ >> 8) & 0xF;
    int Y = (opcode_ >> 4) & 0xF;
    uint8_t &VX = V_[X];
    uint8_t &VY = V_[Y];
    uint8_t &VF = V_[0xF];
    //      assert(opcode_ == opcode);
    std::cout << std::right << std::setw(8) << int(pc) << " -  ";
    std::cout << "0x" << std::hex << std::setw(8) << std::left << int(opcode);
    std::cout << std::right << std::setw(8) << std::dec << int(overwrite);
    std::cout << std::setw(8) << std::dec << int(index);
    std::cout << std::setw(8) << std::dec << int(index_);
    std::cout << std::setw(8) << std::dec << int(sp);
    std::cout << std::setw(8) << std::dec
              << int((uint8_t *)&stack_[stack_pointer_] - memory_);
    std::cout << std::setw(8) << " | ";
    std::cout << std::setw(8) << int(VX);
    std::cout << std::setw(8) << int(expV[X]);
    std::cout << std::setw(8) << int(VY);
    std::cout << std::setw(8) << int(expV[Y]);
    std::cout << std::setw(8) << int(VF);
    std::cout << std::setw(8) << int(expV[0xF]);
    std::cout << std::endl;
    if (overwrite) {
      for (std::size_t i = 0; i < 16; ++i) V_[i] = expV[i];
    } else {
      for (std::size_t i = 0; i < 16; ++i) {
        if (V_[i] != expV[i]) {
          std::cerr << "Expected different V[" << i << "]" << std::endl;
          std::cerr << int(V_[i]) << " <> " << int(expV[i]) << std::endl;
          exit(0);
        }
      }
    }
    return 0;
  }

  int EvaluateInstruction() {
    opcode_ = (memory_[program_counter_] << 8) | memory_[program_counter_ + 1];
    program_counter_ += 2;
    uint8_t C = (opcode_ >> 12) & 0xF;
    uint8_t X = (opcode_ >> 8) & 0xF;
    uint8_t Y = (opcode_ >> 4) & 0xF;
    uint8_t N = opcode_ & 0xF;
    uint8_t NN = opcode_ & 0xFF;
    uint16_t NNN = opcode_ & 0xFFF;
    uint8_t &VX = V_[X], &VY = V_[Y], &VF = V_[0xF];
    uint32_t R;

    switch ((opcode_ >> 12) & 0xF) {
      case 0x0:
        switch (NN) {
          /* 00E0  Clear screen */
          case 0xE0:
            break;
            for (auto &d : graphics_) d = 0;
          /* 00EE Returns */
          case 0xEE:
            program_counter_ = stack_[(--stack_pointer_) & 0xF];
            break;
          default:
            std::cerr << "sorry - this emulator does not have an RCA"
                      << std::endl;
            exit(-1);
        }

        break;
      /* 1NNN Jumps to address NNN. */
      case 0x1:
        program_counter_ = NNN;
        break;
      /* 0xBNNN Jumps to the address NNN plus V0. */
      case 0xB:
        program_counter_ = NNN + V_[0];
        break;
      /* 2NNN   Calls subroutine at NNN. */
      case 0x2:
        stack_[(stack_pointer_++) & 0xF] = program_counter_;
        program_counter_ = NNN;
        break;
      /* 3XNN   Skips the next instruction if VX equals NN. */
      case 0x3:
        program_counter_ += (VX == NN) << 1;
        break;
      /* 4XNN   Skips the next instruction if VX doesn't equal NN.*/
      case 0x4:
        program_counter_ += (VX != NN) << 1;
        break;
      /* 5XY0   Skips the next instruction if VX equals VY. */
      case 0x5:
        program_counter_ += (VX == VY) << 1;
        break;
      // 9XY0 Skips the next instruction if VX doesn't equal VY.
      case 0x9:
        program_counter_ += (VX != VY) << 1;
        break;
      /* 6XNN   Sets VX to NN. */
      case 0x6:
        VX = NN;
        break;
      /* 7XNN   Adds NN to VX.      */
      case 0x7:
        VX += NN;
        break;

      case 0x8:
        switch (N) {
          // 8XY0   Sets VX to the value of VY.
          case 0x0:
            VX = VY;
            break;
          // 8XY1   Sets VX to VX or VY.
          case 0x1:
            VX |= VY;
            break;
          // 8XY2   Sets VX to VX and VY.
          case 0x2:
            VX &= VY;
            break;
          // 8XY3   Sets VX to VX xor VY.
          case 0x3:
            VX ^= VY;
            break;
          // 8XY4   Adds VY to VX. VF is set to 1 when there's a carry, and to 0
          // when there isn't.
          case 0x4:
            R = VX + VY;
            VX = R;
            VF = R >> 8;
            break;
          // 8XY5   VY is subtracted from VX. VF is set to 0 when there's a
          // borrow, and 1 when there isn't.
          case 0x5:
            R = VX - VY;
            VX = R;
            VF = !(R >> 8);
            break;
          // 8XY7   Sets VX to VY minus VX. VF is set to 0 when there's a
          // borrow, and 1 when there isn't.
          case 0x7:
            R = VY - VX;
            VX = R;
            VF = !(R >> 8);
            break;

          // 8XY6   Shifts VX right by one. VF is set to the value of the least
          // significant bit of VX before the shift.[2]
          //        case 0x6: VF = VX & 1; VX = VX >> 1; break; // NOTE!!
          //        DIFFERS FROM WIKIPEDIA
          case 0x6:
            VF = VY & 1;
            VX = VY >> 1;
            break;  // NOTE!! DIFFERS FROM WIKIPEDIA
          // 8XYE   Shifts VX left by one. VF is set to the value of the most
          // significant bit of VX before the shift.[2]
          //        case 0xE: VF = (VX >> 7) & 1; VX = VX << 1; break;
          case 0xE:
            VF = (VY >> 7) & 1;
            VX = VY << 1;
            break;
        }

        break;
      /* ANNN Sets I to the address NNN. */
      case 0xA:
        index_ = NNN;
        break;
      /* CXNN Sets VX to a random number, masked by NN. */
      case 0xC:
        VX = Random() & NN;
        break;
      /* DXYN       Sprites stored in memory at location in index register (I),
       * maximum 8bits wide. Wraps around the screen. If when drawn, clears a
       * pixel, register VF is set to 1 otherwise it is zero. All drawing is XOR
       * drawing (i.e. it toggles the screen pixels) */
      case 0xD:
        VF = 0;
        redraw_ = true;
        for (std::size_t y = 0; y < N; ++y) {
          if (index_ + y >= 4096) break;
          uint64_t p = (uint64_t(memory_[index_ + y]) << 56) >> VX;
          if (VY + y >= 32) break;
          VF |= ((graphics_[VY + y] & p) > 0);
          graphics_[VY + y] ^= p;
        }

        break;
      case 0xE:
        switch (NN) {
          // EX9E   Skips the next instruction if the key stored in VX is
          // pressed.
          case 0x9E:
            program_counter_ += (keypress_[VX & 0xF] > 0) << 1;
            break;
          // EXA1   Skips the next instruction if the key stored in VX isn't
          // pressed.
          case 0xA1:
            program_counter_ += (keypress_[VX & 0xF] == 0) << 1;
            break;
        }

        break;
      case 0xF:

        switch (NN) {
          // FX07   Sets VX to the value of the delay timer.
          case 0x07:
            VX = delay_timer_;
            break;
          // FX0A   A key press is awaited, and then stored in VX.
          case 0x0A:
            break;
          // FX15   Sets the delay timer to VX.
          case 0x15:
            delay_timer_ = VX;
            break;
          // FX18   Sets the sound timer to VX.
          case 0x18:
            sound_timer_ = VX;
            break;
          // FX1E   Adds VX to I.[3] --- DIFFERS FROM OTHER IMPLEMENTATIONS
          case 0x1E:
            index_ += VX;
            break;
          // FX29   Sets I to the location of the sprite for the character in
          // VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font.
          case 0x29:
            index_ = &fonts_[(VX & 0xFF) * 5] - memory_;
            break;
          // FX33   Stores the Binary-coded decimal representation of VX, with
          // the most significant of three digits at the address in I, the
          // middle digit at I plus 1, and the least significant digit at I plus
          // 2. (In other words, take the decimal representation of VX, place
          // the hundreds digit in memory at location in I, the tens digit at
          // location I+1, and the ones digit at location I+2.)
          // Probably you could do something like "Add3 and Shift" / Double
          // dabble
          // to do this in a circuit.
          // http://www.minecraftforum.net/forums/minecraft-discussion/redstone-discussion-and/339552-converting-binary-decimals-to-decimal-decimals
          case 0x33:
            memory_[index_ & 0xFFF] = (VX / 100) % 10;
            memory_[(index_ + 1) & 0xFFF] = (VX / 10) % 10;
            memory_[(index_ + 2) & 0xFFF] = (VX) % 10;
            break;
          // FX55   Stores V0 to VX in memory starting at address I.[4]
          case 0x55:
            for (std::size_t i = 0; i < X + 1; ++i)
              memory_[(index_ + i) & 0xFFF] = V_[i];
            break;
          // FX65   Fills V0 to VX with values from memory starting at address
          // I.[4]
          case 0x65:
            for (std::size_t i = 0; i < X + 1; ++i)
              V_[i] = memory_[(index_ + i) & 0xFFF];

            break;
        }

        break;
    }

    return 0;
  };

  int LoadProgram(std::string const &filename) {
    Reset();
    uint16_t pos = program_counter_;
    for (std::ifstream f(filename, std::ios::binary); f.good();) {
      memory_[pos++] = f.get();

      if (pos >= MEM_SIZE) throw std::runtime_error("program too large!");
    }
    return 0;
  }

  bool CanRedraw() const { return redraw_; }
  void ResetRedrawFlag() { redraw_ = false; }
  uint64_t *graphics() { return graphics_; }
  uint8_t *memory() { return memory_; }

  uint16_t index_register() const { return index_; }
  uint16_t program_counter() const { return program_counter_; }
  uint8_t stack_pointer() const { return stack_pointer_; }
  uint8_t delay_timer() const { return delay_timer_; }
};
};
