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
