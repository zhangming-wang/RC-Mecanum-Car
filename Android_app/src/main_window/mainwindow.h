#pragma once

#include "../control_widget/controlwidget.h"
#include "../gamepad_widget/gamepadwidget.h"
#include <QApplication>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUi();

private:
    GamepadWidget *m_gamepad{nullptr};
    ControlWidget *m_controlWidget{nullptr};
};
