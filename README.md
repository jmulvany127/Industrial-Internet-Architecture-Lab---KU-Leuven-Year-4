## Industrial Internet Architecture Lab - KU Leuven Year 4
Project involved the control of motors and monitoring of sensors of a small RC buggy. 
Project team was split into two groups, one handling the embedded NRF52840 and the transfer of data to the NRF52840 over blue-tooth while my team controlled the higher level buggy control and sensor monitoring programmes ran on both the onboard raspberry PI and a laptop.

A python programme generates a UI within which users can cntrol the buggy movement, these controls are then encrypted and published to an MQTT server, a seperate application running on the onboard rasperry PI is subscribed to the controls topic and recieves these commands via a push notifcation.

The raspberry pi programme then passes these commands through the local bluetooth gateway which process them before sending to the nrf52840 device which control the wheel motors. 

The RPM of each wheel and the battery levels are monitored and the results passed back through the system and published again on an MQTT server. 

The TICK stack is used to record, analyse and visualise these sensor results. A telegraf agent subscribes to the sensor MQTT server and gathers the data before it is record in an InfluxDB timeseries database.

Here all aspects of the RC buggies movemnt can be monitored using inbuilt graphing features and alarms for when battery levels reach too low.

![image](https://github.com/user-attachments/assets/ec76a1b7-58c3-45c7-bf69-199aa3f933ac)





