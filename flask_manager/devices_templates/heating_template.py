import threading
import random
from flask import Flask, jsonify, request

app = Flask(__name__)

state = "off"
temperature = 22

def simulate_temperature_change():
    global temperature
    while True:
        if state == "on":
            temperature += random.choice([-1, 0, 1])
            temperature = max(15, min(30, temperature))
        threading.Event().wait(10)

# Фоновой поток
threading.Thread(target=simulate_temperature_change, daemon=True).start()

@app.route('/status', methods=['GET'])
def status():
    return jsonify({
        "device_name": "{device_name}",
        "device_type": "{device_type}",
        "room_name": "{room_name}",
        "state": state,
        "temperature": temperature
    })

@app.route('/toggle', methods=['POST'])
def toggle():
    global state
    state = "on" if state == "off" else "off"
    return jsonify({"device_name": "{device_name}", "state": state})

@app.route('/temperature', methods=['POST'])
def set_temperature():
    global temperature
    temp = request.json.get('temperature')
    if temp is None or not (15 <= temp <= 30):
        return jsonify({"error": "Temperature out of range"}), 400
    temperature = temp
    return jsonify({"device_name": "{device_name}", "temperature": temperature})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)