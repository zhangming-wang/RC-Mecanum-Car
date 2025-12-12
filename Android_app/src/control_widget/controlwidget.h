#pragma once

#include "../common/enum.h"
#include "../http_client/httpClient.h"
#include "ui_controlwidget.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
    class ControlWidget;
}
QT_END_NAMESPACE

class ControlWidget : public QWidget {
    Q_OBJECT

public:
    ControlWidget(QWidget *parent = nullptr);
    ~ControlWidget();

private:
    void onHttpStatusChanged(bool connect);
    void onRecvData(QJsonObject jsonData);

    void _updateSPeedPercentLabel(double percent);

    bool speed_slider_is_pressed_ = false;

    HttpClient *httpClient_;
    Ui::ControlWidget *ui;
};
