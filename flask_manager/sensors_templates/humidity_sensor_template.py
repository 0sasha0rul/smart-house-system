from flask import Flask, jsonify
import random
import time

app = Flask(__name__)

def generate_humidity():
    return round(random.uniform(30.0, 70.0), 1)

@app.route('/sensor_data', methods=['GET'])
def get_humidity():
    humidity = generate_humidity()
    timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
    return jsonify({
        "sensor_type": "humidity",
        "value": humidity,
        "unit": "%",
        "timestamp": timestamp
    }), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)