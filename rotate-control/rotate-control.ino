// Built w/ arduino:avr v1.8.6
// using library ContinuousStepper v2.2.0 and EEPROM v2.0

#include <EEPROM.h>
#include <ContinuousStepper.h>

//  Global constants
const int motorSteps = 200;
const uint8_t stepPin = 10;
const uint8_t dirPin = 9;
// Taken from stepper driver spec, where index is bitwise translation of dip switches: S1 is bit 1, S2 is bit 2, S3 is bit 3
const int microStep[8] = {32, 4, 8, 1, 16, 2, 2, 0};   

//  Global variables, both read from EEPROM stored settings
int dipSetting;
int rpmSetting;

//  Define spinner motor
ContinuousStepper stepper;

void setup()
{
  // Read EEPROM settings
  EEPROM.update(0,B100);
  dipSetting = EEPROM.read(0);    // Should add a fail-safe check to ensure dipSetting is valid
  rpmSetting = EEPROM.read(2);
  delay(3000);   // Ensure master has initialized and is read to recieve

  // Wait for RPM setting via serial
  Serial.begin(9600);
  Serial.write(rpmSetting);  // Send existing setting to master
  delay(100);
  while(Serial.available()==0); //  Wait for response
  while(Serial.available()>0) {
    rpmSetting = Serial.read();
  }
  Serial.end();
  EEPROM.update(2, rpmSetting);    // Update EEPROM RPM setting if it has changed
  
  // Initialize stepper & set spin rate -- spin is in steps per second.
  stepper.begin(stepPin, dirPin);

  // ContinuousStepper.spin() takes motor steps / sec as input to set rotational speed
  // Since our driver operates in micro step, our input is:
  // microsteps/sec = motor steps/rev. * microsteps/motor step * rev./min * 1 min / 60 s
  stepper.spin((float) motorSteps * microStep[dipSetting] * rpmSetting / 60.0);
}

void loop()
{
  // Run stepper.loop() as often as possible to ensure desired speed
  stepper.loop();
}


