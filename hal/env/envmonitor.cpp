#include "envmonitor.h"

#include <QDebug>

#define ENV_MONITOR_PATH "/dev/ttyUSB0"

EnvMonitor::EnvMonitor(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<EnvData>();

    port = new QSerialPort(ENV_MONITOR_PATH, this);
    port->setBaudRate(QSerialPort::Baud9600);
    port->setDataBits(QSerialPort::Data8);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);
    port->setParity(QSerialPort::NoParity);
    port->open(QSerialPort::OpenModeFlag::ReadWrite);
    connect(port, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

void EnvMonitor::onSendEnvMonitor(QByteArray data) {
    port->write(data);
    port->flush();
}

void EnvMonitor::onReadyRead() {
   QByteArray d = port->readAll();
   EnvData envData = { };

   if (d.length() > 0) {
       readBuffer += QString(d);

       QStringList commandsList = readBuffer.split("\r\n");

       if (commandsList.size() > 0) {
           qDebug() << "env : " << readBuffer;

           foreach (QString command, commandsList) {
               QStringList envDataList = command.split(" ");

               if (envDataList.size() >= 6) {
                   if(envDataList[0]=="S" && envDataList[1] == "2") {
                       envData.temp = envDataList[2];
                       envData.humidity = envDataList[3];
                       envData.gas = envDataList[4];
                       envData.co = envDataList[5];
                       emit onDataFromEnvMonitor(envData);
                   }
               }
           }
           readBuffer.clear();
       }
   }
}
