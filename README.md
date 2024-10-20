## Industrial Internet Architecture Lab - KU Leuven Year 4
Project involved the control of motors and monitoring of sensors of a small RC buggy. 
Project team was split into two teams, one handling the embedded device and the transfer of data over blue tooth while my team controlled the higher level buggy control and sensor monitoring.

A python programme sends publishes encrypted controls for the buggy to an MQTT server, a seperate application running on the onboard arduino nano is subscribed to the controls topic and recieves these commands via a push notifcation.

The arduino then transfers these commands through a bluetooth connection to the nrf52840 devices which controls the wheel motors. 

The RPM of each wheel and the battery levels are monitored and the results passed back through the system and published again on an MQTT server. 

The TICK stack is used to record, analyse and visualise these sensor results. A telegraf agent subscribes to the sensor MQTT server and gathers the data before it is record in an InfluxDB timeseries database.
Here all aspects of the RC buggies movemnt can be monitored using inbuilt graphing features and alarms for when battery levels reach too low.




