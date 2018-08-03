/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDial>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "dialplus.h"
#include "ili9163c_widget.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QWidget *layoutWidget;
    QVBoxLayout *LHS_Layout;
    ili9163c_Widget *widget;
    QPlainTextEdit *eb_Debug;
    QWidget *layoutWidget1;
    QVBoxLayout *RHS_Layout;
    QPushButton *bt_M1;
    QPushButton *bt_Set;
    QPushButton *bt_M2;
    DialPlus *dial;
    QPushButton *bt_On_Off;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QLabel *lb_V_in;
    QDial *dial_V_In;
    QLabel *lb_Load;
    QDial *dial_Load;
    QPushButton *bt_Doit;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->setWindowModality(Qt::ApplicationModal);
        MainWindow->resize(383, 492);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        layoutWidget = new QWidget(centralWidget);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(9, 9, 258, 471));
        LHS_Layout = new QVBoxLayout(layoutWidget);
        LHS_Layout->setSpacing(6);
        LHS_Layout->setContentsMargins(11, 11, 11, 11);
        LHS_Layout->setObjectName(QStringLiteral("LHS_Layout"));
        LHS_Layout->setContentsMargins(0, 0, 0, 0);
        widget = new ili9163c_Widget(layoutWidget);
        widget->setObjectName(QStringLiteral("widget"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(widget->sizePolicy().hasHeightForWidth());
        widget->setSizePolicy(sizePolicy);
        widget->setMinimumSize(QSize(256, 256));

        LHS_Layout->addWidget(widget);

        eb_Debug = new QPlainTextEdit(layoutWidget);
        eb_Debug->setObjectName(QStringLiteral("eb_Debug"));

        LHS_Layout->addWidget(eb_Debug);

        layoutWidget1 = new QWidget(centralWidget);
        layoutWidget1->setObjectName(QStringLiteral("layoutWidget1"));
        layoutWidget1->setGeometry(QRect(278, 10, 91, 201));
        RHS_Layout = new QVBoxLayout(layoutWidget1);
        RHS_Layout->setSpacing(6);
        RHS_Layout->setContentsMargins(11, 11, 11, 11);
        RHS_Layout->setObjectName(QStringLiteral("RHS_Layout"));
        RHS_Layout->setContentsMargins(0, 0, 0, 0);
        bt_M1 = new QPushButton(layoutWidget1);
        bt_M1->setObjectName(QStringLiteral("bt_M1"));

        RHS_Layout->addWidget(bt_M1);

        bt_Set = new QPushButton(layoutWidget1);
        bt_Set->setObjectName(QStringLiteral("bt_Set"));

        RHS_Layout->addWidget(bt_Set);

        bt_M2 = new QPushButton(layoutWidget1);
        bt_M2->setObjectName(QStringLiteral("bt_M2"));

        RHS_Layout->addWidget(bt_M2);

        dial = new DialPlus(layoutWidget1);
        dial->setObjectName(QStringLiteral("dial"));
        dial->setMaximum(29);
        dial->setWrapping(true);

        RHS_Layout->addWidget(dial);

        bt_On_Off = new QPushButton(layoutWidget1);
        bt_On_Off->setObjectName(QStringLiteral("bt_On_Off"));

        RHS_Layout->addWidget(bt_On_Off);

        verticalLayoutWidget = new QWidget(centralWidget);
        verticalLayoutWidget->setObjectName(QStringLiteral("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(280, 220, 91, 261));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(verticalLayoutWidget);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        lb_V_in = new QLabel(groupBox);
        lb_V_in->setObjectName(QStringLiteral("lb_V_in"));
        lb_V_in->setGeometry(QRect(0, 20, 89, 13));
        dial_V_In = new QDial(groupBox);
        dial_V_In->setObjectName(QStringLiteral("dial_V_In"));
        dial_V_In->setGeometry(QRect(0, 30, 89, 81));
        dial_V_In->setMaximum(40000);
        dial_V_In->setValue(24000);
        lb_Load = new QLabel(groupBox);
        lb_Load->setObjectName(QStringLiteral("lb_Load"));
        lb_Load->setGeometry(QRect(0, 130, 91, 16));
        dial_Load = new QDial(groupBox);
        dial_Load->setObjectName(QStringLiteral("dial_Load"));
        dial_Load->setGeometry(QRect(0, 150, 89, 81));
        dial_Load->setMinimum(0);
        dial_Load->setMaximum(500);
        dial_Load->setValue(250);
        bt_Doit = new QPushButton(groupBox);
        bt_Doit->setObjectName(QStringLiteral("bt_Doit"));
        bt_Doit->setGeometry(QRect(10, 230, 75, 23));

        verticalLayout->addWidget(groupBox);

        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0));
        bt_M1->setText(QApplication::translate("MainWindow", "M1", 0));
        bt_Set->setText(QApplication::translate("MainWindow", "Set", 0));
        bt_M2->setText(QApplication::translate("MainWindow", "M2", 0));
        bt_On_Off->setText(QApplication::translate("MainWindow", "On Off", 0));
        groupBox->setTitle(QApplication::translate("MainWindow", "Simulator", 0));
        lb_V_in->setText(QApplication::translate("MainWindow", "V In: ", 0));
        lb_Load->setText(QApplication::translate("MainWindow", "Load:", 0));
        bt_Doit->setText(QApplication::translate("MainWindow", "Do It", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
