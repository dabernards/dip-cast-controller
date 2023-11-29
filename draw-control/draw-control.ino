// Built w/ arduino:avr v1.8.6
// using library StepperDriver v1.3.1, LiquidCrystal 1.0.7 and EEPROM v2.0
// LCDKeypad is unversioned legacy library provided by Sainsmart

#include <EEPROM.h>
#include <BasicStepperDriver.h>
#include <LiquidCrystal.h>
#include <LCDKeypad.h>

// Global constants
#define MOTOR_STEPS 200
#define STEP 3
#define DIR 2
#define PROG_STEPS 3

// 1/4 micro step is preferred generally, S4 is bit 1, S5 is bit 2, S6 is bit 3
// Sainsmart Stepper Driver constants
// const int MICROSTEP[8] = {0, 4, 2, 16, 1, 8, 2, 0};
// Twotrees Stepper Driver constants
const int MICROSTEP[8] = {32, 4, 8, 1, 16, 2, 2, 0};
const float PRESETS[5] = {0.1, 0.5, 1, 5, 10};

// Custom Characters for LCD
const byte c_up[8] = { B00100, B01110, B10101, B00100, B00100, B00100, B00100, B00100};       // up arrow
const byte c_down[8] = { B00100, B00100, B00100, B00100, B00100, B10101, B01110, B00100};     // down arrow
const byte c_right[8] = { B00000, B00100, B00010, B11111, B00010, B00100, B00000, B00000};    // right arrow
const byte c_left[8] = { B00000, B00100, B01000, B11111, B01000, B00100, B00000, B00000};     // left arrow

// Define stepper; specify steps/rev, direction and step pins
BasicStepperDriver stepper(MOTOR_STEPS, DIR, STEP);

// Start the LCD and associated keypad
LCDKeypad lcd;     

// Settings read from rotate-control (Serial)
uint8_t rotate_dip;
uint8_t RPM_setting;

// Settings read from EEPROM
uint8_t draw_dip;
int auto_rpm;   

// Operating variables
int cur_rpm = 240;      // corresponds to default initial speed of 20 mm/s
int key_val=-1;

void setup()
{ 
  // Read settings from EEPROM
  draw_dip = EEPROM.read(0);
  auto_rpm = EEPROM.read(2);

  // Initiate LCD, including some custom characters 
  lcd.createChar(2,c_up);
  lcd.createChar(3,c_down);
  lcd.createChar(4,c_right);
  lcd.createChar(5,c_left);
  lcd.begin(16, 2);
  
  lcd.clear();
  lcd.print("firmware: 1.4.1");

  Serial.begin(9600);

  // Holding a button on start up allows dip settings to be reset/adjusted
  long countup = millis();
  while ((millis()-countup) < 2000) {
    if (lcd.button() != KEYPAD_NONE) resetStored();
    delay(50);
  }

  // Obtain RPM from rotate-control and adjust
  lcd.clear();
  lcd.print("get settings...");
  Serial.write(0xFF);   // Send the all clear to communicate
  while(Serial.available()==0); //  Wait for response
  while(Serial.available()>0) {
    RPM_setting = Serial.read();
  }
  RPM_setting = dispUpdate(RPM_setting, "RPM", " ");
  Serial.write(RPM_setting);
  Serial.end();

  // Adjust speed for automatic draw of 10 cm
  auto_rpm = int(60 * dispUpdate_float(rpmToSpeed(auto_rpm), "Speed", "mm/s") / 5);
  EEPROM.update(2,auto_rpm);

  // Initiate stepper
  stepper.begin(cur_rpm, MICROSTEP[draw_dip]);
  lcd.clear();
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void loop()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.write(byte(2));
  lcd.setCursor(1,0);
  lcd.write(byte(3));
  lcd.setCursor(2,0);
  lcd.print("move");
  lcd.setCursor(7,0);
  lcd.write(byte(4));
  lcd.setCursor(8,0);
  lcd.write(byte(5));
  lcd.setCursor(9,0);
  lcd.print("adj spd");

  while(1) {
    lcd.setCursor(0,1);
    //String num_disp = String(rpmToSpeed(stepper.getRPM()),2);
    lcd.print("Speed=" + String(rpmToSpeed(stepper.getRPM()),2) + "mm/s    ");
    manualMove();
  }
}

void resetStored() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Reset menu,");
  lcd.setCursor(0,1);
  lcd.print("release key...");
  while(lcd.button()!=KEYPAD_NONE);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Clear Settings?");
  lcd.setCursor(0,1);
  lcd.print("y-select  ");
  while((key_val=waitForKey())==KEYPAD_NONE);
  lcd.clear();
  if (key_val==KEYPAD_SELECT) {
    rotate_dip = setDip(B100, "rotate dip");    // default value is 4 (SW3 ON)
    lcd.clear();
    lcd.setCursor(0,0);
    Serial.write(rotate_dip);
    draw_dip = setDip(B001, "draw dip");    // default value is 1 (SW4 ON)
    EEPROM.update(0, draw_dip);
    delay(1000);
    resetFunc();
  } else {
    lcd.print("not reseting");
  }
  delay(2000);
}

uint8_t setDip(uint8_t var_in, String dip_name) {
  lcd.clear();      // Update display w/ instructions
  lcd.setCursor(0,0);
  lcd.write(byte(2));
  lcd.setCursor(1,0);
  lcd.write(byte(3));
  lcd.setCursor(2,0);
  lcd.print("adj#,select=ok");

  while(1) {  // Run until select used to return setting
    lcd.setCursor(0,1);       // Update display w/ new value
    lcd.print(dip_name + ":" + String(var_in, BIN) + "    ");

    while((key_val=waitForKey())==KEYPAD_NONE);    // Wait for keypress

    switch (key_val) {    // Increment/decrement w/ up/down key; return value w/ select
      case KEYPAD_UP:
        if (var_in < B111) var_in += 1;
        break;
      case KEYPAD_DOWN:
        if (var_in > B0) var_in -= 1;
        break;
      case KEYPAD_SELECT:
        return var_in;
    }
  }
}

uint8_t dispUpdate(uint8_t var_in, String var_name, String var_unit) {
  //multip = 5;
  int mult_index = 2;
  key_val = -1;

  lcd.clear();

  lcd.setCursor(0,0);
  lcd.write(byte(2));
  lcd.setCursor(1,0);
  lcd.write(byte(3));
  lcd.setCursor(2,0);
  lcd.print("adj#");
  lcd.setCursor(7,0);
  lcd.write(byte(4));
  lcd.setCursor(8,0);
  lcd.write(byte(5));
  lcd.setCursor(9,0);
  lcd.print("scale ");

  while(key_val != KEYPAD_SELECT) {
    // Update display w/ new value
    lcd.setCursor(0,1);
    lcd.print(var_name + ":" + String(var_in) + var_unit);

    while((key_val=waitForKey())==KEYPAD_NONE);    // Wait for keypress
    if ((key_val==KEYPAD_RIGHT) && (mult_index < 4)) mult_index += 1;
    if ((key_val==KEYPAD_LEFT) && (mult_index > 2)) mult_index -= 1;

    //  Use up and and down to move stage
    if ((key_val==KEYPAD_UP || key_val==KEYPAD_DOWN)) var_in += PRESETS[mult_index] * pow(-1, (key_val==KEYPAD_DOWN));
  }
  return var_in;
}

float dispUpdate_float(float var_in, String var_name, String var_unit) {
  int mult_index = 2;
  key_val = -1;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.write(byte(2));
  lcd.setCursor(1,0);
  lcd.write(byte(3));
  lcd.setCursor(2,0);
  lcd.print("adj#");
  lcd.setCursor(7,0);
  lcd.write(byte(4));
  lcd.setCursor(8,0);
  lcd.write(byte(5));
  lcd.setCursor(9,0);
  lcd.print("scale ");

  while(key_val != KEYPAD_SELECT) {
    // Update display w/ new value
    lcd.setCursor(0,1);
    //String num_disp = String(rpmToSpeed(stepper.getRPM()),2);
    lcd.print(var_name + ":" + String(var_in,2) + var_unit);

    while((key_val=waitForKey())==KEYPAD_NONE);    // Wait for keypress
    if ((key_val==KEYPAD_RIGHT) && (mult_index < 4)) mult_index += 1;
    if ((key_val==KEYPAD_LEFT) && (mult_index > 0)) mult_index -= 1;

    //  Use up and and down to move stage
    if ((key_val==KEYPAD_UP || key_val==KEYPAD_DOWN)) var_in += PRESETS[mult_index] * pow(-1, (key_val==KEYPAD_DOWN));
  }
  return var_in;
}



int manualMove() {

  while((key_val=waitForKey())==KEYPAD_NONE);    // Wait for keypress
  //  Left and right key to adjust RPM up and down
  if ((key_val==KEYPAD_RIGHT) && (stepper.getRPM()<300)) stepper.setRPM(60*((stepper.getRPM()/60))+60);
//  if (key_val==KEYPAD_LEFT && (stepper.getRPM()>60)) stepper.setRPM(stepper.getRPM()-12);
  if (key_val==KEYPAD_LEFT && (stepper.getRPM()>60)) stepper.setRPM(stepper.getRPM()-54);
  if (key_val==KEYPAD_LEFT && (stepper.getRPM()>6)) stepper.setRPM(stepper.getRPM()-6);
  //  Use up and and down to move stage
  if ((key_val==KEYPAD_UP || key_val==KEYPAD_DOWN)) {
    stepper.startMove(20 * MOTOR_STEPS * MICROSTEP[draw_dip] * pow(-1, (key_val==KEYPAD_DOWN)));
    while(lcd.button()!=KEYPAD_NONE) {
      stepper.nextAction();
      if (stepper.getStepsRemaining() == 0) stepper.startMove(10 * MOTOR_STEPS * MICROSTEP[draw_dip] * pow(-1, (key_val==KEYPAD_DOWN)));
    }
    stepper.stop();
    stepper.startMove(0);
    stepper.nextAction();
  }
  // if (key_val==KEYPAD_SELECT) setMode();
  if (key_val==KEYPAD_SELECT) {
    cur_rpm = stepper.getRPM();
    stepper.setRPM(auto_rpm);
    lcd.setCursor(0,1);
    lcd.print("Speed=" + String(rpmToSpeed(stepper.getRPM()),2) + "mm/s    ");
    stepper.startMove(102 * MOTOR_STEPS * MICROSTEP[draw_dip]);
    while(stepper.getStepsRemaining() != 0) {
      stepper.nextAction();
    }
    stepper.setRPM(cur_rpm);
    lcd.setCursor(0,1);
    lcd.print("Speed=" + String(rpmToSpeed(stepper.getRPM()),2) + "mm/s    ");
  }
  return key_val;
}


float rpmToSpeed(int rpm_in) {
  float speed_calc;
  speed_calc = 5 * float(rpm_in) / 60;
  return speed_calc;
}


int waitForKey()   // Single key press and release functionality
{
  int buttonPressed; 

  // Delay to wait for button release -- otherwise button overactivation is a big issue
  delay(50);
  while(lcd.button()!=KEYPAD_NONE);
  delay(50);

  //  Capture button that is pressed
  while((buttonPressed=lcd.button())==KEYPAD_NONE);
  delay(50);  
  return buttonPressed;
}

