/*
 * An extension to the QT Dial class that sends "Double Click" ,Increment and Decrement signals
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


#ifndef DIALPLUS_H
#define DIALPLUS_H

#include <QDial>
#include <QMouseEvent>

class DialPlus: public QDial
{
  Q_OBJECT

public:
  DialPlus(QWidget* parent = NULL);

  signals:
  void Double_Click();
  void Increment();
  void Decrement();

protected:
  void mousePressEvent(QMouseEvent *event);

public Q_SLOTS:
  void sliderMoved(int Value);

};

#endif //DIALPLUS_H

