#include <EEPROM.h>
#include <BasicStepperDriver.h>

//  Global constants
#define MOTOR_STEPS 200
#define STEP 10
#define DIR 9
const int MICROSTEP[8] = {0, 1, 2, 2, 4, 8, 16, 0};
//  Global variables
int DIP;            //  read from EEPROM
int RPM_setting;    //  read from EEPROM

//  Define spinner motor
BasicStepperDriver spinner(MOTOR_STEPS, DIR, STEP);

//  Software function to reset Arduinos
//  void (* resetFunc) (void) = 0;

void setup()
{
  // Read EEPROM settings
  EEPROM.update(0, 4);   //
  DIP = EEPROM.read(0);
  RPM_setting = EEPROM.read(2);
  delay(3000);   // Ensure master has initialized and is read to recieve

  // Wait for RPM setting via serial
  Serial.begin(9600);
  Serial.write(RPM_setting);  // Send existing setting to master
  delay(100);
  while(Serial.available()==0); //  Wait for response
  while(Serial.available()>0) {
    RPM_setting = Serial.read();
  }
  Serial.end();
  EEPROM.update(2, RPM_setting);    // Update EEPROM RPM setting if it has changed
  

  spinner.setRPM(MICROSTEP[DIP]*RPM_setting);    // set RPM to use with driver
  spinner.startMove(20 * MOTOR_STEPS * MICROSTEP[DIP]); // Give baseline move

}

void loop()
{
  spinner.nextAction();   // keep moving motor
  if (spinner.getStepsRemaining() == 0) spinner.startMove(20 * MOTOR_STEPS * MICROSTEP[DIP]); // if motor runs out of steps, just keep it going
}


