#include "mainwindow.h"
#include "networkmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QJsonArray>
#include <QGraphicsDropShadowEffect>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    initUI();

    qDebug() << "----IBUSKO---- MainWindow(QWidget *parent)";
    connect(
        &NetworkManager::instance(),
        &NetworkManager::responseReceived,
        this,
        &MainWindow::handleServerResponse);
    loadRoomsFromDatabase();
}
void MainWindow::setUserRole(const QString &role) {
    userRole = role;
    configureUIBasedOnRole();
}
void MainWindow::configureUIBasedOnRole() {
    bool isAdmin = (userRole == "admin");

    addRoomButton->setVisible(isAdmin);
    addDeviceButton->setVisible(isAdmin);
    addScenarioButton->setVisible(isAdmin);
    addScenarioButton->setVisible(isAdmin);
}
MainWindow::~MainWindow() {}
auto addShadowEffect = [](QPushButton* button) {
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setOffset(0, 3);
    shadowEffect->setColor(QColor(0, 0, 0, 127));
    shadowEffect->setBlurRadius(5);
    button->setGraphicsEffect(shadowEffect);
};
void MainWindow::initUI() {
    logoutButton = new QPushButton("Выйти", this);
    connect(logoutButton, &QPushButton::clicked, this, [this](){ emit backToMain(); });

    addRoomButton = new QPushButton("Добавить комнату", this);
    connect(addRoomButton, &QPushButton::clicked, this, &MainWindow::onAddRoomButtonClicked);

    addDeviceButton = new QPushButton("Добавить устройство", this);
    connect(addDeviceButton, &QPushButton::clicked, this, &MainWindow::onAddDeviceButtonClicked);

    scenarioButton = new QPushButton("Сценарии", this);
    connect(scenarioButton, &QPushButton::clicked, this, &MainWindow::onScenarioButtonClicked);

    addScenarioButton = new QPushButton("Добавить сценарий", this);
    connect(addScenarioButton, &QPushButton::clicked, this, &MainWindow::onAddScenarioButtonClicked);

    allDevicesButton = new QPushButton("Все устройства", this);
    connect(allDevicesButton, &QPushButton::clicked, this, &MainWindow::onAllDevicesButtonClicked);

    addDeviceButton->setFixedSize(200, 50);
    scenarioButton->setFixedSize(200, 50);
    allDevicesButton->setFixedSize(200, 50);
    addRoomButton->setFixedSize(200, 50);
    addScenarioButton->setFixedSize(200, 50);
    logoutButton->setFixedSize(100, 35);

    sideMenu = new QWidget(this);
    sideMenuLayout = new QVBoxLayout(sideMenu);
    sideMenuLayout->addWidget(scenarioButton);
    sideMenuLayout->addWidget(allDevicesButton);
    sideMenuLayout->addStretch();


    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->addWidget(addRoomButton);
    headerLayout->addWidget(addDeviceButton);
    headerLayout->addWidget(addScenarioButton);
    headerLayout->addStretch();
    headerLayout->addWidget(logoutButton);

    displayWidget = new QStackedWidget(this);
    QWidget *defaultView = new QWidget(displayWidget);
    gridLayout = new QGridLayout(defaultView);
    displayWidget->addWidget(defaultView);

    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addWidget(sideMenu);
    mainLayout->addWidget(displayWidget);

    QVBoxLayout *windowLayout = new QVBoxLayout(this);
    windowLayout->addLayout(headerLayout);
    windowLayout->addLayout(mainLayout);

    setLayout(windowLayout);
    setWindowTitle("Умный дом");
    resize(800, 600);

    currentRoom = QString();

    addShadowEffect(scenarioButton);
    addShadowEffect(allDevicesButton);
    addShadowEffect(addDeviceButton);
    addShadowEffect(addRoomButton);
    addShadowEffect(logoutButton);
    addShadowEffect(addScenarioButton);

    setStyleSheet("QWidget {background-color: #6974e4;}");

    QString buttonStyle = "QPushButton {"
                          "background-color: #b3a2ee;"
                          "border-radius: 25px;"
                          "padding: 10px;"
                          "font: bold 16px 'New York';"
                          "}"
                          "QPushButton:hover {"
                          "background-color: #ffbaf5;"
                          "}";
    scenarioButton->setObjectName("scenarioButton");
    scenarioButton->setStyleSheet(buttonStyle);
    allDevicesButton->setObjectName("allDevicesButton");
    allDevicesButton->setStyleSheet(buttonStyle);
    logoutButton -> setStyleSheet("QPushButton {""background-color: #f78dae; ""border-radius: 10px;""padding: 6px;""font: bold 16px  'New york';""}""QPushButton:hover {""background-color: #ffbaf5;""}");
    addRoomButton->setStyleSheet(buttonStyle);
    addDeviceButton ->setStyleSheet(buttonStyle);
    addScenarioButton->setStyleSheet(buttonStyle);

}

void MainWindow::loadRoomsFromDatabase()
{
    QJsonObject request;
    request["action"] = "loadRooms";
    NetworkManager::instance().sendRequest(request);
}

void MainWindow::onAllDevicesButtonClicked()
{
    loadDevicesFromDatabase();
}

void MainWindow::loadDevicesFromDatabase()
{
    QJsonObject request;
    request["action"] = "loadAllDevices";
    NetworkManager::instance().sendRequest(request);
}

void MainWindow::onAddRoomButtonClicked()
{
    QStringList predefinedRooms = {"Баня", "Подвал", "Ванная", "Гостиная", "Детская", "Гараж", "Сауна", "Кухня", "Чердак"};
    QString selectedRoom = QInputDialog::getItem(this, "Добавить помещение", "Выберите помещение:", predefinedRooms, 0, false);

    if (selectedRoom.isEmpty() || roomDevices.contains(selectedRoom)) {
        QMessageBox::warning(this, "Ошибка", roomDevices.contains(selectedRoom) ? "Комната уже существует." : "Не выбрано.");
        return;
    }

    QJsonObject request;
    request["action"] = "addRoom";
    request["roomName"] = selectedRoom;
    NetworkManager::instance().sendRequest(request);
}
void MainWindow::requestRoomDevices(const QString &roomName) {
    QJsonObject request;
    request["action"] = "loadRoomDevices";
    request["room"] = roomName;
    NetworkManager::instance().sendRequest(request);

}
void MainWindow::onAddDeviceButtonClicked()
{
    if (currentRoom.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите комнату.");
        return;
    }

    QStringList predefinedDevices = {"Лампа", "Термостат", "Камера", "Датчик движения"};
    QString selectedDevice = QInputDialog::getItem(this, "Добавить устройство", "Выберите устройство:", predefinedDevices, 0, false);

    if (selectedDevice.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не выбрано.");
        return;
    }

    QJsonObject request;
    request["action"] = "addDevice";
    request["roomName"] = currentRoom;
    request["deviceName"] = selectedDevice;
    NetworkManager::instance().sendRequest(request);
}
void MainWindow::onScenarioButtonClicked() {
    QJsonObject request;
    request["action"] = "loadScenarios";
    NetworkManager::instance().sendRequest(request);
}
void MainWindow::onAddScenarioButtonClicked()
{
    QStringList scenarios = {"Наступила ночь", "Наступило утро", "Стало холодно", "Стало жарко", "Наступила зима"};
    QString selectedScenario = QInputDialog::getItem(this, "Добавить сценарий", "Выберите сценарий:", scenarios, 0, false);

    if (!selectedScenario.isEmpty()) {
        QJsonObject request;
        request["action"] = "addScenario";
        request["scenarioName"] = selectedScenario;
        NetworkManager::instance().sendRequest(request);
    } else {
        QMessageBox::information(this, "Информация", "Добавление сценария отменено.");
    }
}

void MainWindow::handleServerResponse(const QJsonObject &response)
{
    QString action = response["action"].toString();
    qDebug() << "-----IBUSKO----- Received response from server:" << action;

    if (action == "loadRooms") {
        handleLoadRoomsResponse(response);
    } else if (action == "loadAllDevices") {
        handleLoadAllDevicesResponse(response);
    } else if (action == "addRoom") {
        handleAddRoomResponse(response);
    } else if (action == "addDevice") {
        handleAddDeviceResponse(response);
    } else if (action=="loadScenarios"){
        handleLoadScenariosResponse(response);
    }else if(action=="addScenario"){
        handleAddScenarioResponse(response);
    }else if(action=="loadRoomDevices")
        handleLoadRoomDevicesResponse(response);
}

void MainWindow::handleLoadRoomsResponse(const QJsonObject &response) {
    QJsonArray roomsArray = response["rooms"].toArray();

    QLayoutItem *item;
    QList<QWidget*> widgetsToRemove;
    for (int i = 0; i < sideMenuLayout->count(); ++i) {
        item = sideMenuLayout->itemAt(i);
        QWidget *widget = item ? item->widget() : nullptr;
        if (widget && widget != scenarioButton && widget != allDevicesButton) {
            widgetsToRemove.append(widget);
        }
    }

    for (QWidget *widget : widgetsToRemove) {
        sideMenuLayout->removeWidget(widget);
        delete widget;
    }

    for (const QJsonValue &value : roomsArray) {
        QString roomName = value.toString();

        QPushButton *roomButton = new QPushButton(roomName, this);
        roomButton->setFixedSize(200, 50);
        connect(roomButton, &QPushButton::clicked, this, [this, roomName]() {
            currentRoom = roomName;
            requestRoomDevices(roomName);
        });
        QString buttonStyle = "QPushButton {"
                              "background-color: #b3a2ee;"
                              "border-radius: 25px;"
                              "padding: 10px;"
                              "font: bold 16px 'New York';"
                              "}"
                              "QPushButton:hover {"
                              "background-color: #ffbaf5;"
                              "}";
        roomButton->setObjectName(roomName);
        roomButton->setStyleSheet(buttonStyle);
        addShadowEffect(roomButton);
        sideMenuLayout->insertWidget(sideMenuLayout->count() - 1, roomButton);
    }


    sideMenu->update();
}



void MainWindow::handleLoadRoomDevicesResponse(const QJsonObject &response) {
    QString roomName = response["room"].toString();
    QJsonArray devicesArray = response["roomDevices"].toArray();
    QVector<QString> devices;
    if (currentRoom == roomName) {
        clearDisplay();

        for (const QJsonValue &deviceValue : devicesArray) {
            QString deviceName = deviceValue.toString();
            devices.push_back(deviceName);
            displayItemsInGrid(devices, true);
        }
        }

}
void MainWindow::handleLoadAllDevicesResponse(const QJsonObject &response) {
    QJsonArray devicesArray = response["devices"].toArray();

    QVector<QString> devices;
    for (const QJsonValue &device : devicesArray) {
        QString deviceName = device.toString();
        devices.push_back(deviceName);
        displayItemsInGrid(devices, true);
    }
}
void MainWindow::handleAddRoomResponse(const QJsonObject &response) {
    bool status = response["success"].toBool();
    QString roomName = response["roomName"].toString();

    if (status) {

        QMessageBox::information(this, "Success", "Room added successfully: " + roomName);
    } else {
        QMessageBox::warning(this, "Error", "Failed to add room: " + response["message"].toString());
    }
    connect(
        &NetworkManager::instance(),
        &NetworkManager::responseReceived,
        this,
        &MainWindow::handleServerResponse);
    loadRoomsFromDatabase();
}

void MainWindow::handleAddDeviceResponse(const QJsonObject &response)
{
    bool status = response["success"].toBool();
    QString deviceName = response["deviceName"].toString();
    QString roomName = response["roomName"].toString();

    if (status) {
        if (roomDevices.contains(roomName)) {
            roomDevices[roomName].append(deviceName);
        } else {
            roomDevices[roomName] = QVector<QString>({deviceName});
        }

        loadDevicesFromDatabase();
        QMessageBox::information(this, "Success", "Device added successfully: " + deviceName);
    } else {
        QMessageBox::warning(this, "Error", "Failed to add device: " + response["message"].toString());
    }
}
void MainWindow::handleAddScenarioResponse(const QJsonObject &response)
{
    QMessageBox::information(this, "Сценарий", response["message"].toString());
}

void MainWindow::handleLoadScenariosResponse(const QJsonObject &response) {
    QJsonArray scenariosArray = response["scenarios"].toArray();
    QVector<QString> scenarios;
    for (const QJsonValue &scenario : scenariosArray) {
        QString scenarioName = scenario.toString();
        scenarios.push_back(scenarioName);
    displayItemsInGrid(scenarios, false);
    }
}



void MainWindow::displayItemsInGrid(const QVector<QString> &items, bool isDevices)
{
    clearGridLayout(gridLayout);

    int row = 0, col = 0;
    for (const QString &item : items) {
        QPushButton *button = new QPushButton(item, this);
        if (isDevices){
            button->setFixedSize(100, 100);
        }else{
           button->setFixedSize(150, 50);
        }

        button->setCheckable(true);
        QString buttonStyle = "QPushButton {"
                              "background-color: #b3a2ee;"
                              "border-radius: 25px;"
                              "padding: 10px;"
                              "font: bold 16px 'New York';"
                              "}"
                              "QPushButton:hover {"
                              "background-color: #ffbaf5;"
                              "}";
        button->setObjectName(item);
        button->setStyleSheet(buttonStyle);
        addShadowEffect(button);
        connect(button, &QPushButton::clicked, this, [button]() {
            button->setStyleSheet(button->isChecked() ? "background-color: green;" : "background-color: red;");
        });
        gridLayout->addWidget(button, row, col);
        if (++col >= 3) {
            col = 0;
            ++row;
        }
    }
}

void MainWindow::clearGridLayout(QLayout *layout)
{
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
}
void MainWindow::clearDisplay()
{
    QLayoutItem *item;
    while ((item = gridLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

}
