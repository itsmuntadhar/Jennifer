# Jennifer
A digital inclinometer using Arduino and MPU6050 or MPU9250

# Abstract
In this prototype, a gyroscope-based module was developed that has relatively low cost and good accuracy. The gyroscope module was used to provide data which then get converted into angles in the Atmega328p-based Arduino Nano board. The prototype also has several functionalities such as displaying the angles on a 16x2 LCD screen, freeze the readings or compare two angles. The gyroscope module that was used is MPU9250, MPU6050 can also be used.

# Circuit Description
-LCD and the MPU are connected using the I2C protocol.

-Three (four if the 7-segments is used) buttons are connected to the digital pins (5, 6, 7) using the internal pull-up resistor.

  First (pin 5) button is used to "freeze" or "hold" the current value on the display.
  
  Second (pin 6) button is used to enter compare mode, where the following readings will be in respect to the values when the button is pressed.
  
  Third (pin 7) button is used to "set to zero" or make all future readings with respect the values when the button is pressed.
  
  Forth (pin 4) button is used to toggle which axis to view its value on the 7-segments display.
  
# About
This project was developed by Muntadhar Haydar and uses the "TM1637Display" and "LiquidCrystal_I2C" to communicate with the 7-segment and the LCD displays respectively. It also uses "MPU6050" library to communicate with the MPU module.
