#include "mainapp.h"
#include "ui_mainapp.h"
#include <QDateTime>

#include <QDebug>

#define MAINAPP_STACKED_TIME_VIEW 0
#define MAINAPP_STACKED_BROADCAST_VIEW 1
#define MAINAPP_STACKED_GROUP_VIEW 2

MainApp::MainApp(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainApp),
    currentUIMode(UIMODE_IDLE),
    grpSelected(0)
{
    ui->setupUi(this);
    connect(ui->WidgetGroup, &GroupDisplay::groupClicked, [this](int i){
        grpSelected = i+1;
        if (currentUIMode == UIMODE_BROADCAST_GROUP_MENU) {
            currentUIMode = UIMODE_BROADCAST_GROUP_SELECTED;
        } else {
            currentUIMode = UIMODE_IDLE;
        }
        onBroadcastButtonClicked();
    });
    connect(this, SIGNAL(showNotification(QString)),
            ui->ViewControl->widget(MAINAPP_STACKED_TIME_VIEW), SLOT(createNotification(QString)));
}

MainApp::~MainApp()
{
    delete ui;
}

void MainApp::onDisableUI() {
    ui->buttonBroadcastAll->setEnabled(false);
    ui->buttonBroadcastGrp->setEnabled(false);
    ui->buttonBroadcastIndividual->setEnabled(false);
}

void MainApp::onEnableUI() {
    ui->buttonBroadcastAll->setEnabled(true);
    ui->buttonBroadcastGrp->setEnabled(true);
    ui->buttonBroadcastIndividual->setEnabled(true);
}

void MainApp::onMigrateUI(UIMode mode) {
    for(int i = 0; i < ui->ViewControl->count(); i++) {
        ui->ViewControl->widget(i)->hide();
    }

    currentUIMode = mode;

    ui->buttonBroadcastAll->setEnabled(false);
    ui->buttonBroadcastGrp->setEnabled(false);
    ui->buttonBroadcastIndividual->setEnabled(false);

    switch (currentUIMode) {
    case UIMODE_IDLE:
        ui->buttonBroadcastAll->setText("전체방송");
        ui->buttonBroadcastGrp->setText("그룹방송");
        ui->buttonBroadcastIndividual->setText("개별방송");
        ui->buttonBroadcastAll->setEnabled(true);
        ui->buttonBroadcastGrp->setEnabled(true);
        ui->buttonBroadcastIndividual->setEnabled(true);
        ui->ViewControl->widget(MAINAPP_STACKED_TIME_VIEW)->show();
        break;

    case UIMODE_BROADCAST_ALL_SELECTED:
        ui->buttonBroadcastAll->setText("방송종료");
        ui->buttonBroadcastAll->setEnabled(true);
        ui->WidgetBroadcast->setBroadcastMessage("전체방송 중!!!");
        ui->ViewControl->widget(MAINAPP_STACKED_BROADCAST_VIEW)->show();
        break;

    case UIMODE_BROADCAST_GROUP_MENU:
        ui->buttonBroadcastGrp->setText("그룹선택");
        ui->buttonBroadcastGrp->setEnabled(true);
        ui->ViewControl->widget(MAINAPP_STACKED_GROUP_VIEW)->show();
        break;

    case UIMODE_BROADCAST_INDIVIDUAL:
        ui->buttonBroadcastIndividual->setText("방송종료");
        ui->buttonBroadcastIndividual->setEnabled(true);
        ui->WidgetBroadcast->setBroadcastMessage("개별방송 중!!!");
        ui->ViewControl->widget(MAINAPP_STACKED_BROADCAST_VIEW)->show();
        break;

    case UIMODE_BROADCAST_GROUP_SELECTED:
        ui->buttonBroadcastGrp->setText("방송종료");
        ui->buttonBroadcastGrp->setEnabled(true);
        ui->WidgetBroadcast->setBroadcastMessage(QString::number(grpSelected) + "그룹 방송 중!!!");
        ui->ViewControl->widget(MAINAPP_STACKED_BROADCAST_VIEW)->show();
        break;

    case UIMODE_BROADCAST_UNKNOWN_GROUP_SELECTED:
        ui->buttonBroadcastGrp->setText("방송종료");
        ui->buttonBroadcastGrp->setEnabled(true);
        ui->WidgetBroadcast->setBroadcastMessage("그룹 방송 중!!!");
        ui->ViewControl->widget(MAINAPP_STACKED_BROADCAST_VIEW)->show();
        break;
    }
}

void MainApp::onBroadcastButtonClicked() {
    for(int i = 0; i < ui->ViewControl->count(); i++) {
        ui->ViewControl->widget(i)->hide();
    }

    ui->buttonBroadcastAll->setEnabled(false);
    ui->buttonBroadcastGrp->setEnabled(false);
    ui->buttonBroadcastIndividual->setEnabled(false);

    onMigrateUI(currentUIMode);

    switch (currentUIMode) {
    case UIMODE_IDLE:
        emit endBroadcast();
        break;

    case UIMODE_BROADCAST_ALL_SELECTED:
        emit startBroadcast(0xFF);
        break;

    case UIMODE_BROADCAST_INDIVIDUAL:
        emit startBroadcast(0xFF);
        break;

    case UIMODE_BROADCAST_GROUP_SELECTED:
        emit startBroadcast(grpSelected);
        break;
    }

}

void MainApp::on_buttonBroadcastAll_clicked()
{
    if (currentUIMode == UIMODE_IDLE) {
        currentUIMode = UIMODE_BROADCAST_ALL_SELECTED;
    } else {
        currentUIMode = UIMODE_IDLE;
    }
    onBroadcastButtonClicked();
}

void MainApp::on_buttonBroadcastGrp_clicked()
{
    if (currentUIMode == UIMODE_IDLE) {
        currentUIMode = UIMODE_BROADCAST_GROUP_MENU;
    } else {
        currentUIMode = UIMODE_IDLE;
    }
    onBroadcastButtonClicked();
}

void MainApp::on_buttonBroadcastIndividual_clicked()
{
    if (currentUIMode == UIMODE_IDLE) {
        currentUIMode = UIMODE_BROADCAST_INDIVIDUAL;
    } else {
        currentUIMode = UIMODE_IDLE;
    }
    onBroadcastButtonClicked();
}

void MainApp::onNotification(QString msg) {
    emit showNotification(msg);
}
