#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QProcessEnvironment>
#include <QJsonArray>

DatabaseManager::DatabaseManager() {

    QString dbHost = QString::fromUtf8(qgetenv("DB_HOST"));
    QString dbPort = QString::fromUtf8(qgetenv("DB_PORT"));
    QString dbUser = QString::fromUtf8(qgetenv("DB_USER"));
    QString dbPassword = QString::fromUtf8(qgetenv("DB_PASSWORD"));
    QString dbName = QString::fromUtf8(qgetenv("DB_NAME"));

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName(dbHost);
    db.setPort(dbPort.toInt());
    db.setDatabaseName(dbName);
    db.setUserName(dbUser);
    db.setPassword(dbPassword);

    if (!db.open()) {
        qDebug() << "Database error occurred:" << db.lastError().text();
    } else {
        qDebug() << "Database opened successfully:" << db.lastError().text();
    }
}

DatabaseManager::~DatabaseManager() {
    if (db.isOpen()) {
        db.close();
    }
}

bool DatabaseManager::openConnection() {
    if (!db.open()) {
        qDebug() << "Database connection error:" << db.lastError().text();
        return false;
    }
    qDebug() << "Database connection successful.";
    return true;
}

QSqlDatabase DatabaseManager::getDatabase() {
    return db;
}

bool DatabaseManager::registerUser(const QString &username, const QString &password, const QString &role) {
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password, role) VALUES (:username, :password, :role)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.bindValue(":role", role);

    if (!query.exec()) {
        qDebug() << "Failed to register user: " << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::authenticateUser(const QString &username, const QString &password) {
    QSqlQuery query;
    query.prepare("SELECT password FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        QString storedPassword = query.value(0).toString();
        return (storedPassword == password);
    }
    return false;
}

QString DatabaseManager::getUserRole(const QString &username) {
    QSqlQuery query;
    query.prepare("SELECT role FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

bool DatabaseManager::userExists(const QString &username) {
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

bool DatabaseManager::adminExists() {
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM users WHERE role = 'admin'");

    if (query.exec() && query.next()) {
        int adminCount = query.value(0).toInt();
        return adminCount > 0;
    }
    return false;
}

bool DatabaseManager::addRoom(const QString &roomName) {
    QSqlQuery query;
    query.prepare("INSERT INTO rooms (name) VALUES (:name)");
    query.bindValue(":name", roomName);

    if (!query.exec()) {
        qDebug() << "Failed to add room: " << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::addDevice(const QString &roomName, const QString &deviceType, QString &generatedDeviceName, QString &deviceGroup, QJsonObject &parameters) {
    QSqlQuery query;

    query.prepare("SELECT id FROM rooms WHERE name = :name");
    query.bindValue(":name", roomName);
    if (!query.exec() || !query.next()) {
        qDebug() << "Room not found: " << roomName;
        return false;
    }
    int roomId = query.value(0).toInt();

    query.prepare("SELECT id FROM device_types WHERE type = :device_type");
    query.bindValue(":device_type", deviceType);
    if (!query.exec()) {
        qDebug() << "Failed to check device type: " << query.lastError().text();
        return false;
    }

    int deviceTypeId;
    if (!query.next()) {
        query.prepare("INSERT INTO device_types (type, parameters) VALUES (:type, :parameters)");
        query.bindValue(":type", deviceType);
        query.bindValue(":parameters", QString(QJsonDocument(parameters).toJson(QJsonDocument::Compact)));
        if (!query.exec()) {
            qDebug() << "Failed to insert new device type: " << query.lastError().text();
            return false;
        }
        deviceTypeId = query.lastInsertId().toInt();
    } else {
        deviceTypeId = query.value(0).toInt();
    }

    query.prepare("SELECT COUNT(*) FROM devices WHERE room_id = :room_id AND device_type_id = :device_type_id");
    query.bindValue(":room_id", roomId);
    query.bindValue(":device_type_id", deviceTypeId);
    if (!query.exec() || !query.next()) {
        qDebug() << "Failed to count devices: " << query.lastError().text();
        return false;
    }
    int deviceCount = query.value(0).toInt();
    generatedDeviceName = deviceCount == 0 ? deviceType : QString("%1 %2").arg(deviceType).arg(deviceCount + 1);

    query.prepare("INSERT INTO devices (room_id, device_type_id, device_group, name, status) "
                  "VALUES (:room_id, :device_type_id, :device_group, :name, 'off')");
    query.bindValue(":room_id", roomId);
    query.bindValue(":device_type_id", deviceTypeId);
    query.bindValue(":device_group", deviceGroup);
    query.bindValue(":name", generatedDeviceName);

    if (!query.exec()) {
        qDebug() << "Failed to add device: " << query.lastError().text();
        return false;
    }

    // Добавить датчик, связанный с устройством
    /*if (deviceGroup == "отопление") {
        QString sensorType = (deviceType == "увлажнитель") ? "влажность" : "температура";

        query.prepare("INSERT INTO sensors (room_id, type, status, last_signal) "
                      "VALUES (:room_id, :type, 'active', CURRENT_TIMESTAMP)");
        query.bindValue(":room_id", roomId);
        query.bindValue(":type", sensorType);

        if (!query.exec()) {
            qDebug() << "Failed to add sensor for device: " << query.lastError().text();
            return false;
        }
    }*/

    QStringList roomSensorTypes = {"влажность", "температура"};
    for (const QString &type : roomSensorTypes) {
        query.prepare("SELECT COUNT(*) FROM sensors WHERE room_id = :room_id AND type = :type");
        query.bindValue(":room_id", roomId);
        query.bindValue(":type", type);

        if (!query.exec()) {
            qDebug() << "Failed to check room sensors: " << query.lastError().text();
            return false;
        }

        if (query.next() && query.value(0).toInt() == 0) {
            query.prepare("INSERT INTO sensors (room_id, type, status, last_signal) "
                          "VALUES (:room_id, :type, 'active', CURRENT_TIMESTAMP)");
            query.bindValue(":room_id", roomId);
            query.bindValue(":type", type);

            if (!query.exec()) {
                qDebug() << "Failed to add room sensor: " << query.lastError().text();
                return false;
            }
        }
    }

    return true;
}



QMap<QString, QStringList> DatabaseManager::getAllDevices() {
    QMap<QString, QStringList> deviceNameMap;

    QString sqlQuery = R"(
        SELECT
            devices.name AS device_name,
            rooms.name AS room_name
        FROM devices
        LEFT JOIN rooms ON devices.room_id = rooms.id
    )";

    QSqlQuery query;
    if (!query.exec(sqlQuery)) {
        qDebug() << "Failed to execute query: " << sqlQuery;
        qDebug() << "Error: " << query.lastError().text();
        return deviceNameMap;
    }

    while (query.next()) {
        QString deviceName = query.value(0).toString();
        QString roomName = query.value(1).toString();

        if (!deviceNameMap.contains(deviceName)) {
            deviceNameMap[deviceName] = QStringList();
        }

        if (!roomName.isEmpty()) {
            deviceNameMap[deviceName].append(roomName);
        }
    }

    return deviceNameMap;
}

QStringList DatabaseManager::getAllRooms() {
    QStringList rooms;

    QSqlQuery query;
    QString sqlQuery = "SELECT name FROM rooms";

    if (!query.exec(sqlQuery)) {
        qDebug() << "Failed to execute query: " << sqlQuery;
        qDebug() << "Error: " << query.lastError().text();
        return rooms;
    }

    while (query.next()) {
        rooms << query.value(0).toString();
    }

    return rooms;
}

QStringList DatabaseManager::getDevicesForRoom(const QString &roomName) {
    QStringList devices;
    QSqlQuery query;

    query.prepare("SELECT d.name FROM devices d JOIN rooms r ON d.room_id = r.id WHERE r.name = :roomName");
    query.bindValue(":roomName", roomName);

    if (query.exec()) {
        while (query.next()) {
            devices << query.value(0).toString();
        }
    } else {
        qDebug() << "Failed to retrieve devices for room" << roomName << ":" << query.lastError().text();
    }

    return devices;
}

QStringList DatabaseManager::getAllScenarios() {
    QStringList scenarios;

    QSqlQuery query;
    QString sqlQuery = "SELECT name FROM scenarios";

    if (!query.exec(sqlQuery)) {
        qDebug() << "Failed to execute query: " << sqlQuery;
        qDebug() << "Error: " << query.lastError().text();
        return scenarios;
    }

    while (query.next()) {
        scenarios << query.value(0).toString();
    }

    return scenarios;
}
bool DatabaseManager::addScenario(const QString &name, const QJsonArray &devices) {
    QSqlQuery query(db);
    QString devicesJson = QString::fromUtf8(QJsonDocument(devices).toJson(QJsonDocument::Compact));
    query.prepare("INSERT INTO scenarios (name, devices) VALUES (:name, :devices)");
    query.bindValue(":name", name);
    query.bindValue(":devices", devicesJson);

    if (!query.exec()) {
        qDebug() << "Failed to insert scenario:" << query.lastError();
        return false;
    }
    return true;
}

QJsonArray DatabaseManager::getDevicesByScenario(const QString &scenarioName) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);
    query.prepare("SELECT devices FROM scenarios WHERE name = :scenarioName");
    query.bindValue(":scenarioName", scenarioName);

    if (!query.exec()) {
        qDebug() << "Failed to fetch devices for scenario:" << query.lastError();
        return QJsonArray();
    }

    if (query.next()) {
        QString devicesJson = query.value(0).toString();
        QJsonDocument doc = QJsonDocument::fromJson(devicesJson.toUtf8());
        if (!doc.isNull() && doc.isArray()) {
            return doc.array();
        } else {
            qDebug() << "Failed to parse devices JSON:" << devicesJson;
        }
    }

    return QJsonArray();
}


QMap<QString, QStringList> DatabaseManager::getDevicesGroupedByType() {
    QMap<QString, QStringList> groupedDevices;
    QSqlQuery query("SELECT device_group, name FROM devices");

    while (query.next()) {
        QString group = query.value(0).toString();
        QString deviceName = query.value(1).toString();

        if (!groupedDevices.contains(group)) {
            groupedDevices[group] = QStringList();
        }
        groupedDevices[group].append(deviceName);
    }

    return groupedDevices;
}
QMap<QString, QStringList> DatabaseManager::getAllRoomSensors() {
    QSqlQuery query;
    QMap<QString, QStringList> sensors;

    if (!query.prepare("SELECT sensors.type AS sensor_type, rooms.name AS room_name "
                       "FROM sensors "
                       "JOIN rooms ON sensors.room_id = rooms.id "
                       )) {
        qDebug() << "Failed to prepare query for room sensors:" << query.lastError().text();
        return sensors;
    }

    if (!query.exec()) {
        qDebug() << "Failed to execute query for room sensors:" << query.lastError().text();
        return sensors;
    }

    while (query.next()) {
        QString sensorType = query.value("sensor_type").toString();
        QString roomName = query.value("room_name").toString();

        if (!sensors.contains(sensorType)) {
            sensors[sensorType] = QStringList();
        }
        if (!sensors[sensorType].contains(roomName)) {
            sensors[sensorType].append(roomName);
        }
    }

    return sensors;
}






