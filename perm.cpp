#include "perm.h"
#include "setting.h"
#include "util/filedownloader.h"
#include "permdata.h"
#include <QFile>
#include <QList>
#include <QMutex>

#define PERM_DEFAULT_DB_PATH "/var/lib/gagahoho/perm.csv"

Perm::Perm(QObject *parent) : QObject(parent)
{
    QFile permFile(PERM_DEFAULT_DB_PATH);

    updateLock = new QMutex();

    if (permFile.exists()) {
        applyPerm();
    }

}

bool Perm::isDataExist() {
    QFile permFile(PERM_DEFAULT_DB_PATH);
    return permFile.exists();
}

void Perm::updatePerm() {

    fileDownloader = new FileDownloader(QString(SERVER_ADDRESS) + QString(SERVER_PATH_PERM_DB),
                                        PERM_DEFAULT_DB_PATH);

    connect(fileDownloader, &FileDownloader::downloaded, [this]() {
        applyPerm();
    });

    fileDownloader->download();
}

bool Perm::isAccessable(QString phoneNumber) {

    updateLock->lock();
    Q_FOREACH (PermData* perm, permList) {
        qDebug() << "Compare " << perm->getPhoneNumber() << ", : " << phoneNumber;

        if (QString::compare(perm->getPhoneNumber(), phoneNumber) == 0) {
            if(perm->getCanBroadcast() == 1) {
                updateLock->unlock();
                return true;
            }
        }
    }

    updateLock->unlock();
    return false;
}


void Perm::applyPerm() {
    QFile permFile(PERM_DEFAULT_DB_PATH);

    qDebug() << "applyPerm";

    if (permFile.exists()) {
        updateLock->lock();
        permFile.open(QIODevice::ReadOnly | QIODevice::Text);
        QString permData = QString(permFile.readAll());

        // clearPerm
        Q_FOREACH (PermData* perm, permList) {
            delete perm;
        }
        permList.clear();

        if (permData.size() > 0) {
            QStringList permLine = permData.split("\n");
            for(int i = 1; i < permLine.size(); i++) {
                QString csvLine = permLine.at(i);

                if (csvLine.size() > 0) {
                    QStringList csvElements = csvLine.split(",");

                    if (csvElements.size() >= 7) {
                        PermData *d = new PermData(
                                    csvElements.at(0).toInt(),
                                    csvElements.at(1).toInt(),
                                    csvElements.at(2).toInt(),
                                    csvElements.at(3),
                                    csvElements.at(4),
                                    csvElements.at(5),
                                    csvElements.at(6));

                        qDebug() << "Accessible : " << QString::number(d->getCanBroadcast()) <<
                                    "Phone : " << QString(d->getPhoneNumber());

                        permList.append(d);
                    }
                }

            }
        }
        updateLock->unlock();

    }
}
