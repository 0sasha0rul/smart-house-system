#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QListWidget>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadRoomsFromDatabase();
    void setUserRole(const QString &role);

signals:
    void backToMain();
    void devicesLoaded(const QVector<QString> &devices);

private slots:
    //void onNoticeButtonClicked();
    void onAddRoomButtonClicked();
    void onAddDeviceButtonClicked();
    void onAllDevicesButtonClicked();
    void onScenarioButtonClicked();
    void onAddScenarioButtonClicked();
    void onRoomSensorsButtonClicked();
    void requestRoomDevices(const QString &roomName);
    void handleLoadAllDevicesForScenarios(const QJsonObject &response);
    void handleServerResponse(const QJsonObject &response);
    void handleLoadRoomsResponse(const QJsonObject &response);
    void handleLoadAllDevicesResponse(const QJsonObject &response);
    void handleAddRoomResponse(const QJsonObject &response);
    void handleAddDeviceResponse(const QJsonObject &response);
    void handleLoadScenariosResponse(const QJsonObject &response);
    void handleAddScenarioResponse(const QJsonObject &response);
    void handleLoadRoomDevicesResponse(const QJsonObject &response);
    void handleToggleDeviceResponse(const QJsonObject &response);
    void handleLoadRoomSensorsResponse(const QJsonObject &response);


private:

    QPushButton *logoutButton;
    //QPushButton *noticeButton;
    QPushButton *addRoomButton;
    QPushButton *addDeviceButton;
    QPushButton *scenarioButton;
    QPushButton *addScenarioButton;
    QPushButton *allDevicesButton;
    QPushButton *roomSensorsButton;
    QListWidget *roomsListWidget;

    QWidget *centralWidget;
    QWidget *sideMenu;
    QVBoxLayout *sideMenuLayout;
    QVBoxLayout *roomButtonsLayout;
    QStackedWidget *displayWidget;
    QGridLayout *gridLayout;

    QString userRole;
    QString currentRoom;
    QMap<QString, QVector<QString>> roomDevices;

    void initUI();
    void configureUIBasedOnRole();
    void displayItemsInGrid(const QVector<QString> &items, const QString roomName, bool isDevices);
    void displayScenariosInGrid(QVector<QString> &scenarios);
    void displayAllDevicesInGrid(const QVector<QString> &items);
    void displayAllSensorsInGrid(const QVector<QString> &items);
    void clearGridLayout(QLayout *layout);
    void clearDisplay();

};

#endif // MAINWINDOW_H
