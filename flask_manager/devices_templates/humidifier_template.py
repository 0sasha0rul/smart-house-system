import threading
import random
from flask import Flask, jsonify, request

app = Flask(__name__)

state = "off"
humidity = 50

def simulate_humidity_change():
    """Функция для имитации изменения уровня влажности."""
    global humidity
    while True:
        if state == "on":
            humidity += random.choice([-2, -1, 0, 1, 2])
            humidity = max(30, min(70, humidity))
        threading.Event().wait(10)

# Запуск фонового потока
threading.Thread(target=simulate_humidity_change, daemon=True).start()

@app.route('/status', methods=['GET'])
def status():
    """Получение текущего состояния устройства."""
    return jsonify({
        "device_name": "{device_name}",
        "device_type": "{device_type}",
        "room_name": "{room_name}",
        "state": state,
        "humidity": humidity
    })

@app.route('/toggle', methods=['POST'])
def toggle():
    """Переключение состояния устройства."""
    global state
    state = "on" if state == "off" else "off"
    return jsonify({"device_name": "{device_name}", "state": state})

@app.route('/humidity', methods=['POST'])
def set_humidity():
    """Установка уровня влажности вручную."""
    global humidity
    hum = request.json.get('humidity')
    if hum is None or not (30 <= hum <= 70):
        return jsonify({"error": "Humidity out of range"}), 400
    humidity = hum
    return jsonify({"device_name": "{device_name}", "humidity": humidity})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
