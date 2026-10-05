#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUi();
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);

    m_gamepad = new GamepadWidget();
    root->addWidget(m_gamepad);

    // m_controlWidget = new ControlWidget();
    // root->addWidget(m_controlWidget);
}