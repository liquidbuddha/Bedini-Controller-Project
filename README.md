Bedini-Controller-Project
=========================

Arduino controller for Bedini 3-pole Radiant Energy Charger kit
by liquidbuddha

Distributed under GNU GENERAL PUBLIC LICENSE v2


The goal of this project was to use an Arduino to control a pulse-motor, specifically a Bedini 3-polo monopole kit.
As the project evolved, multiple arduinos were required, with the following configuration:
  - BAT_Master - this is the UI and master of the system.
  - BAT_Slave - the Slave which takes inputs from the Master and actually pulses the coils.
  - BATTManager - or Battery Manager, was used to monitor and swap the Source and Charge battery banks. 
	The BATTManager also took on an ethernet shield, for control via the local web.
  - BAT_CapCharger - further evolution included a Capacitor charging bank, which then dumped into the charge battery bank
  - BAT_Voltage_Monitor - in a stripped down version, just to monitor voltages of various batteries and power sources
  
The Fritzig files are for visual reference only, and not for use to determine curcuit elements (ie resistor values, etc).

In summary, the classic Bedini circuit needs to be modified to be driven by a Opto-Isolator, which isolates the higher
current pulse side of the circuit, from the sensitive Arduino hardware. The drive-coil is connected to an analog input
of the Arduino, and the pulses flow from the digital outputs, into the opto-isolators, and into the Gate of the classic
Bedini charging circuit, often called the "Simplified School-Girl" or SSG circuit.
