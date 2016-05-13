/* Copyright (c) 2016, Troels F. Roennow */
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdexcept>
#include <iostream>
#include <vector>

namespace emulators {
class Canvas {
  uint16_t screen_height_, screen_width_;
  GLuint texture_id_ = 0;
  GLuint *pixels_ = nullptr;
  GLuint texture_width_ = 0;
  GLuint texture_height_ = 0;
  GLuint size_ = 0;

  void GenerateTexture() {
    size_ = texture_width_ * texture_height_;
    pixels_ = new GLuint[size_];
    for (std::size_t i = 0; i < size_; ++i) pixels_[i] = 0;

    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width_, texture_height_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
      throw std::runtime_error((char *)gluErrorString(error));
  }

  void RenderTexture() {
    if (texture_id_) {
      glLoadIdentity();
      glColor3f(1., 1., 1.);

      GLfloat texTop = 0;
      GLfloat texBottom = 1;
      GLfloat texLeft = 0;
      GLfloat texRight = 1;

      GLfloat quadWidth = screen_width_;
      GLfloat quadHeight = screen_height_;

      glBindTexture(GL_TEXTURE_2D, texture_id_);
      glBegin(GL_QUADS);
      glTexCoord2f(texLeft, texTop);
      glVertex2f(0, 0);
      glTexCoord2f(texRight, texTop);
      glVertex2f(quadWidth, 0);
      glTexCoord2f(texRight, texBottom);
      glVertex2f(quadWidth, quadHeight);
      glTexCoord2f(texLeft, texBottom);
      glVertex2f(0, quadHeight);
      glEnd();
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }

  std::size_t width() const { return texture_width_; }
  std::size_t height() const { return texture_height_; }

 public:
  Canvas(std::size_t const &texture_height = 32,
         std::size_t const &texture_width = 64,
         std::size_t const &screen_height = 480,
         std::size_t const &screen_width = 640)
      : texture_height_(texture_height),
        texture_width_(texture_width),
        screen_height_(screen_height),
        screen_width_(screen_width) {}

  void Clear() {
    Lock();
    std::size_t W = width();
    std::size_t H = height();
    for (std::size_t i = 0; i < H; ++i)
      for (std::size_t j = 0; j < W; ++j) {
        SetPixel(i, j, 0);
      }
    Unlock();
  }

  void SetPixel(std::size_t const &i, std::size_t const &j, GLuint val) {
    pixels_[i * texture_width_ + j] = val;
  }

  void Finalize() {
    if (texture_id_ != 0) {
      glDeleteTextures(1, &texture_id_);
      texture_id_ = 0;
    }

    if (pixels_ != nullptr) {
      delete[] pixels_;
      pixels_ = nullptr;
    }
  }
  ~Canvas() { Finalize(); }

  void Initialize() {
    int argc = 0;
    glutInit(&argc, {});
    glutInitContextVersion(2, 1);
    glutInitDisplayMode(GLUT_DOUBLE);
    glutInitWindowSize(screen_width_, screen_height_);
    glutCreateWindow("Emulator");

    glViewport(0., 0., screen_width_, screen_height_);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, screen_width_, screen_height_, 0.0, 1.0, -1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0., 0., 0., 1.);

    glEnable(GL_TEXTURE_2D);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
      throw std::runtime_error((char *)gluErrorString(error));

    GenerateTexture();
  }

  bool Lock() {
    if (texture_id_ != 0) {
      glBindTexture(GL_TEXTURE_2D, texture_id_);
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels_);
      glBindTexture(GL_TEXTURE_2D, 0);
      return true;
    }
    return false;
  }

  bool Unlock() {
    if (texture_id_ != 0) {
      glBindTexture(GL_TEXTURE_2D, texture_id_);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_width_, texture_height_,
                      GL_RGBA, GL_UNSIGNED_BYTE, pixels_);
      glBindTexture(GL_TEXTURE_2D, 0);
      return true;
    }
    return false;
  }

  void Render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0., 0., 0., 1.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    RenderTexture();
    glutSwapBuffers();
  }
};
};
