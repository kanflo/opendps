/*
 *An extension to the QT Dial class that sends "Double Click" ,Increment and Decrement signals
 *An Copyright (C) 2018 Aaron Keith
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


#include <math.h>
#include "dialplus.h"
#include <QDebug>
#include <QMouseEvent>

DialPlus::DialPlus(QWidget* parent) :
    QDial(parent)
{
connect(this, SIGNAL (valueChanged(int)), this, SLOT (sliderMoved(int)));
}

void DialPlus::mousePressEvent(QMouseEvent *event)
{
if(event == NULL)
  return;

if(event->type() == QEvent::MouseButtonDblClick)
{
  emit Double_Click();
}
return;
}

void DialPlus::sliderMoved(int iValue)
{
static int old_value = 0;

int diff = iValue - old_value;
int step = 0;
int Half_Turn = this->maximum() / 2;

if((old_value >= 0) && (old_value < Half_Turn) && (iValue > Half_Turn))
{ //Just gone from 0 to maximum
  step = -(old_value + ((this->maximum() + 1) - iValue));
}
else if((old_value > Half_Turn) && (iValue >= 0) && (iValue < Half_Turn))
{ //Just gone from maximum to 0
  step = (iValue + ((this->maximum() + 1) - old_value));
}
else
{
  step = diff;
}

//    qDebug() << int(iValue) << "," << int(step);

if(step > 0)
{
  step = std::min(step, 6); //limit the step count to just 6
  while (step--)
  {
    emit Increment();
  }

}
else if(step < 0)
{
  step = abs(step);
  step = std::min(step, 6); //limit the step count to just 6
  while (step--)
  {
    emit Decrement();
  }
}

old_value = iValue;

}

