import struct
import paho.mqtt.client as mqtt
import serial
import json
import threading
import time


SERIAL_PORT = '/dev/ttyS0'
BAUD_RATE = 115200

# MQTT settings
MQTT_BROKER = 'test.mosquitto.org'
CONTROL_TOPIC = 'iii24/13/control'
SENSE_TOPIC = 'iii24/13/sense'


def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Connected to MQTT Broker!")
        print(f"Subscribed to {CONTROL_TOPIC} topic!")
    else:
        print(f"Connect failed with result code {rc}")


# Create an MQTT client instance
mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

#certs removed for safety reasons
mqtt_client.tls_set(ca_certs='../certs/ca/Mosquitto.org certificate.crt',
                    certfile='../certs/client/raspberrypi/raspberrypi-client.crt', keyfile='../certs/client/raspberrypi/client.key')

mqtt_client.on_connect = on_connect
mqtt_client.connect(MQTT_BROKER, 8884)
ser = serial.Serial(SERIAL_PORT, BAUD_RATE)


# this function is called when a message is received from the MQTT broker, it will then pass the message to the UART
def on_mqtt_message(client, userdata, message):

    # received_data = fernet.decrypt(message.payload.decode("utf-8"))
    encoded_data = message.payload
    received_string = message.payload.decode("utf-8")
    print("Received control command: " + received_string)
    write_UART(encoded_data)
    return 0


def write_UART(encoded_data):
    print("Sending control to UART")
    ser.write(encoded_data)
    time.sleep(1)


def mqtt_subscribe():
    mqtt_client.on_message = on_mqtt_message
    mqtt_client.subscribe(CONTROL_TOPIC)
    print(f"Subscribed to {CONTROL_TOPIC} topic!")
    mqtt_client.loop_forever(10)


MESSAGE_LENGTH = 14


def read_UART():
    try:
        received_data = ser.read()
        time.sleep(0.2)
        data_left = ser.inWaiting()
        received_data += ser.read(data_left)
        print(len(received_data))
        if (len(received_data) != MESSAGE_LENGTH):
            ser.flushInput()
            return None
        decoded_received_data = received_data.decode('utf-8')
        print("\033[94m" + decoded_received_data + "\033[0m")
        return decoded_received_data
    except Exception as e:
        print(f"Exception occurred: {e}")


def parse_decoded_uart_data(decoded_data):
    try:
        sense_values = []
        sense_keys = ["charge", "Front Left RPM",
                      "Front Right RPM", "Back Left RPM", "Back Right RPM"]
        split_string = decoded_data.split(',')
        print(split_string)
        for i in range(len(split_string)):
            reading = split_string[i]
            byte_string = reading.encode('latin1')
            sense_values.append(struct.unpack('<H', byte_string)[0])

        sense_data = dict(zip(sense_keys, sense_values))
        return sense_data
    except Exception as e:
        print(f"Exception occurred while parsing decoded UART data: {e}")


def get_alarm_state(charge):
    global alarm_state
    global previous_charge
    global setup

    try:
        if setup == 0:
            previous_charge = charge
            setup = 1

        if charge is not None:
            if previous_charge - charge >= 1:
                alarm_state = 1
                previous_charge = charge
            else:
                alarm_state = 0

        return alarm_state

    except Exception as e:
        print(f"Exception occurred in get_alarm_state: {e}")


def create_sense_message(decode_data):
    try:
        sense_data = parse_decoded_uart_data(decode_data)
        sense_data['Alarm'] = get_alarm_state(sense_data['charge'])
        return json.dumps(sense_data)
    except Exception as e:
        print(f"Exception occurred in create_sense_message: {e}")


def mqtt_publish():
    while True:
        decoded_received_data = read_UART()
        if (decoded_received_data is None):
            continue
        try:
            sensing_data_msg = create_sense_message(decoded_received_data)
            print(sensing_data_msg)
        except Exception as e:
            print(
                f"Exception occurred while creating sensing data message: {e}")

            continue
        # publish with MQTT
        print("Sending sensor readings to database")
        try:
            mqtt_client.publish(SENSE_TOPIC, sensing_data_msg.encode('utf-8'))
        except Exception as e:
            print(f"Exception occurred while publishing MQTT message: {e}")
            continue


if __name__ == "__main__":
    alarm_state = 0
    alarm_counter = 0
    previous_charge = 0
    setup = 0

    # Create threads for MQTT publish and subscribe functions
    publish_thread = threading.Thread(target=mqtt_publish)
    subscribe_thread = threading.Thread(target=mqtt_subscribe)

    # Start both threads
    while True:
        try:
            publish_thread.start()
            subscribe_thread.start()
            publish_thread.join()
            subscribe_thread.join()
        except Exception as e:
            print(f"Exception occurred: {e}")
            publish_thread = threading.Thread(target=mqtt_publish)
            subscribe_thread = threading.Thread(target=mqtt_subscribe)

    # Wait for both threads to finish
    # publish_thread.join()
    # subscribe_thread.join()
