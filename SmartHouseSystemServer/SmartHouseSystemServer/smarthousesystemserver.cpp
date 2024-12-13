#include "smarthousesystemserver.h"
#include "databasemanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QSqlQuery>
#include <QNetworkReply>

SmartHouseSystemServer::SmartHouseSystemServer(QObject *parent)
    : QTcpServer(parent), networkManager(new QNetworkAccessManager(this)) {
    if (!this->listen(QHostAddress::Any, 1234)) {
        qDebug() << "Server could not start!";
    } else {
        qDebug() << "Server started!";
    }

    connect(this, &QTcpServer::newConnection, this, [this]() {
        QTcpSocket *socket = this->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            while (socket->canReadLine()) {
                QByteArray line = socket->readLine();
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
                QJsonObject request = doc.object();

                qDebug() << "----IBUSKO---- QJsonObject request = doc.object();" << request;

                if (parseError.error != QJsonParseError::NoError) {
                    return;
                }

                QString action = request["action"].toString();
                if (action == "register") {
                    processRegistrationRequest(socket, request);
                } else if (action == "login") {
                    processLoginRequest(socket, request);
                } else if (action == "loadRooms") {
                    processRoomLoadRequest(socket, request);
                } else if (action == "addRoom") {
                    processAddRoomRequest(socket, request);
                } else if (action == "loadAllDevices") {
                    processLoadDevicesRequest(socket, request);
                } else if (action == "loadScenarios") {
                    processLoadScenariosRequest(socket, request);
                } else if (action == "addScenario") {
                    processAddScenarioRequest(socket, request);
                } else if (action == "addDevice") {
                    processAddDeviceRequest(socket, request);
                } else if (action=="loadRoomDevices"){
                    processRoomDevicesRequest(socket, request);
                }else if (action=="toggleDevice"){
                    processToggleDeviceRequest(socket, request);
                }else if (action=="loadScenarioDevices"){
                    processLoadScenarioDevicesRequest(socket, request);
                }else if (action=="toggleScenario"){
                    processToggleScenarioRequest(socket, request);
                }else if (action=="loadRoomSensors"){
                    processLoadRoomSensors(socket, request);
                }

            }
        });

        connect(socket, &QTcpSocket::disconnected, this, [socket]() {
            qDebug() << "Socket disconnected from:" << socket->peerAddress().toString();
            socket->deleteLater();
        });
    });

}

void SmartHouseSystemServer::processRegistrationRequest(QTcpSocket *socket, const QJsonObject &request) {
    QString username = request["username"].toString();
    QString password = request["password"].toString();
    QString role = request["role"].toString();
    bool adminExists = DatabaseManager::instance().adminExists();
    bool checkRole = ((role=="admin")&&(adminExists))? true: false;
    if (DatabaseManager::instance().userExists(username)) {
        QJsonObject response;
        response["success"] = false;
        response["message"] = "Username already exists.";
        socket->write(QJsonDocument(response).toJson());
        socket->flush();

        return;
    }
    if (checkRole){
        QJsonObject response;
        response["success"] = false;
        response["message"] = "Admin already exist. You cannot be admin.";
        socket->write(QJsonDocument(response).toJson());
        socket->flush();

        return;
    }

    if (DatabaseManager::instance().registerUser(username, password, role)) {
        QJsonObject response;
        response["success"] = true;
        response["message"] = "User registered successfully.";
        socket->write(QJsonDocument(response).toJson());
    } else {
        QJsonObject response;
        response["success"] = false;
        response["message"] = "Registration failed.";
        socket->write(QJsonDocument(response).toJson());
    }

    socket->flush();

}

void SmartHouseSystemServer::processLoginRequest(QTcpSocket *socket, const QJsonObject &request) {
    QString username = request["username"].toString();
    QString password = request["password"].toString();

    if (DatabaseManager::instance().authenticateUser(username, password)) {
        QString userRole = DatabaseManager::instance().getUserRole(username);
        QJsonObject response;
        response["authenticated"] = true;
        response["role"] = userRole;
        response["message"] = "Login successful.";
        socket->write(QJsonDocument(response).toJson());
    } else {
        QJsonObject response;
        response["authenticated"] = false;
        response["message"] = "Invalid username or password.";
        socket->write(QJsonDocument(response).toJson());
    }

    socket->flush();

}

void SmartHouseSystemServer::processRoomLoadRequest(QTcpSocket *socket, const QJsonObject &request) {
    QJsonArray roomList = QJsonArray::fromStringList(DatabaseManager::instance().getAllRooms());
    QJsonObject response;
    response["action"] = "loadRooms";
    response["rooms"] = roomList;
    qDebug()<< "----IBUSKO---- processRoomLoadRequest " << response;
    socket->write(QJsonDocument(response).toJson());
    socket->flush();

}
void SmartHouseSystemServer::processRoomDevicesRequest(QTcpSocket *socket, const QJsonObject &request) {
    QString roomName = request["room"].toString();
    QStringList devices = DatabaseManager::instance().getDevicesForRoom(roomName);

    QJsonArray deviceArray;
    for (const QString &device : devices) {
        deviceArray.append(device);
    }

    QJsonObject response;
    response["action"] = "loadRoomDevices";
    response["room"] = roomName;
    response["roomDevices"] = deviceArray;

    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}
void SmartHouseSystemServer::processAddRoomRequest(QTcpSocket *socket, const QJsonObject &request) {
    QString roomName = request["roomName"].toString();
    bool success = DatabaseManager::instance().addRoom(roomName);

    QJsonObject response;
    response["action"] = "addRoom";
    response["success"] = success;
    response["message"] = success ? "Room added successfully." : "Failed to add room.";
    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}

void SmartHouseSystemServer::processLoadDevicesRequest(QTcpSocket *socket, const QJsonObject &request) {
    QMap<QString, QStringList> devices = DatabaseManager::instance().getAllDevices();
    QJsonObject response;
    response["action"] = "loadAllDevices";

    QJsonArray devicesArray;
    for (auto it = devices.begin(); it != devices.end(); ++it) {
        QJsonObject deviceObject;
        deviceObject["name"] = it.key();
        deviceObject["rooms"] = QJsonArray::fromStringList(it.value());
        devicesArray.append(deviceObject);
    }
    response["devices"] = devicesArray;
    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}
void SmartHouseSystemServer::determineDeviceGroup(const QString &deviceType,QString &deviceGroup, QJsonObject &parameters){
    if (deviceType == "лампа"||deviceType == "шторы") {
        deviceGroup = "освещение";
        parameters["on"] = false;
    } else if (deviceType == "кондиционер"||(deviceType == "обогреватель")||(deviceType == "тёплый пол")) {
        deviceGroup = "отопление";
        parameters["temperature"] = 22;
        parameters["on"] = false;
    }  else if (deviceType == "увлажнитель") {
        deviceGroup = "отопление";
        parameters["humidity"] = 50;
        parameters["on"] = false;
    } else if (deviceType == "кофемашина"||deviceType == "стиральная машина"||deviceType == "робот-пылесос"||deviceType == "колонка") {
        deviceGroup = "бытовая техника";
        parameters["on"] = false;
    } else if (deviceType == "замок"||deviceType == "сигнализация") {
        deviceGroup = "безопасность";
        parameters["on"] = false;
    } else {
        qDebug() << "Unknown device type: " << deviceType;
    }
}
void SmartHouseSystemServer::processAddDeviceRequest(QTcpSocket *socket, const QJsonObject &request) {
    QString roomName = request["roomName"].toString();
    QString deviceType = request["deviceType"].toString();

    QString generatedDeviceName;
    QString deviceGroup;
    QJsonObject parameters;
    determineDeviceGroup(deviceType, deviceGroup, parameters);
    bool success = DatabaseManager::instance().addDevice(roomName, deviceType, generatedDeviceName, deviceGroup, parameters);

    QJsonObject response;
    response["action"] = "addDevice";
    response["success"] = success;
    response["message"] = success ? "Device added successfully." : "Failed to add device.";

    if (success) {
        response["deviceName"] = generatedDeviceName;

        QJsonObject flaskRequest;
        flaskRequest["deviceName"] = generatedDeviceName;
        flaskRequest["deviceGroup"] = deviceGroup;
        flaskRequest["roomName"] = roomName;
        sendCreateContainerRequest(generatedDeviceName, deviceGroup, roomName);
    }

    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}


void SmartHouseSystemServer::sendCreateContainerRequest(const QString &deviceName, const QString &deviceGroup, const QString &roomName) {
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    QUrl url("http://flask_manager:5000/create_image");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["device_name"] = deviceName;
    json["device_group"] = deviceGroup;
    json["room_name"] = roomName;


    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Device container created successfully:" << reply->readAll();
        } else {
            qDebug() << "Failed to create device container:" << reply->errorString();
        }
        reply->deleteLater();
    });
}
void SmartHouseSystemServer::toggleDevice(const QString &deviceName, const QString &roomName, bool state) {
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    QUrl url("http://flask_manager:5000/toggle_device");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["device_name"] = deviceName;
    json["room_name"] = roomName;
    json["action"] = state ? "start" : "stop";

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    qDebug() << "Sending request to Flask:" << data;
    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [reply, deviceName, roomName, state]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << (state ? "Device started:" : "Device stopped:") << reply->readAll();
        } else {
            qDebug() << "Failed to toggle device state:" << reply->errorString();
        }
        reply->deleteLater();
    });
}
void SmartHouseSystemServer::processToggleDeviceRequest(QTcpSocket *socket, const QJsonObject &request) {
    QString deviceName = request["deviceName"].toString();
    QString roomName = request["roomName"].toString();
    bool state = request["state"].toBool();

    QJsonObject response;
    response["action"] = "toggleDevice";
    response["deviceName"] = deviceName;
    response["roomName"] = roomName;
    // Вызов метода управления устройством
    toggleDevice(deviceName, roomName, state);

    response["success"] = true;

    response["message"] = state ? "Device started successfully." : "Device stopped successfully.";

    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}

void SmartHouseSystemServer::toggleScenario(const QString &scenarioName, bool state, QJsonArray devices) {
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    QUrl url("http://flask_manager:5000/toggle_scenario");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["scenario_name"] = scenarioName;
    json["action"] = state ? "activate" : "deactivate";
    json["devices"] = devices;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    qDebug() << "Sending request to Flask:" << data;
    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [reply, scenarioName, state, devices]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << (state ? "Scenario activated:" : "Scenario deactivated:") << reply->readAll();
        } else {
            qDebug() << "Failed to toggle scenario state:" << reply->errorString();
        }
        reply->deleteLater();
    });
}
void SmartHouseSystemServer::processToggleScenarioRequest(QTcpSocket *socket, const QJsonObject &request) {
    QString scenarioName = request["scenarioName"].toString();
    bool state = request["state"].toBool();
    QJsonArray devices = DatabaseManager::instance().getDevicesByScenario(scenarioName);

    QJsonObject response;
    response["action"] = "toggleScenario";
    response["scenarioName"] = scenarioName;
    response["devices"] = devices;

    toggleScenario(scenarioName, state, devices);

    response["success"] = true;
    response["message"] = state ? "Scenario activated successfully." : "Scenario deactivated successfully.";

    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}


void SmartHouseSystemServer::processLoadScenariosRequest(QTcpSocket *socket, const QJsonObject &request) {
    QJsonArray scenarioList = QJsonArray::fromStringList(DatabaseManager::instance().getAllScenarios());

    QJsonObject response;
    response["action"] = "loadScenarios";
    response["scenarios"] = scenarioList;
    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}

void SmartHouseSystemServer::processAddScenarioRequest(QTcpSocket *socket, const QJsonObject &request) {
    QString scenarioName = request["scenarioName"].toString();
    QJsonArray devicesArray = request["devices"].toArray();
    bool success = DatabaseManager::instance().addScenario(scenarioName, devicesArray);

    QJsonObject response;

    if (!success) {
        response["action"] = "addScenario";
        response["success"] = false;
        response["message"] = "Failed to create scenario.";
        socket->write(QJsonDocument(response).toJson());
        socket->flush();
        return;
    }
    response["action"] = "addScenario";
    response["success"] = true;
    response["message"] = "Scenario added successfully.";
    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}

void SmartHouseSystemServer::processLoadScenarioDevicesRequest(QTcpSocket *socket, const QJsonObject &request){
    QMap<QString, QStringList> devices = DatabaseManager::instance().getAllDevices();
    QJsonObject response;
    response["action"] = "loadScenarioDevices";

    QJsonArray devicesArray;
    for (auto it = devices.begin(); it != devices.end(); ++it) {
        QJsonObject deviceObject;
        deviceObject["type"] = it.key();
        deviceObject["rooms"] = QJsonArray::fromStringList(it.value());
        devicesArray.append(deviceObject);
    }
    response["devices"] = devicesArray;
    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}
void SmartHouseSystemServer::processLoadRoomSensors(QTcpSocket *socket, const QJsonObject &request){
    QMap<QString, QStringList> sensors = DatabaseManager::instance().getAllRoomSensors();

    QJsonObject response;
    response["action"] = "loadRoomSensors";

    QJsonArray sensorsArray;
    for (auto it = sensors.begin(); it != sensors.end(); ++it) {
        QJsonObject sensorObject;
        sensorObject["type"] = it.key();
        sensorObject["rooms"] = QJsonArray::fromStringList(it.value());
        sensorsArray.append(sensorObject);
    }
    response["sensors"] = sensorsArray;

    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}
void SmartHouseSystemServer::sendRequestToFlask(const QJsonObject &request, const QString &endpoint) {
    QUrl url("http://127.0.0.1:5000/" + endpoint);
    QNetworkRequest flaskRequest(url);

    flaskRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(flaskRequest, QJsonDocument(request).toJson());

    connect(reply, &QNetworkReply::finished, this, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "Flask response: " << responseData;
        } else {
            qDebug() << "Error communicating with Flask: " << reply->errorString();
        }
        reply->deleteLater();
    });
}
void SmartHouseSystemServer::processControlDeviceRequest(QTcpSocket *socket, const QJsonObject &request) {
    QString deviceName = request["deviceName"].toString();
    QString action = request["actionType"].toString(); // Например, "on" или "off"

    QJsonObject flaskRequest;
    flaskRequest["deviceName"] = deviceName;
    flaskRequest["action"] = action;

    sendRequestToFlask(flaskRequest, "control_device");

    QJsonObject response;
    response["success"] = true;
    response["message"] = "Device control request sent to Flask.";
    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}
void SmartHouseSystemServer::processDeviceGroupsRequest(QTcpSocket *socket, const QJsonObject &request) {
    // Получаем устройства из базы данных
    QMap<QString, QStringList> groupedDevices = DatabaseManager::instance().getDevicesGroupedByType();

    // Формируем JSON-объект для отправки
    QJsonObject jsonForFlask;
    QJsonArray groupsArray;

    for (auto it = groupedDevices.begin(); it != groupedDevices.end(); ++it) {
        QJsonObject groupObject;
        groupObject["group"] = it.key();
        groupObject["devices"] = QJsonArray::fromStringList(it.value());
        groupsArray.append(groupObject);
    }

    jsonForFlask["deviceGroups"] = groupsArray;

    // Отправляем запрос на Flask
    QJsonObject flaskResponse;
    sendRequestToFlask(jsonForFlask, "distribute_tasks");

    // Возвращаем результат клиенту
    QJsonObject response;
    response["action"] = "deviceGroupsComputation";
    response["success"] = flaskResponse["success"].toBool();
    response["message"] = flaskResponse["message"].toString();

    socket->write(QJsonDocument(response).toJson());
    socket->flush();
}


