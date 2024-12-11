import ray
from flask import Flask, request, jsonify

# Инициализация Ray
ray.init()

# Удалённые функции для управления устройствами
@ray.remote
def control_lighting(intensity):
    print(f"Adjusting lighting to intensity {intensity}")
    return f"Lighting set to {intensity}"
 
@ray.remote
def control_heating(temperature):
    print(f"Setting heating to {temperature} °C")
    return f"Heating set to {temperature} °C"

# Удалённые классы для датчиков.
@ray.remote
class TemperatureSensor:
    def __init__(self):
        self.current_temperature = 20.0  # Стартовое значение температуры

    def update_temperature(self, temperature):
        self.current_temperature = temperature
        return f"Temperature updated to {temperature} °C"

    def get_temperature(self):
        return self.current_temperature


@ray.remote
class MotionSensor:
    def __init__(self):
        self.motion_detected = False

    def detect_motion(self):
        self.motion_detected = True
        return "Motion detected!"

    def reset_motion(self):
        self.motion_detected = False
        return "Motion reset"

    def get_motion_status(self):
        return self.motion_detected


# Flask приложение
app = Flask(__name__)

# Хранилище для устройств и датчиков
devices = {}
sensors = {
    "temperature_sensor": TemperatureSensor.remote(),
    "motion_sensor": MotionSensor.remote(),
}


# Управление устройствами
@app.route('/control_device', methods=['POST'])
def control_device():
    try:
        # Получение данных из запроса
        data = request.get_json()

        # Проверка наличия необходимых полей
        if not all(key in data for key in ("device_type", "value")):
            return jsonify({"error": "Invalid data, 'device_type' and 'value' required"}), 400

        device_type = data['device_type']
        value = data['value']

        # Обработка команды
        if device_type == "lighting":
            result = ray.get(control_lighting.remote(value))
        elif device_type == "heating":
            result = ray.get(control_heating.remote(value))
        else:
            return jsonify({"error": "Unknown device type"}), 400

        # Возврат результата
        return jsonify({"result": result})

    except Exception as e:
        return jsonify({"error": str(e)}), 500


# Управление датчиками
@app.route('/sensor/update', methods=['POST'])
def update_sensor():
    try:
        # Получение данных из запроса
        data = request.get_json()

        # Проверка входных данных
        if not all(key in data for key in ("sensor_type", "value")):
            return jsonify({"error": "Invalid data, 'sensor_type' and 'value' required"}), 400

        sensor_type = data['sensor_type']
        value = data['value']

        # Обновление состояния датчика
        if sensor_type == "temperature":
            result = ray.get(sensors["temperature_sensor"].update_temperature.remote(value))
        elif sensor_type == "motion":
            if value:  # Если значение True, фиксируем движение
                result = ray.get(sensors["motion_sensor"].detect_motion.remote())
            else:  # Иначе сбрасываем движение
                result = ray.get(sensors["motion_sensor"].reset_motion.remote())
        else:
            return jsonify({"error": "Unknown sensor type"}), 400

        return jsonify({"result": result})
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route('/sensor/status', methods=['GET'])
def get_sensor_status():
    try:
        # Получение типа датчика из параметров запроса
        sensor_type = request.args.get('sensor_type')

        if sensor_type == "temperature":
            result = ray.get(sensors["temperature_sensor"].get_temperature.remote())
        elif sensor_type == "motion":
            result = ray.get(sensors["motion_sensor"].get_motion_status.remote())
        else:
            return jsonify({"error": "Unknown sensor type"}), 400

        return jsonify({"status": result})
    except Exception as e:
        return jsonify({"error": str(e)}), 500


# Автоматизация на основе датчиков
@app.route('/sensor/automation', methods=['POST'])
def automate_device():
    try:
        # Получение данных о текущем состоянии датчиков
        temperature = ray.get(sensors["temperature_sensor"].get_temperature.remote())
        motion = ray.get(sensors["motion_sensor"].get_motion_status.remote())

        # Простая автоматизация
        actions = []
        if temperature > 30:
            actions.append(ray.get(control_heating.remote(22)))  # Установить комфортную температуру
        if motion:
            actions.append(ray.get(control_lighting.remote(100)))  # Включить свет

        if not actions:
            actions = ["No action required"]

        return jsonify({"actions": actions})
    except Exception as e:
        return jsonify({"error": str(e)}), 500


# Запуск сервера
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)

