const long unsigned int BAUD_RATE = 115200; //Baud rate

const long unsigned int DELAY_TIME = 3; //Amount to delay each loop

//const int OFFSET = 750; //Potentiometers read ~750 at the end when the fingers are at an open hand pose, but ideally it makes more sense for them to be 0, so invert the readings.

const double ADJUSTED_SCALE = 1000; //The max limit for our adjusted output (see below) 

const int a_0 = A0; //index finger pot
const int a_1 = A1; //middle finger pot
const int a_2 = A2; //ring finger pot
const int a_3 = A3; //pinkie finger pot

const int a_xJoyStick = A4; //x joystick
const int a_yJoyStick = A5; //y joystick

const int in_Joystick = 2; //Joystick button
const int in_Trigger = 3; //Trigger button
const int in_A = 4; //A button

//{each finger: {raw value, min, max, adjusted value}}
double fingerValues[4][4] = {{0,1000,0,0},{0,1000,0,0},{0,1000,0,0},{0,1000,0,0}};

//Size of our output array of bytes
unsigned outputByteSize = 98;

//Byte array output
byte outputBytes[98];

String response;

void setup() {
  pinMode(in_Joystick, INPUT_PULLUP);
  digitalWrite(in_Joystick, HIGH);
  pinMode(in_Trigger, INPUT);
  pinMode(in_A, INPUT);
  Serial.begin(BAUD_RATE);
}

void loop() {
  //Read the potentiometers into the array of fingerValues
  fingerValues[0][0] = 750 - analogRead(a_0);
  fingerValues[1][0] = 750 - analogRead(a_1);
  fingerValues[2][0] = 750 - analogRead(a_2);
  fingerValues[3][0] = 750 - analogRead(a_3);
  
  //For most cases, fingers may not strech the entirety of the potentiometer, so we will want to scale it accordingly to how much of the potentiometer they use. This value will be between 0-1000.
  //To do this, we iterate over each finger, getting the minimum and maximum values for each of the fingers and finding the limits for how far they strech, then fit that range to the range of 0-1000.
  for(int i = 0; i < 4; i++) {
    
    if(fingerValues[i][0] < fingerValues[i][1]) { //Check if we have a min value
      fingerValues[i][1] = fingerValues[i][0];
    } else if(fingerValues[i][0] > fingerValues[i][2]) { //Check if we have a max value
      fingerValues[i][2] = fingerValues[i][0];
    }
    
    fingerValues[i][3] = round(scaleBetween(fingerValues[i][0],fingerValues[i][1],fingerValues[i][2])); //Scale to the desired amount
  }

  //Build the response to include separators so the receiver can get each value
  response = buildResponse(fingerValues[0][3], fingerValues[1][3], fingerValues[2][3], fingerValues[3][3], analogRead(a_xJoyStick), analogRead(a_yJoyStick), digitalRead(in_Joystick), digitalRead(in_Trigger), digitalRead(in_A), 0);

  //We will end up sending a set amount of bytes, so the computer knows how long each message is
  response.getBytes(outputBytes, outputByteSize);
  
  Serial.write(outputBytes, outputByteSize);
  Serial.println();
  Serial.flush();
  
 delay(DELAY_TIME);
}

double scaleBetween(double val, double minVal, double maxVal) {
  //Calculate the percentage
  double percentage = (val - minVal)/(maxVal - minVal);
  
  //Get a percentage according to our adjusted scale, and return it
  return ((val - minVal)/(maxVal - minVal)) * ADJUSTED_SCALE;
}

//Build the response with separators

//                   index   middle  ring    pinkie  joyX    joyY    joyBtn  trgBtn  ABtn    BBtn
String buildResponse(int r1, int r2, int r3, int r4, int r5, int r6, int r7, int r8, int r9, int r10) {
    return String(r1) + "&" + String(r2) + "&" + String(r3) + "&" + String(r4) + "&" + String(r5) + "&" + String(r6) + "&" + String(r7 == HIGH ? 0 : 1)+ "&" + String(r8 == HIGH ? 1 : 0)+ "&" + String(r9 == HIGH ? 1 : 0) + "&" + String(r10 == HIGH ? 1 : 0);
}
