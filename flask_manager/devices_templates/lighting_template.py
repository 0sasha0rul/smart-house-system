from flask import Flask, jsonify, request

app = Flask(__name__)

state = "off"

@app.route('/status', methods=['GET'])
def status():
    return jsonify({
        "device_name": "{device_name}",
        "device_type": "{device_type}",
        "room_name": "{room_name}",
        "state": state
    })

@app.route('/toggle', methods=['POST'])
def toggle():
    global state
    state = "on" if state == "off" else "off"
    return jsonify({"device_name": "{device_name}", "state": state})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
