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


#ifndef OGLWIDGET_H
#define OGLWIDGET_H

#include <QWidget>
#include <QOpenGLWidget>
#include <gl/GLU.h>
#include <gl/GL.h>

const unsigned int W = 128;
const unsigned int H = 128;
#define  SCALE 2


class ili9163c_Widget : public QOpenGLWidget
{
public:
    ili9163c_Widget(QWidget *parent = 0);
    ~ili9163c_Widget();

    uint16_t Limit_X0;
    uint16_t Limit_X1;
    uint16_t Limit_Y0;
    uint16_t Limit_Y1;

    uint16_t Pos_X;
    uint16_t Pos_Y;

    uint32_t PixBuff[W][H];

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();



};

#endif // OGLWIDGET_H
