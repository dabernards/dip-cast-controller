// Built w/ arduino:avr v1.8.6
// using library ContinuousStepper v2.2.0 and EEPROM v2.0


#include <EEPROM.h>
//#include <BasicStepperDriver.h>   // Old driver
#include <ContinuousStepper.h>



//  Global constants
const uint8_t motorSteps = 200;
const uint8_t stepPin = 10;
const uint8_t dirPin = 9;
const int microStep[8] = {0, 1, 2, 2, 4, 8, 16, 0};

//  Global variables
int dipSetting;            //  read from EEPROM
int rpmSetting;    //  read from EEPROM

//  Define spinner motor
//BasicStepperDriver spinner(MOTOR_STEPS, DIR, STEP);   // Old driver
ContinuousStepper stepper;

void setup()
{
  // Read EEPROM settings
  EEPROM.update(0, 4);   //
  dipSetting = EEPROM.read(0);
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
  

  /* Old driver process
  spinner.setRPM(MICROSTEP[DIP]*RPM_setting);    // set RPM to use with driver
  spinner.startMove(20 * MOTOR_STEPS * MICROSTEP[DIP]); // Give baseline move
  */

  // Initialize stepper & set spin rate -- spin is in steps per second.
  stepper.begin(stepPin, dirPin);
  // ContinuousStepper.spin() takes motor steps / sec as input to set rotational speed
  // Since our driver operates in micro step, our input is:
  // microsteps/sec = motor steps/rev. * rev./min * microsteps/motor step * 1 min / 60 s
  stepper.spin(motorSteps*rpmSetting*microStep[dipSetting]/60);

}

void loop()
{
  /* Old driver process  
  spinner.nextAction();   // keep moving motor
  if (spinner.getStepsRemaining() == 0) spinner.startMove(20 * MOTOR_STEPS * MICROSTEP[DIP]); // if motor runs out of steps, just keep it going
  */

  // Run stepper process as often as possible
  stepper.loop();

}


