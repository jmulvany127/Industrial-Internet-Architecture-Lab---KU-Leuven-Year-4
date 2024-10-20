## Overview
The repo contains two publishers and two subscribers:
* the control publisher and sensor subscriber should run on your local device.
* The contorl subscriber and sensor publisher should run on the pi.

To run both the pub and sub simultaneously on the pi use the command below:
* echo "subscriber_control.py publisher_sense.py" | xargs -n 1 -P 2 python

For your local device the pub and sub can just be run in two seperate cmd windows or for windws i use:
* start python subscriber_sense.py & start python publisher_control.

The alarm_server can be ran to receive notifications of charge dropping through http notifications 

The control_publisher is now publishin artificial data using json, I included my telegraf config file so that influx can receive this data 
Note the publisher_sense and subscriber control will have to be combined into one multithreaded programme, locking will have to be used to pass the sensor data between the publisher and listener threads 
Note alos that subscriber_sense is just for testing, telegraf is technically the subscriber to sense.

All data visualisation and alerting now finished, alot of small querying code and UI work is done to create the dashboard, this cannot really be included in the git code  
  
Here is an overview of all of the important information we chose:
<img width="976" alt="Screenshot 2024-04-27 at 16 05 33" src="https://github.com/Victor-Vansteenkiste/III-Lab-Sessions/assets/61208001/36de08dc-d564-41e5-86b7-838f07cb5ba0">

## Encryption keys
To ensure that the communication with the MQTT broker uses TLS/SSL encryption, update the telegraf config file as shown below.

* qos = 0
* tls_cert = "*path to correct directory*/III/III-Lab-Sessions/key/cert.pem"
* tls_key = "*path to correct directory*/III/III-Lab-Sessions/key/key.pem"
* insecure_skip_verify = false

These lines can be found under the [[inputs.mqtt_consumer]] section of the config file. 

![config-tele](https://github.com/Victor-Vansteenkiste/III-Lab-Sessions/assets/125087856/cde36ae1-b3e9-40c3-9554-0fdd831bb7d4)

![qos](https://github.com/Victor-Vansteenkiste/III-Lab-Sessions/assets/125087856/11058cb4-27a4-492f-a672-bbd32ca04733)


## Setup
Command for ssh: ssh -i ~/.ssh/id_rsa group13@raspberrypi13.local


