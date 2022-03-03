#include "clientapp.h"
#include "ui_clientapp.h"

#include <QDebug>

#define CLIENTAPP_STACKED_TIME_VIEW 0

ClientApp::ClientApp(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ClientApp)
{
    ui->setupUi(this);

    connect(this, SIGNAL(showNotification(QString)),
            ui->ViewControl->widget(CLIENTAPP_STACKED_TIME_VIEW), SLOT(createNotification(QString)));
}

ClientApp::~ClientApp()
{
    delete ui;
}

void ClientApp::onNewsChanged(QString news) {
    ui->LabelNews->setText(news);
}

void ClientApp::onEnvDataReceived(EnvData data) {
    qDebug() << "onEnvDataReceived =====";
    ui->labelTemp->setText(data.temp);
    ui->labelHumidity->setText(data.humidity);
    ui->labelGas->setText(data.gas);
    ui->labelCo->setText(data.co);
}

void ClientApp::onNotification(QString msg) {
    qDebug() << "onNotification : " << msg;
    emit showNotification(msg);
}
