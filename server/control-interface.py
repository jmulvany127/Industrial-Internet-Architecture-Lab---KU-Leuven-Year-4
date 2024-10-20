import tkinter as tk
from tkinter import ttk
import threading
import paho.mqtt.client as mqtt

broker = 'test.mosquitto.org'
port = 8884
topic = "iii24/13/control"

# Create a MQTT client
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.tls_set(ca_certs='../certs/ca/Mosquitto.org certificate.crt',
               certfile='../certs/client/control-server/control-client.crt', keyfile='../certs/client/control-server/client.key')

client.connect(broker, port)


def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Connected to MQTT Broker!")
    else:
        print(f"Connect failed with result code {rc}")


def on_disconnect(client, userdata, flags, rc, properties=None):
    if rc != 0:
        print("Unexpected disconnection from MQTT Broker. Trying to reconnect...")
        client.reconnect()


client.on_disconnect = on_disconnect
client.on_connect = on_connect


class BuggyControlApp:

    def __init__(self, master):
        self.master = master
        master.title("Remote Control Buggy")
        # master.configure(bg="white")
        master.geometry("500x250")  # Set a fixed window size

        self.create_buttons(master)

    def create_buttons(self, master):
        # Create a style for the buttons
        # style = ttk.Style()
        # style.configure("TButton", padding=10, relief="flat",
        #               background="#0000FF", foreground="white")

        # Create buttons directly using ttk.Button

        style = ttk.Style()

        style.configure('TButton', font=('calibri', 15, 'bold'),
                        borderwidth='4',
                        width=10,  # Set width in text units
                        height=2)

        # Changes will be reflected
        # by the movement of mouse.
        style.map('TButton', foreground=[('active', '!disabled', 'blue')],
                  background=[('active', 'black')])
        buttons = [
            ("North", self.fb_straight_forw, 0, 1),
            ("South", self.fb_straight_back, 2, 1),
            ("West", self.fb_side_left, 1, 0),
            ("East", self.fb_side_right, 1, 2),
            ("Rotate CW", self.fb_rotate_cw, 5, 0),
            ("Rotate CCW", self.fb_rotate_ccw, 5, 2),
            ("North-East", self.fb_side_d45, 0, 2),
            ("South-East", self.fb_side_d135, 2, 2),
            ("South-West", self.fb_side_d225, 2, 0),
            ("North-West", self.fb_side_d315, 0, 0)
        ]

        for text, command, row, column in buttons:
            button = ttk.Button(master, text=text,
                                command=command)
            button.grid(row=row, column=column, padx=10, pady=10)

    def fb_straight_forw(self):
        client.publish('iii24/13/control', "<F>".encode('utf-8'))
        print("Robot going forward")

    def fb_straight_back(self):
        client.publish('iii24/13/control', "<B>".encode('utf-8'))
        print("Robot going backward")

    def fb_side_right(self):
        client.publish('iii24/13/control', "<R>".encode('utf-8'))
        print("Robot going right")

    def fb_side_left(self):
        client.publish('iii24/13/control', "<L>".encode('utf-8'))
        print("Robot going left")

    def fb_side_d45(self):
        client.publish('iii24/13/control', "<1>".encode('utf-8'))
        print("Robot going 45°")

    def fb_side_d135(self):
        client.publish('iii24/13/control', "<2>".encode('utf-8'))
        print("Robot going 135°")

    def fb_side_d225(self):
        client.publish('iii24/13/control', "<3>".encode('utf-8'))
        print("Robot going 225°")

    def fb_side_d315(self):
        client.publish('iii24/13/control', "<4>".encode('utf-8'))
        print("Robot going 315°")

    def fb_rotate_cw(self):
        client.publish('iii24/13/control', "<C>".encode('utf-8'))
        print("Robot rotating clockwise")

    def fb_rotate_ccw(self):
        client.publish('iii24/13/control', "<D>".encode('utf-8'))
        print("Robot rotating counterclockwise")

# Main function to run the GUI


def main():
    root = tk.Tk()
    app = BuggyControlApp(root)
    # Create a thread for MQTT client loop
    mqtt_thread = threading.Thread(target=client.loop_forever)
    mqtt_thread.start()

    # Start the GUI main loop in the main thread
    interface_thread = threading.Thread(target=root.mainloop())
    interface_thread.start()


if __name__ == "__main__":

    main()
