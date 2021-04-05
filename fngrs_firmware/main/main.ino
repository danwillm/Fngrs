const long unsigned int BAUD_RATE = 115200; //Baud rate

const long unsigned int DELAY_TIME = 5; //Amount to delay each loop

const int OFFSET = 1023;
const double ADJUSTED_SCALE = 1023; //The max limit for our adjusted output (see below) 

const int THRESHOLD_GRAB = 500;
const int THRESHOLD_PINCH = 500;

const int in_Thumb = A0; //index finger pot
const int in_Index = A1; //middle finger pot
const int in_Middle = A2; //ring finger pot
const int in_Ring = A3; //pinkie finger pot
const int in_Pinky = A4; //pinkie finger pot

const int a_xJoyStick = A5; //x joystick
const int a_yJoyStick = A6; //y joystick

const int in_Joystick = 2; //Joystick button
const int in_Trigger = 3; //Trigger button
const int in_A = 4; //A button
const int in_B = 5;
//{each finger: {raw value, min, max, adjusted value}}
//Set the thumb value to the threshold for pinching - we don't have a thumb input.

int fingerValues[5][4] = {{0,1000,0,0},{0,1000,0,0},{0,1000,0,0},{0,1000,0,0},{0,1000,0,0}};


void setup() {
  pinMode(in_Joystick, INPUT_PULLUP);
  digitalWrite(in_Joystick, HIGH);
  pinMode(in_Trigger, INPUT);
  pinMode(in_A, INPUT);
  pinMode(in_B, INPUT);
  Serial.begin(BAUD_RATE);
}

void loop() {
  fingerValues[0][0] = analogRead(in_Thumb);
  fingerValues[1][0] = analogRead(in_Index);
  fingerValues[2][0] = analogRead(in_Middle);
  fingerValues[3][0] = analogRead(in_Ring);
  fingerValues[4][0] = analogRead(in_Pinky);

  //For most cases, fingers may not strech the entirety of the potentiometer, so we will want to scale it accordingly to how much of the potentiometer they use. This value will be between 0-1023.
  //To do this, we iterate over each finger, getting the minimum and maximum values for each of the fingers and finding the limits for how far they strech, then fit that range to the range of 0-1023.
  for(int i = 0; i < 5; i++) {
    
    if(fingerValues[i][0] < fingerValues[i][1]) { //Check if we have a min value
      fingerValues[i][1] = fingerValues[i][0];
    } else if(fingerValues[i][0] > fingerValues[i][2]) { //Check if we have a max value
      fingerValues[i][2] = fingerValues[i][0];
    }
    
    fingerValues[i][3] = OFFSET - round(scaleBetween(fingerValues[i][0],fingerValues[i][1],fingerValues[i][2])); //Scale to the desired amount
  }

  const int pinchVal = round((fingerValues[0][3] + fingerValues[1][3]) / 2);
  const int grabVal = round((pinchVal + fingerValues[2][3] + fingerValues[3][03] + fingerValues[4][3]) / 4);
  
  bool inGrab = false;
  bool inPinch = false;

  if(grabVal > THRESHOLD_GRAB) {
    inGrab = true;
  } else if(pinchVal > THRESHOLD_PINCH) {
    inPinch = true;
  }

  char stringToPrint[75];

  sprintf(stringToPrint, "%d&%d&%d&%d&%d&%d&%d&%d&%d&%d&%d&%d&%d\n", fingerValues[0][3], fingerValues[1][3], fingerValues[2][3], fingerValues[3][3], fingerValues[4][3], analogRead(a_xJoyStick), analogRead(a_yJoyStick), 1 - digitalRead(in_Joystick), digitalRead(in_Trigger), digitalRead(in_A), 0, inGrab ? 1 : 0, inPinch ? 1 : 0);

  Serial.print(stringToPrint);
  Serial.flush();
  
 delay(DELAY_TIME);
}

double scaleBetween(double val, double minVal, double maxVal) {  
  //Get a percentage according to our adjusted scale, and return it
  return ((val - minVal)/(maxVal - minVal)) * ADJUSTED_SCALE;
}
