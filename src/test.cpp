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
#include <iostream>
#include <fstream>
#include "chip8.hpp"
using Emulator = emulators::Chip8<>;

Emulator *emulator;

int main(int argc, char **argv) {
  /**  Creating emulator and loading rom **/
  emulator = new Emulator;
  if (argc != 3) {
    std::cerr << "usage: " << argv[0] << " [rom] [test]" << std::endl;
    return -1;
  }

  if (int err = emulator->LoadProgram(argv[1]) != 0) {
    std::cerr << "loading " << argv[1] << " returned error code " << err
              << std::endl;
    return -1;
  }

  std::fstream in(argv[2], std::ios::in);
  uint16_t pc, opcode, v[16], index, sp;
  bool overwrite;
  int line = 1;
  while (in.good()) {
    in >> pc >> opcode >> overwrite;
    for (std::size_t i = 0; i < 16; ++i) in >> v[i];
    in >> index >> sp;
    std::cout << std::setw(8) << line << ": ";
    if (!in.good()) break;
    emulator->DecrementTimers();
    emulator->TestEvaluateInstruction(pc, opcode, overwrite, v, index, sp);
    ++line;
  }

  std::cout << "Test finished" << std::endl;
  delete emulator;

  return 0;
}
