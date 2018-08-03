/*
 * A QT OpenDPS Simulator
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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qtimer.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
    void On_Click_M1();
    void On_Click_M2();
    void On_Click_Set();
    void On_Click_On_Off();


    void dial_incrementer();
    void dial_decrementer();
    void On_dial_Double_Click();

    void on_Dial_V_In_Changed(int Value);
    void on_Dial_Load(int Value);

private:
    Ui::MainWindow *ui;\

    int m_argc;
    char **m_argv;


};

#endif // MAINWINDOW_H
