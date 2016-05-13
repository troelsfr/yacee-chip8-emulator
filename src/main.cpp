#include <iostream>
#include "chip8.hpp"
#include "canvas.hpp"
using Emulator = emulators::Chip8<>;

emulators::Canvas *cv;
Emulator *emulator;
int frame = 0;
#define TARGET_SCREEN_FPS 1000

void render() { cv->Render(); }
void main_loop(int val = 0) {
  //  for(std::size_t i=0; i<8; ++i)
  for (std::size_t i = 0; i < 20; ++i) {
    emulator->DecrementTimers();
    emulator->EvaluateInstruction();
  }

  if (emulator->CanRedraw()) {
    cv->Lock();
    for (std::size_t i = 0; i < 32; ++i) {
      uint64_t p = emulator->graphics()[i];
      uint64_t mask = 1ull << 63;
      for (int j = 0; j < 64; ++j) {
        if (p & mask)
          cv->SetPixel(i, j, 255);
        else
          cv->SetPixel(i, j, 0);

        mask >>= 1;
      }
    }
    cv->Unlock();
    render();
    emulator->ResetRedrawFlag();
  }

  glutTimerFunc(1000 / TARGET_SCREEN_FPS, main_loop, frame);
}

int main(int argc, char **argv) {
  /**  Creating emulator and loading rom **/
  emulator = new Emulator;
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " [filename]" << std::endl;
    return -1;
  }

  if (int err = emulator->LoadProgram(argv[1]) != 0) {
    std::cerr << "loading " << argv[1] << " returned error code " << err
              << std::endl;
    return -1;
  }

  /** Starting main loop **/
  cv = new emulators::Canvas;
  cv->Initialize();
  glutDisplayFunc(render);
  glutTimerFunc(1000 / TARGET_SCREEN_FPS, main_loop, frame);
  glutMainLoop();
  delete cv;

  return 0;
}
