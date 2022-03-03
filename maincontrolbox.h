#ifndef MAINCONTROLBOX_H
#define MAINCONTROLBOX_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

enum MainControlButtons {
    MAINCONTROL_BROADCAST_ALL,
    MAINCONTROL_BROADCAST_GRP,
    MAINCONTROL_BROADCAST_IND,
};
Q_DECLARE_METATYPE(MainControlButtons)

class MainControlBox : public QWidget
{
    Q_OBJECT
public:
    explicit MainControlBox(QWidget *parent = nullptr);

private:
    QLabel logo;
    QPushButton bcAll;
    QPushButton bcGrp;
    QPushButton bcInd;

signals:
    void onBroadcastClicked(int button);

};

#endif // MAINCONTROLBOX_H
