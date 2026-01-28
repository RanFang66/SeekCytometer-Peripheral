# Seek Cytometer Peripheral Control

This project is for peripheral control in Seek Cytometer, a Fluorescence-activated Cell Sorter（FACS). It composed by four part:

- AD and Communicate board: for signal sampling and communication with HMI and other boards(communication bridge)
- Motor Control board: for motors control, laser and led control and temperature control.
- MFC board: for microfluidic control.
- PZT Control: for PZT auto pressing control
- HMI: A HMI based on QT widgets to control all those peripheral through MODBUS-RTU protocol. 