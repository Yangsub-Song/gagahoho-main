#ifndef PERMDATA_H
#define PERMDATA_H

#include <QString>

class PermData
{
public:
    PermData(int canBroadcast,
             int groupId,
             int homeId,
             QString name,
             QString phoneNumber,
             QString address,
             QString destNumber) {

        this->canBroadcast = canBroadcast;
        this->groupId = groupId;
        this->homeId = homeId;
        this->name = name;
        this->phoneNumber = phoneNumber;
        this->address = address;
        this->destNumber = destNumber;
    }


    int getCanBroadcast() {
        return canBroadcast;
    }

    int getGroupId() {
        return groupId;
    }

    int getHomeId() {
        return homeId;
    }

    QString getName() {
        return name;
    }

    QString getPhoneNumber() {
        return phoneNumber;
    }

    QString getAddress() {
        return address;
    }

    QString getDestNumber() {
        return destNumber;
    }

private:
    int canBroadcast;
    int groupId;
    int homeId;
    QString name;
    QString phoneNumber;
    QString address;
    QString destNumber;
};

#endif // PERMDATA_H
