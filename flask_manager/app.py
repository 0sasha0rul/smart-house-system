from flask import Flask, request, jsonify
import docker
import re
import unidecode
import os
import shutil
import logging


logging.basicConfig(level=logging.DEBUG)
app = Flask(__name__)
client = docker.from_env()

def sanitize_name(name):
    sanitized = unidecode.unidecode(name)
    sanitized = re.sub(r'[^a-zA-Z0-9_-]', '_', sanitized)
    return sanitized.lower()

def select_template(device_type, device_name):
    templates = {
        "on_off_devices": "devices_templates/lighting_template.py",
        "on_off_temp_devices": "devices_templates/heating_template.py",
        "humidifier": "devices_templates/humidifier_template.py"
    }
    if device_type in ['освещение', 'безопасность', 'бытовая техника']:
        return templates['on_off_devices']
    elif device_type == "отопление" and device_name.startswith("увлажнитель"):
        return templates['humidifier']
    elif device_type == "отопление":
        return templates['on_off_temp_devices']
    return 'not found'


def create_or_start_room_sensor(room_name, sensor_type):
    sanitized_room_name = sanitize_name(room_name)
    container_name = f"sensor_{sanitized_room_name}_{sensor_type}"

    # Проверка существования контейнера
    try:
        container = client.containers.get(container_name)
        if container.status != "running":
            container.start()
        logging.info(f"Container {container_name} already exists and is running.")
        return True
    except docker.errors.NotFound:
        logging.info(f"Container {container_name} not found. Creating...")
    except Exception as e:
        logging.error(f"Error checking container {container_name}: {str(e)}")
        return False

    # Создание контейнера
    build_dir = os.path.join('/tmp', container_name)
    os.makedirs(build_dir, exist_ok=True)

    try:
        template_path = f"sensors_templates/{sensor_type}_sensor_template.py"
        if not os.path.exists(template_path):
            logging.error(f"Template for sensor type '{sensor_type}' not found")
            return False

        shutil.copy(template_path, os.path.join(build_dir, 'sensor_simulator.py'))
        dockerfile_content = f"""
        FROM python:3.9-slim
        COPY . /app
        WORKDIR /app
        RUN pip install flask
        CMD ["python", "sensor_simulator.py"]
        """
        with open(os.path.join(build_dir, 'Dockerfile'), 'w') as dockerfile:
            dockerfile.write(dockerfile_content)

        image, _ = client.images.build(path=build_dir, tag=f"{container_name}:latest")
        network_name = "smarthousesystem_app-network"
        container = client.containers.create(
            image=container_name,
            name=container_name,
            detach=True,
            environment={
                "ROOM_NAME": room_name,
                "SENSOR_TYPE": sensor_type
            },
            network=network_name
        )
        container.start()
        return True
    except Exception as e:
        logging.error(f"Failed to create container {container_name}: {str(e)}")
        return False
    finally:
        shutil.rmtree(build_dir)


@app.route('/create_image', methods=['POST'])
def create_image():
    logging.info("Received request to create an image")
    data = request.json
    device_name = data.get('device_name')
    device_type = data.get('device_group')
    room_name = data.get('room_name')

    if not device_name or not room_name:
        logging.error("Missing required parameters: device_name or room_name")
        return jsonify({"error": "Device name and room name are required"}), 400

    sanitized_device_name = sanitize_name(device_name)
    sanitized_room_name = sanitize_name(room_name)
    container_name = f"device_{sanitized_room_name}_{sanitized_device_name}"

    build_dir = os.path.join('/tmp', container_name)
    logging.debug(f"Build directory: {build_dir}")

    try:
        if device_type == "отопление":
            create_or_start_room_sensor(room_name, "temperature")
            if device_name.startswith("увлажнитель"):
                create_or_start_room_sensor(room_name, "humidity")
        os.makedirs(build_dir, exist_ok=True)

        template_path = select_template(device_type, device_name)
        if template_path == 'not found' or not os.path.exists(template_path):
            logging.error(f"Template for device type '{device_name}{device_type}' not found")
            return jsonify({"error": f"Template for device type '{device_name}{device_type}' not found"}), 400

        shutil.copy(template_path, os.path.join(build_dir, 'device_simulator.py'))
        logging.debug(f"Template {template_path} copied to {build_dir}")

        dockerfile_content = f"""
        FROM python:3.9-slim
        COPY . /app
        WORKDIR /app
        RUN pip install flask
        CMD ["python", "device_simulator.py"]
        """
        dockerfile_path = os.path.join(build_dir, 'Dockerfile')
        with open(dockerfile_path, 'w') as dockerfile:
            dockerfile.write(dockerfile_content)
        logging.debug(f"Dockerfile created at {dockerfile_path}")

        logging.info(f"Building Docker image: {container_name}")
        image, build_logs = client.images.build(path=build_dir, tag=f"{container_name}:latest")
        for log in build_logs:
            logging.debug(log)

        network_name = "smarthousesystem_app-network"
        networks = [n.name for n in client.networks.list()]
        logging.debug(f"Available networks: {networks}")
        if network_name not in networks:
            logging.error(f"Network {network_name} does not exist")
            raise ValueError(f"Network {network_name} does not exist")

        logging.info(f"Creating container: {container_name}")
        container = client.containers.create(
            image=container_name,
            name=container_name,
            detach=True,
            environment={
                "DEVICE_NAME": device_name,
                "DEVICE_TYPE": device_type,
                "ROOM": room_name
            },
            network=network_name
        )
        logging.debug(f"Container created: {container}")

        return jsonify({"message": f"Image and container for {container_name} created successfully"}), 201
    except Exception as e:
        logging.exception("An error occurred while creating the image and container")
        return jsonify({"error": str(e)}), 500
    finally:
        if os.path.exists(build_dir):
            shutil.rmtree(build_dir)
            logging.debug(f"Temporary build directory {build_dir} removed")


@app.route('/toggle_device', methods=['POST'])
def toggle_device():
    data = request.json
    device_name = data.get('device_name')
    room = data.get('room_name')
    action = data.get('action')  # "start" или "stop"

    if not device_name or not room or action not in ["start", "stop"]:
        return jsonify({'error': 'Invalid parameters'}), 400

    sanitized_name = f"device_{sanitize_name(room)}_{sanitize_name(device_name)}"

    try:
        app.logger.debug(f"Looking for container: {sanitized_name}")
        container = client.containers.get(sanitized_name)

        if action == "start":
            if container.status != "running":
                container.start()
                return jsonify({'message': f'Device {device_name} started successfully' , 'success':'true'}), 200
            else:
                return jsonify({'error': f'Device {device_name} is already running', 'success':'false'}), 400
        elif action == "stop":
            if container.status == "running":
                container.stop()
                return jsonify({'message': f'Device {device_name} stopped successfully', 'success':'true'}), 200
            else:
                return jsonify({'error': f'Device {device_name} is not running', 'success':'false'}), 400
    except docker.errors.NotFound:
        app.logger.error(f"Container not found: {sanitized_name}")
        return jsonify({'error': f'Container for {device_name} not found'}), 404
    except Exception as e:
        app.logger.error(f"Unexpected error: {str(e)}")
        return jsonify({'error': str(e)}), 500


@app.route('/device_status', methods=['GET'])
def device_status():
    device_name = request.args.get('device_name')
    room_name = request.args.get('room_name')  # Учитываем комнату

    if not device_name or not room_name:
        return jsonify({'error': 'Device name and room name are required'}), 400

    sanitized_device_name = sanitize_name(device_name)
    sanitized_room_name = sanitize_name(room_name)

    container_name = f"device_{sanitized_room_name}_{sanitized_device_name}"
    try:
        container = client.containers.get(container_name)
        return jsonify({
            "device_name": device_name,
            "room_name": room_name,
            "status": container.status
        }), 200
    except docker.errors.NotFound:
        return jsonify({'error': f'Container for {device_name} in room {room_name} not found'}), 404
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/toggle_scenario', methods=['POST'])
def toggle_scenario():
    data = request.json
    scenario_name = data.get("scenario_name")
    devices = data.get("devices")
    action = data.get("action")

    if not scenario_name or not devices or action not in ["activate", "deactivate"]:
        return jsonify({"success": False, "message": "Invalid parameters"}), 400

    results = []
    for device in devices:
        try:
            room_name, device_name = device.split(": ")
            sanitized_name = f"device_{sanitize_name(room_name)}_{sanitize_name(device_name)}"

            container = client.containers.get(sanitized_name)

            if action == "activate":
                if container.status != "running":
                    container.start()
                    results.append({
                        "device": device,
                        "success": True,
                        "message": "Device started successfully"
                    })
                else:
                    results.append({
                        "device": device,
                        "success": False,
                        "message": "Device is already running"
                    })
            elif action == "deactivate":
                if container.status == "running":
                    container.stop()
                    results.append({
                        "device": device,
                        "success": True,
                        "message": "Device stopped successfully"
                    })
                else:
                    results.append({
                        "device": device,
                        "success": False,
                        "message": "Device is not running"
                    })

        except docker.errors.NotFound:
            results.append({
                "device": device,
                "success": False,
                "message": "Container not found"
            })
        except Exception as e:
            results.append({
                "device": device,
                "success": False,
                "message": f"Error: {str(e)}"
            })

    all_success = all(r["success"] for r in results)
    return jsonify({
        "success": all_success,
        "message": f"Scenario {action}d {'successfully' if all_success else 'with some errors'}",
        "details": results
    })


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
