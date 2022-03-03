#ifndef MAINDISPLAY_H
#define MAINDISPLAY_H

#include <QWidget>

namespace Ui {
class MainApp;
}

enum UIMode {
    UIMODE_IDLE,
    UIMODE_BROADCAST_GROUP_MENU,
    UIMODE_BROADCAST_GROUP_SELECTED,
    UIMODE_BROADCAST_ALL_SELECTED,
    UIMODE_BROADCAST_INDIVIDUAL,
    UIMODE_BROADCAST_UNKNOWN_GROUP_SELECTED,
};
Q_DECLARE_METATYPE(UIMode)

enum BroadcastMode {
    BROADCAST_MODE_NONE,
    BROADCAST_MODE_ALL,
    BROADCAST_MODE_GROUP,
    BROADCAST_MODE_INDIVIDUAL
};

class MainApp : public QWidget
{
    Q_OBJECT

public:
    explicit MainApp(QWidget *parent = nullptr);
    ~MainApp();

private slots:
    void on_buttonBroadcastAll_clicked();
    void on_buttonBroadcastGrp_clicked();
    void on_buttonBroadcastIndividual_clicked();

public slots:
    void onNotification(QString msg);
    void onMigrateUI(UIMode mode);
    void onDisableUI();
    void onEnableUI();

private:
    void onBroadcastButtonClicked();

    Ui::MainApp *ui;
    UIMode currentUIMode;

    int grpSelected;

signals:
    void showNotification(QString msg);

    void startBroadcast(int grp);
    void endBroadcast();
};

#endif // MAINDISPLAY_H
