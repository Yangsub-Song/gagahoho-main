#include "smsreceiver.h"

#include <QDebug>

#include <QMap>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusArgument>
#include <QDBusPendingCall>

#define DBUS_BUS_NAME QString("org.freedesktop.ModemManager1")
#define DBUS_OBJECT_PATH QString("/org/freedesktop/ModemManager1")
#define MODEM_MANAGER_OBJECT_MANAGER QString("org.freedesktop.DBus.ObjectManager")
#define MODEM_MANAGER_MESSAGING QString("org.freedesktop.ModemManager1.Modem.Messaging")
#define MODEM_MANAGER_VOICE QString("org.freedesktop.ModemManager1.Modem.Voice")

#define MODEM_MANAGER_MESSAGE_PROPERTIES QString("org.freedesktop.ModemManager1.Sms")
#define MODEM_MANAGER_MODEM QString("org.freedesktop.ModemManager1.Modem")
#define MODEM_MANAGER_CALL QString("org.freedesktop.ModemManager1.Call")

SMSReceiver::SMSReceiver(QObject *parent) : QObject(parent)
{



}

QString SMSReceiver::getAvailableModemPath() {
    QDBusInterface intf(DBUS_BUS_NAME,
                               DBUS_OBJECT_PATH,
                               MODEM_MANAGER_OBJECT_MANAGER,
                               QDBusConnection::systemBus(), this);

    QDBusMessage modemObjects = intf.call("GetManagedObjects");
    QMap<QString, QMap<QString, QVariant>> objectMap;
    QDBusArgument args = modemObjects.arguments().at(0).value<QDBusArgument>();
    args >> objectMap;
    auto keys = objectMap.keys();

    if (keys.size() > 0) {
        return keys.at(0);
    }

    return "";
}

void SMSReceiver::turnModem(QString path, bool onOff) {
    QDBusInterface intf(DBUS_BUS_NAME,
                               path,
                               MODEM_MANAGER_MODEM,
                               QDBusConnection::systemBus(), this);

    if (intf.isValid()) {
        intf.call("Enable", QVariant::fromValue(onOff));
    }
}
QString SMSReceiver::getOwnNumber(QString path) {
    QDBusInterface intf(DBUS_BUS_NAME,
                               path,
                               MODEM_MANAGER_MODEM,
                               QDBusConnection::systemBus(), this);

    if (intf.isValid()) {
        QVariant numbers = intf.property("OwnNumbers");
        return numbers.toStringList().at(0);
    }
    return "";
}

bool SMSReceiver::registerMessageAdded(QString path) {
    return QDBusConnection::systemBus().connect(DBUS_BUS_NAME,
                                                path,
                                                MODEM_MANAGER_MESSAGING,
                                                "Added",
                                                this,
                                                SLOT(onSMSNotified(QDBusObjectPath, bool)));
}

bool SMSReceiver::registerCallAdded(QString path) {
    return QDBusConnection::systemBus().connect(DBUS_BUS_NAME,
                                                path,
                                                MODEM_MANAGER_VOICE,
                                                "CallAdded",
                                                this,
                                                SLOT(onCallAdded(QDBusObjectPath)));
}

bool SMSReceiver::registerCallDeleted(QString path) {
    return QDBusConnection::systemBus().connect(DBUS_BUS_NAME,
                                                path,
                                                MODEM_MANAGER_VOICE,
                                                "CallDeleted",
                                                this,
                                                SLOT(onCallDeleted(QDBusObjectPath)));
}

bool SMSReceiver::registerCallStateChanged(QString path) {
    // onCallStateChanged

    return QDBusConnection::systemBus().connect(DBUS_BUS_NAME,
                                                path,
                                                MODEM_MANAGER_CALL,
                                                "StateChanged",
                                                this,
                                                SLOT(onCallStateChanged(int, int, uint)));
}

void SMSReceiver::deleteMessage(QString modemPath, QString messagePath) {
    QDBusInterface intf(DBUS_BUS_NAME,
                               modemPath,
                               MODEM_MANAGER_MESSAGING,
                               QDBusConnection::systemBus(), this);
    intf.call("Delete", QVariant::fromValue(QDBusObjectPath(messagePath)));
}

void SMSReceiver::deleteCall(QString modemPath, QString callPath) {
    QDBusInterface intf(DBUS_BUS_NAME,
                               modemPath,
                               MODEM_MANAGER_VOICE,
                               QDBusConnection::systemBus(), this);
    intf.call("DeleteCall", QVariant::fromValue(QDBusObjectPath(callPath)));
}

void SMSReceiver::deleteAllMessages(QString path) {
    QDBusInterface intf(DBUS_BUS_NAME,
                               path,
                               MODEM_MANAGER_MESSAGING,
                               QDBusConnection::systemBus(), this);

    QDBusMessage messageObjects = intf.call("List");
    QDBusArgument args = messageObjects.arguments().at(0).value<QDBusArgument>();
    QList<QDBusObjectPath> messageList;

    args >> messageList;

    if (messageList.length() > 0) {
        Q_FOREACH (const QDBusObjectPath& p, messageList) {
            qDebug() << "smss: " << p.path();
            intf.call("Delete", QVariant::fromValue(p));
        }
    }
}

void SMSReceiver::deleteAllCalls(QString path) {
    QDBusInterface intf(DBUS_BUS_NAME,
                               path,
                               MODEM_MANAGER_VOICE,
                               QDBusConnection::systemBus(), this);

    QDBusMessage messageObjects = intf.call("ListCalls");
    QDBusArgument args = messageObjects.arguments().at(0).value<QDBusArgument>();
    QList<QDBusObjectPath> messageList;

    args >> messageList;

    if (messageList.length() > 0) {
        Q_FOREACH (const QDBusObjectPath& p, messageList) {
            qDebug() << "calls: " << p.path();
            intf.call("DeleteCall", QVariant::fromValue(p));
        }
    }
}

void SMSReceiver::onStartReceiver() {
    modemPath = getAvailableModemPath();

    qDebug() << "modemPath : " << modemPath;
    turnModem(modemPath, true);
    getOwnNumber(modemPath);

    if (modemPath.length() > 0) {
        deleteAllMessages(modemPath);
        deleteAllCalls(modemPath);
        registerMessageAdded(modemPath);
        registerCallAdded(modemPath);
        registerCallDeleted(modemPath);
    }
}

QString SMSReceiver::getMessageText(QString messagePath) {
    QDBusInterface intf(DBUS_BUS_NAME,
                               messagePath,
                               MODEM_MANAGER_MESSAGE_PROPERTIES,
                               QDBusConnection::systemBus(), this);

    QVariant t = intf.property("Text");

    return t.toString();
}

QString SMSReceiver::getMessageFrom(QString messagePath) {
    QDBusInterface intf(DBUS_BUS_NAME,
                               messagePath,
                               MODEM_MANAGER_MESSAGE_PROPERTIES,
                               QDBusConnection::systemBus(), this);

    QVariant t = intf.property("Number");

    return t.toString();
}

QString SMSReceiver::getCallFrom(QString callPath) {
    QDBusInterface intf(DBUS_BUS_NAME,
                               callPath,
                               MODEM_MANAGER_CALL,
                               QDBusConnection::systemBus(), this);

    QVariant t = intf.property("Number");

    return t.toString();
}

void SMSReceiver::onSMSNotified(QDBusObjectPath path, bool received) {
    qDebug() << "sms notified : " << path.path();
    QString t = getMessageText(path.path());
    QString number = getMessageFrom(path.path());

    if (received) {
        emit smsReceived(number, t);
    }
}


void SMSReceiver::onHangupCall() {
    QDBusInterface intf(DBUS_BUS_NAME,
                               modemPath,
                               MODEM_MANAGER_VOICE,
                               QDBusConnection::systemBus(), this);
    intf.call("HangupAll");
}


void SMSReceiver::onCallAdded(QDBusObjectPath path) {

    QString number = getCallFrom(path.path());

    qDebug() << "onCallAdded : " << number;
    qDebug() << "onCallAdded : " << path.path();

    registerCallStateChanged(path.path());

    currentCallPath = path.path();

    // MODEM_MANAGER_CALL
    emit phoneCall(path.path(), number);
}

void SMSReceiver::onCallDeleted(QDBusObjectPath path) {

    QString number = getCallFrom(path.path());

    qDebug() << "onCallDeleted : " << path.path();

    emit phoneCallHangup();
}

void SMSReceiver::onCallAccepted(QString path) {
    QDBusInterface intf(DBUS_BUS_NAME,
                               path,
                               MODEM_MANAGER_CALL,
                               QDBusConnection::systemBus(), this);


    intf.call("Accept");
    qDebug() << "onCallAccepted : " << path;
}

void SMSReceiver::onCallStateChanged(int oldState, int newState, uint reason) {

    qDebug() << "onCallStateChanged oldState : " << QString::number(oldState);
    qDebug() << "onCallStateChanged : " << QString::number(newState);
    qDebug() << "onCallStateChanged Reason : " << QString::number(reason);

    // MM_CALL_STATE_TERMINATED 7
    if (newState == 7) {
        qDebug() << "onCallHanguped, Path : " << currentCallPath;

        if (currentCallPath.size() > 0) {
            QDBusConnection::systemBus().disconnect(DBUS_BUS_NAME,
                                                            currentCallPath,
                                                            MODEM_MANAGER_CALL,
                                                            "StateChanged",
                                                            this,
                                                            SLOT(onCallStateChanged(int, int, int)));
            deleteCall(modemPath, currentCallPath);
            currentCallPath = "";
            emit phoneCallEnd();
        }
    }

}

void SMSReceiver::onSendSMS(QString number, QString contents) {

    QDBusInterface intf(DBUS_BUS_NAME,
                               modemPath,
                               MODEM_MANAGER_MESSAGING,
                               QDBusConnection::systemBus(), this);

    QVariant text = QVariant::fromValue(contents);

    QVariantMap map;
    map.insert(QString("text"), QVariant::fromValue(contents));
    map.insert(QString("number"), QVariant::fromValue(number));
    QDBusMessage message = intf.call("Create", map);

    qDebug() << "onSendSMS : " << number << ", conetnts : " + contents;

    QString smsPath = message.arguments().at(0).value<QDBusObjectPath>().path();

    if (smsPath.size() > 0) {
        QDBusInterface sendIntf(DBUS_BUS_NAME,
                                   smsPath,
                                   MODEM_MANAGER_MESSAGE_PROPERTIES,
                                   QDBusConnection::systemBus(), this);
        sendIntf.asyncCall("Send");
    }

}
