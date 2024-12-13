from flask import Flask, jsonify
import random
import time

app = Flask(__name__)

def generate_temperature():
    return round(random.uniform(18.0, 30.0), 1)

@app.route('/sensor_data', methods=['GET'])
def get_temperature():
    temperature = generate_temperature()
    timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
    return jsonify({
        "sensor_type": "temperature",
        "value": temperature,
        "unit": "Celsius",
        "timestamp": timestamp
    }), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
