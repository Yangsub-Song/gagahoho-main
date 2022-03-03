#ifndef SMSRECEIVER_H
#define SMSRECEIVER_H

#include <QObject>
#include <QDBusObjectPath>

class SMSReceiver : public QObject
{
    Q_OBJECT
public:
    explicit SMSReceiver(QObject *parent = nullptr);

private:
    QString modemPath;
    QString currentCallPath;

    QString getAvailableModemPath();
    bool registerMessageAdded(QString path);
    bool registerCallAdded(QString path);
    bool registerCallDeleted(QString path);
    bool registerCallStateChanged(QString path);

    void deleteAllMessages(QString path);
    void deleteAllCalls(QString path);
    void deleteMessage(QString modemPath, QString messagePath);
    void deleteCall(QString modemPath, QString callPath);

    QString getMessageText(QString messagePath);
    QString getMessageFrom(QString messagePath);
    QString getOwnNumber(QString path);
    void turnModem(QString path, bool onOff);
    QString getCallFrom(QString callPath);

private slots:
    void onSMSNotified(QDBusObjectPath path, bool received);
    void onCallAdded(QDBusObjectPath path);
    void onCallDeleted(QDBusObjectPath path);
    void onCallStateChanged(int oldState, int newState, uint reason);

public slots:
    void onStartReceiver();
    void onHangupCall();
    void onCallAccepted(QString path);
    void onSendSMS(QString number, QString contents);

signals:
    void smsReceived(QString number, QString text);
    void phoneCall(QString path, QString number);
    void phoneCallHangup();
    void phoneCallEnd();

};

#endif // SMSRECEIVER_H
