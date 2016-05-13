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
