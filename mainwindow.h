#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QUrl>
#include "mainapp.h"

#include "hal/rf/rfmodule.h"

class Linux;
class Gpio;
class DataBase;
class SMSReceiver;
class Perm;
class RFModule;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum CurrentUIMode {
    UI_ON_APP,
    UI_ON_CALENDAR,
    UI_ON_REPLAY,
};

enum KeyInput {
    KEYINPUT_CALENDAR,
    KEYINPUT_REPLAY,
    KEYINPUT_STOP_REPLAY,
    KEYINPUT_PREV,
    KEYINPUT_NEXT,
};

enum RecordingMode {
    RECORDING_UNKNOWN,
    RECORDING_EQUIPMENT,
    RECORDING_CALL,
    RECORDING_MESSAGE,
};

enum CurrentAudioMode {
    AUDIO_ON_IDLE,

    AUDIO_ON_START_VOICE,
    AUDIO_ON_START_CHIME,

    AUDIO_ON_PLAY_RECORDED_FILE_ONCE,
    AUDIO_ON_PLAY_RECORDED_FILE_TWICE,

    AUDIO_ON_END_CHIME,
    AUDIO_ON_RECORDING,
};

void onKeyInput(KeyInput i);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:

    Ui::MainWindow *ui;
    CurrentUIMode uiMode;
    CurrentAudioMode audioMode = AUDIO_ON_IDLE;

    Gpio *gpio = nullptr;
    DataBase *db = nullptr;
    Linux *os = nullptr;
    Perm* perm= nullptr;
    SMSReceiver *smsReceiver = nullptr;
    RFModule *rfModule = nullptr;

    // onSendSMS

    QString currentCallNumber;
    QString currentCallPath;

    int currentVolume;
    int currentGroup;
    int currentChannel;

    bool rfModuleInitialized = false;

    RecordingMode recordingMode = RECORDING_UNKNOWN;
    bool isCallRecording = false;
    bool callHanguped = false;

    QString recordFilename;

    QList<uint8_t> broadcastGroupList;

    void systemInit();
    void databaseInit();
    void audioInit();
    void rfModuleInit();
    void permInit();
    void smsInit();
    void appInit();

    void onKeyInput(KeyInput i);

    QString generateFilePath();


private slots:
    void onGpioInterrupted(int pin);
    void onAudioPlayed();
    void onAudioRecorded();
    void onPhoneCallHanguped();
    void onBroadcastEnd(PacketHandShakes type);
    void onSMSReceived(QString number, QString text);
    void onStartCallRecording(int group);

signals:
    void createNotification(QString message);
    void volumeChanged(int v);
    void playMessage(QString path);
    void playMessage(QUrl path);

    void stopMessage();
    void changeNews(QString txt);
    void requestSyncTime();
    void calendarNext();
    void calendarPrev();
    void timeSync(long sec, long usec);
    void powerOff();

    void startAudioRecord(QString path);
    void endAudioRecord();

    void startSMSReceiver();
    void callHangup();
    void callAccept(QString path);

    void startRecording(int grp);
    void startCallRecording(int grp);

    void endCallBroadcast();

    void broadcast(QString path, uint8_t grpId, PacketHandShakes type);
    void broadcast(QByteArray arr, uint8_t grpId, PacketHandShakes type);

    void sendSMS(QString number, QString contents);
    void migrateAppUI(UIMode mode);

    void enableUI();
    void disableUI();

public slots:
    void onStartRecording(int grp);
    void onEndRecording();

};
#endif // MAINWINDOW_H
