#ifndef PERM_H
#define PERM_H

#include <QObject>

class FileDownloader;
class PermData;
class QMutex;

class Perm : public QObject
{
    Q_OBJECT
public:
    explicit Perm(QObject *parent = nullptr);
    bool isDataExist();
    void updatePerm();

    bool isAccessable(QString phoneNumber);

private:
    FileDownloader *fileDownloader;
    QList<PermData*> permList;

    void applyPerm();
    QMutex *updateLock;

signals:

};

#endif // PERM_H
