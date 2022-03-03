#include <QThread>
#include <QDebug>
#include <QRandomGenerator>
#include <QFile>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "calendar/calendarview.h"

#include "util/database.h"

#include "hal/rf/rfmodule.h"
#include "hal/audio/audiomanager.h"
#include "setting.h"


#include "perm.h"
#include "hal/linux/linux.h"
#include "hal/gpio/gpio.h"
#include "util/database.h"
#include "hal/sms/smsreceiver.h"
#include "util/filedownloader.h"

#define RESOURCE_AUDIO_START_VOICE "qrc:/resources/sound/broadcast_start_voice.mp3"
#define RESOURCE_AUDIO_START_CHIME "qrc:/resources/sound/broadcast_start_chime.mp3"
#define RESOURCE_AUDIO_END_CHIME "qrc:/resources/sound/broadcast_end_chime.mp3"

#define MESSAGE_ALREADY_BROADCASTING "점검 요망"
#define MESSAGE_SUCCESS_BROADCASTING "방송 성공"

#define GPIO_PIN_RF_MODEM_OPTION_DELAY 300
#define GPIO_PIN_CALL_WAIT_TIME 5000

#define GPIO_PIN_KEY_RECORD

#define GPIO_PIN_AMP 16
#define GPIO_PIN_RF_OPTION 1
#define GPIO_PIN_POWER_STATUS 22
#define GPIO_PIN_BROADCAST_CHECK 17
#define GPIO_PIN_CHANNEL_ONE 26
#define GPIO_PIN_CHANNEL_TWO 19
#define GPIO_PIN_CHANNEL_THREE 13
#define GPIO_PIN_CHANNEL_FOUR 6
#define GPIO_PIN_CHANNEL_FIVE 5

static const int GPIO_PIN_CHANNEL_ARR[5] = {
    GPIO_PIN_CHANNEL_ONE,
    GPIO_PIN_CHANNEL_TWO,
    GPIO_PIN_CHANNEL_THREE,
    GPIO_PIN_CHANNEL_FOUR,
    GPIO_PIN_CHANNEL_FIVE
};

#define STACKEDVIEW_APP 0
#define STACKEDVIEW_CALENDAR 1

#define CALENDAR_MAX_APPEAR_TIME 20000

void MainWindow::systemInit() {
    int channels[5] = { 0 };
    int power = 0;

    os = new Linux(this);
    connect(this, SIGNAL(timeSync(long,long)), os, SLOT(onTimeSync(long,long)));
    connect(this, SIGNAL(powerOff()), os, SLOT(onSystemOff()));

    gpio = new Gpio(this);
    connect(gpio, SIGNAL(gpioInterrupted(int)), this, SLOT(onGpioInterrupted(int)));

    gpio->pinInit(GPIO_PIN_AMP, 1);  // Amp Output
    gpio->pinInit(GPIO_PIN_RF_OPTION, 1);
    gpio->pinInit(GPIO_PIN_CHANNEL_ONE,0);
    gpio->pinInit(GPIO_PIN_CHANNEL_TWO,0);
    gpio->pinInit(GPIO_PIN_CHANNEL_THREE,0);
    gpio->pinInit(GPIO_PIN_CHANNEL_FOUR,0);
    gpio->pinInit(GPIO_PIN_CHANNEL_FIVE,0);
    gpio->pinInit(GPIO_PIN_BROADCAST_CHECK, 0);

    gpio->set(GPIO_PIN_AMP, 1);
    gpio->set(GPIO_PIN_RF_OPTION, 0);
    QThread::msleep(GPIO_PIN_RF_MODEM_OPTION_DELAY);

    // Power check manually
    power = gpio->get(GPIO_PIN_POWER_STATUS);

    if (power == 0) {
        emit powerOff();
    }

    for(int i = 0; i < 5; i++) {
        channels[i] = gpio->get(GPIO_PIN_CHANNEL_ARR[i]);
        qDebug() << "GPio : " << GPIO_PIN_CHANNEL_ARR[i] << ", Value : " << channels[i];
    }


    currentChannel = (channels[0] << 4 | channels[1] << 3 |
                      channels[2] << 2 | channels[3] << 1 |
                      channels[4]);
}

void MainWindow::databaseInit() {
    QThread *dbThread = new QThread(this);
    db = new DataBase();
    db->moveToThread(dbThread);
    dbThread->start();
}

void MainWindow::audioInit() {
    AudioManager *audio = new AudioManager(currentVolume, this);
    connect(this, SIGNAL(volumeChanged(int)), audio, SLOT(onVolumeChanged(int)));
    connect(this, SIGNAL(playMessage(QUrl)), audio, SLOT(play(QUrl)));
    connect(this, SIGNAL(stopMessage()), audio, SLOT(stopPlay()));
    connect(this, SIGNAL(startAudioRecord(QString)), audio, SLOT(recordStart(QString)));
    connect(this, SIGNAL(endAudioRecord()), audio, SLOT(recordEnd()));

    connect(audio, SIGNAL(audioPlayEnd()), this, SLOT(onAudioPlayed()));
    connect(audio, SIGNAL(recordingEnd()), this, SLOT(onAudioRecorded()));
}

void MainWindow::permInit() {
    QThread *permThread = new QThread(this);
    perm = new Perm();
    perm->moveToThread(permThread);
    permThread->start();

    if (!perm->isDataExist()) {
        perm->updatePerm();
    }
}


void MainWindow::smsInit() {
    QThread *smsReceiveThread = new QThread(this);
    smsReceiver = new SMSReceiver();
    smsReceiver->moveToThread(smsReceiveThread);
    smsReceiveThread->start();

    connect(this, SIGNAL(startSMSReceiver()), smsReceiver, SLOT(onStartReceiver()));
    connect(this, SIGNAL(callHangup()), smsReceiver, SLOT(onHangupCall()));
    connect(this, SIGNAL(callAccept(QString)), smsReceiver, SLOT(onCallAccepted(QString)));
    connect(this, SIGNAL(sendSMS(QString,QString)), smsReceiver, SLOT(onSendSMS(QString,QString)));

    connect(smsReceiver, SIGNAL(phoneCallHangup()), this, SLOT(onPhoneCallHanguped()));
    connect(smsReceiver, &SMSReceiver::phoneCall, [this](QString path, QString number) {
        bool isAccessable = perm->isAccessable(number);

        qDebug() << "isAccessable ? : " << isAccessable;
        qDebug() << "audioMode : " <<  audioMode;
        qDebug() << "rfModule->isIdle() : " <<  rfModule->isIdle();

        if (audioMode == AUDIO_ON_IDLE && isAccessable && rfModule->isIdle()) {
            emit disableUI();

            currentCallNumber = number;
            currentCallPath = path;
            recordingMode = RECORDING_CALL;

            emit startCallRecording(0xFF);
        } else {
            emit callHangup();
        }
    });
    connect(smsReceiver, SIGNAL(smsReceived(QString,QString)), this, SLOT(onSMSReceived(QString,QString)));

    emit startSMSReceiver();
}

void MainWindow::rfModuleInit() {

    QThread *rfThread = new QThread(this);
    rfModule = new RFModule(currentChannel);
    gpio->set(GPIO_PIN_RF_OPTION, 1);
    QThread::msleep(GPIO_PIN_RF_MODEM_OPTION_DELAY);
    rfModuleInitialized = true;

    connect(rfModule, &RFModule::rfModuleInitialized, [this]() {
       qDebug() << "RF Option High";
       gpio->set(GPIO_PIN_RF_OPTION, 1);
       QThread::msleep(GPIO_PIN_RF_MODEM_OPTION_DELAY);
    });

    connect(this, SIGNAL(broadcast(QString,uint8_t,PacketHandShakes)),
            rfModule, SLOT(onBroadcast(QString,uint8_t,PacketHandShakes)));

    connect(this, SIGNAL(broadcast(QByteArray,uint8_t,PacketHandShakes)),
            rfModule, SLOT(onBroadcast(QByteArray,uint8_t,PacketHandShakes)));

    connect(rfModule, SIGNAL(broadcastEnd(PacketHandShakes)), this, SLOT(onBroadcastEnd(PacketHandShakes)));

    connect(rfModule, &RFModule::showAlert, [this](int individualCode, QString kindCode) {
        emit createNotification(individualCode + "번 유저 " + kindCode);
    });

    rfModule->moveToThread(rfThread);
    rfThread->start();
}

void MainWindow::appInit() {
    MainApp *mainApp = new MainApp();
    ui->ViewControl->addWidget(mainApp);
    connect(this, SIGNAL(createNotification(QString)), mainApp, SLOT(onNotification(QString)));
    connect(mainApp, SIGNAL(startBroadcast(int)), this, SLOT(onStartRecording(int)));
    connect(this, SIGNAL(startRecording(int)), this, SLOT(onStartRecording(int)));
    connect(this, SIGNAL(startCallRecording(int)), this, SLOT(onStartCallRecording(int)));

    connect(mainApp, SIGNAL(endBroadcast()), this, SLOT(onEndRecording()));
    connect(this, SIGNAL(endCallBroadcast()), this, SLOT(onEndRecording()));
    connect(this, SIGNAL(migrateAppUI(UIMode)), mainApp, SLOT(onMigrateUI(UIMode)));

    connect(this, SIGNAL(enableUI()), mainApp, SLOT(onEnableUI()));
    connect(this, SIGNAL(disableUI()), mainApp, SLOT(onDisableUI()));
}

void MainWindow::onAudioPlayed() {
    qDebug() << "Play End! : " << audioMode;

    switch (audioMode) {
    case AUDIO_ON_IDLE:
        break;

    case AUDIO_ON_START_CHIME:

        qDebug() << "recordingMode : " << recordingMode;


        if (recordingMode == RECORDING_CALL) {
            emit callAccept(currentCallPath);
        }

        if (recordingMode == RECORDING_CALL || recordingMode == RECORDING_MESSAGE) {
            int brStatus = gpio->get(GPIO_PIN_BROADCAST_CHECK);

            if (brStatus) {
                emit sendSMS(currentCallNumber, MESSAGE_ALREADY_BROADCASTING);
            } else {
                emit sendSMS(currentCallNumber, MESSAGE_SUCCESS_BROADCASTING);
            }

            if (recordingMode == RECORDING_CALL) {
                recordFilename = QString("/tmp/") + generateFilePath() + ".mp3";
                emit startAudioRecord(recordFilename);
                audioMode = AUDIO_ON_RECORDING;
                qInfo() << "Record Start [" << recordFilename << "]";
            } else {

                Q_FOREACH(uint8_t grp, broadcastGroupList) {
                    emit broadcast(recordFilename, grp, RF_PACKET_HANDSHAKE_AUDIO);
                    qInfo() << "Broadcast Start(" << grp << " : " << recordFilename;
                }
            }
        } else {
            recordFilename = QString("/tmp/") + generateFilePath() + ".mp3";
            emit startAudioRecord(recordFilename);
            audioMode = AUDIO_ON_RECORDING;
            qInfo() << "Record Start [" << recordFilename << "]";
        }
        break;

    case AUDIO_ON_START_VOICE:
        if (recordingMode != RECORDING_UNKNOWN) {
            audioMode = AUDIO_ON_START_CHIME;
            emit playMessage(QUrl(RESOURCE_AUDIO_START_CHIME));
        }
        break;

    case AUDIO_ON_PLAY_RECORDED_FILE_ONCE:
        audioMode = AUDIO_ON_PLAY_RECORDED_FILE_TWICE;
        emit playMessage(QUrl("file:" + recordFilename));
        break;

    case AUDIO_ON_PLAY_RECORDED_FILE_TWICE:

        Q_FOREACH(uint8_t grp, broadcastGroupList) {
            emit broadcast(recordFilename, grp, RF_PACKET_HANDSHAKE_AUDIO);
            qInfo() << "Broadcast Start(" << grp << " : " << recordFilename;
        }

        break;

    case AUDIO_ON_END_CHIME:
        audioMode = AUDIO_ON_IDLE;
        break;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , uiMode(UI_ON_APP)
    , currentVolume(50)
    , currentGroup(0)
{
    ui->setupUi(this);

    qRegisterMetaType<uint8_t>("uint8_t");

    systemInit();
    databaseInit();
    audioInit();
    permInit();
    smsInit();
    rfModuleInit();
    appInit();
}

void MainWindow::onPhoneCallHanguped() {
    qInfo() << "PhoneCall: onEndRecording()";

    if (audioMode == AUDIO_ON_RECORDING) {
        emit endAudioRecord();
    } else {
        recordingMode = RECORDING_UNKNOWN;
        audioMode = AUDIO_ON_IDLE;
        emit stopMessage();
    }
}

void MainWindow::onBroadcastEnd(PacketHandShakes type) {

    if (type == RF_PACKET_HANDSHAKE_AUDIO) {
        qInfo() << "Broadcast End [" << recordFilename << "]";
        recordingMode = RECORDING_UNKNOWN;
        audioMode = AUDIO_ON_END_CHIME;
        emit playMessage(QUrl(RESOURCE_AUDIO_END_CHIME));
        emit migrateAppUI(UIMODE_IDLE);
        emit enableUI();
    }
}

void MainWindow::onStartRecording(int group) {
    broadcastGroupList.clear();
    broadcastGroupList.append(group);
    audioMode = AUDIO_ON_START_VOICE;
    recordingMode = RECORDING_EQUIPMENT;
    emit playMessage(QUrl(RESOURCE_AUDIO_START_VOICE));
}

void MainWindow::onStartCallRecording(int group) {
    broadcastGroupList.clear();
    broadcastGroupList.append(group);
    audioMode = AUDIO_ON_START_VOICE;
    recordingMode = RECORDING_CALL;
    emit playMessage(QUrl(RESOURCE_AUDIO_START_VOICE));
}

// on UI
void MainWindow::onEndRecording() {
    qInfo() << "UI: onEndRecording()";

    if (audioMode == AUDIO_ON_RECORDING) {
        emit disableUI();
        emit endAudioRecord();
    } else {
        audioMode = AUDIO_ON_IDLE;
        emit stopMessage();
    }
}

// on Audio Instance
void MainWindow::onAudioRecorded() {

    qInfo() << "Audio: onAudioRecorded()";

    if (audioMode == AUDIO_ON_RECORDING) {
        emit disableUI();
        audioMode = AUDIO_ON_PLAY_RECORDED_FILE_ONCE;
        qInfo() << "Audio: Play Recorded File";
        emit playMessage(QUrl("file:" + recordFilename));
    }
}

void MainWindow::onSMSReceived(QString number, QString text) {
    qDebug() << "received text : " << text;
    qDebug() << "received number : " << number;

    if (number.size() > 0) {
        bool isAccessable = perm->isAccessable(number);

        qDebug() << "onSMSReceived, isAccessable : " << isAccessable;
        currentCallNumber = number;

        if (isAccessable) {
            QStringList dataList = text.split(",");

            if (dataList.size() >= 3) {
                QString header = dataList.at(0);
                QString value1 = dataList.at(1);
                QString value2 = dataList.at(2);

                if (QString::compare(header, "updb") == 0) {
                    perm->updatePerm();
                } else if (QString::compare(header, "txtbrcast") == 0 ||
                           QString::compare(header, "recbrcast") == 0) {

                    QStringList groupList = value2.split("-");
                    QString fileName = QString("/tmp/") + generateFilePath() + QString(".mp3");

                    FileDownloader *downloader = new FileDownloader(
                                QString(SERVER_ADDRESS) + QString(SERVER_PATH_DOWNLOAD_AUDIO) +
                                value1,
                                fileName,
                                smsReceiver);

                    connect(downloader, &FileDownloader::downloaded, [this, groupList, value2, number, fileName](QString path) {
                        int grp = 0;

                        if (audioMode == AUDIO_ON_IDLE) {

                            if (grp == 0xFF) {
                                emit migrateAppUI(UIMODE_BROADCAST_ALL_SELECTED);
                            } else {
                                emit migrateAppUI(UIMODE_BROADCAST_UNKNOWN_GROUP_SELECTED);
                            }

                            emit disableUI();

                            broadcastGroupList.clear();

                            if (groupList.size() > 0) {
                                Q_FOREACH(QString groupString, groupList) {
                                    int groupDestination = 0;

                                    groupDestination = groupString.toInt();
                                    broadcastGroupList.append(groupDestination);
                                }
                            } else {
                                broadcastGroupList.append(value2.toInt());
                                grp = value2.toInt();
                            }

                            recordFilename = fileName;
                            recordingMode = RECORDING_MESSAGE;
                            audioMode = AUDIO_ON_START_CHIME;
                            emit playMessage(QUrl(RESOURCE_AUDIO_START_CHIME));
                        } else {
                            emit sendSMS(currentCallNumber, MESSAGE_ALREADY_BROADCASTING);
                        }
                    });

                    downloader->download();
                } else if (QString::compare(header, "subtxt") == 0) {
                    QStringList groupList = value2.split("-");

                    qDebug() << "URL : " <<  QString(SERVER_ADDRESS) + QString(SERVER_PATH_DOWNLOAD_NEWS) +
                                value1;

                    FileDownloader *downloader = new FileDownloader(
                                QString(SERVER_ADDRESS) + QString(SERVER_PATH_DOWNLOAD_NEWS) +
                                value1,
                                QString("/tmp/") + generateFilePath() + QString(".txt"),
                                this);

                    connect(downloader, &FileDownloader::downloaded, [this, groupList, value2](QString path) {
                        QFile textFile(path);
                        textFile.open(QFile::ReadOnly);

                        QByteArray texts = textFile.readAll();

                        if (groupList.size() > 0) {
                            int grp = 0;
                            Q_FOREACH(QString groupString, groupList) {
                                emit broadcast(texts, (uint8_t)groupString.toInt(), RF_PACKET_HANDSHAKE_TEXT);
                            }
                        } else {
                            emit broadcast(texts, (uint8_t)value2.toInt(), RF_PACKET_HANDSHAKE_TEXT);

                        }
                    });
                    downloader->download();

                }
            }
        }
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::generateFilePath()
{
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    QString randomString;

    for(int i = 0; i< SERVER_DOWNLOADED_NAME_SIZE; i++){
        int index = QRandomGenerator::global()->generate() % possibleCharacters.length();
        QChar nextChar = possibleCharacters.at(index);
        randomString.append(nextChar);
    }

    return QString(randomString);
}


void MainWindow::onGpioInterrupted(int pin) {
    qDebug() << "onGpioInterrupted : " << pin;

    switch(pin) {


    case GPIO_PIN_POWER_STATUS:
        emit powerOff();
        return;
    }
}
