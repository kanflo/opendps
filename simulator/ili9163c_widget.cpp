/*
 * A QT widget that emulates a ili9163 LCD module
 * Copyright (C) 2018 Aaron Keith
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ili9163c_widget.h"

ili9163c_Widget::ili9163c_Widget(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

ili9163c_Widget::~ili9163c_Widget()
{

}

void ili9163c_Widget::initializeGL()
{
glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
}




void ili9163c_Widget::paintGL()
{
    glClearColor(0, 0, 0, 1);
    glClear( GL_COLOR_BUFFER_BIT);


    GLubyte checkImage[H][W][3];

    uint32_t y, x;
    uint32_t pixeles;

    for (y = 0; y < H; y++)
    {
      for (x = 0; x < W; x++)
      {

        pixeles = PixBuff[x][(H-1)-y];

        checkImage[y][x][0] = (GLubyte) (pixeles >> 16);
        checkImage[y][x][1] = (GLubyte) (pixeles >> 8);
        checkImage[y][x][2] = (GLubyte) (pixeles >> 0);

      }
    }

    glPixelZoom(SCALE, SCALE);

    glDrawPixels(W, H, GL_RGB, GL_UNSIGNED_BYTE, checkImage);


}


void ili9163c_Widget::resizeGL(int w, int h)
  {
      glViewport(0, 0, w, h);
  }
