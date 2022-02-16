# README

To reliably dip cast polymer coatings, an Arduino-based controller was developed to allow control of dip-casting draw speed and mandrel rotation rate.

## Design

The dip casting apparatus described there employs two stepper motors and stepper drivers to reliably draw a mandrel (often a metal wire) from a casting solution. Mandrel draw from the casting solution is achieved with a stepper motor linear actuator. Mandrel rotation is achieved via a linear-actuator mounted stepper motor and mandrel coupling. Each motor-driver combination was controlled with an Arduino Uno in this embodiment, but other microcontrollers could be used in its place. The two Arduinos exchange settings information via a Serial connection, and a 12 V power supply is sufficient to power the Arduinos and stepper drivers. Additional details of the physical build will be added at a later time.
