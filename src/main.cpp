
#include <main.h>


Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_NUM , LED_PIN, NEO_GRB + NEO_KHZ800);

struct STRIP_PARAM myStrip;
struct SINGLE_LED_PARAM *ledPointer = nullptr;

void ignoreSerie();
void startUpPlug();
void I2CValueRead(int howMany);
void colorChanger(int power);
void ledMotion();
void stripOff();
void blinkFourTime();
void blinkFourTime();
void readDelay();

void setup() {
  Serial.begin(9600);      // initialize serial communication
  Serial.print("Setting Up Arduino");
  Wire.begin(8);                // join i2c bus with address #8
  Wire.onReceive(I2CValueRead); // register event
  strip.begin();
  stripOff();              //Ensures that the strip is off when the plug is turned on
  strip.setBrightness(30);
  blinkFourTime();
  myStrip.canStartMovement = 0;
  pinMode(HEART_BEAT_PIN, OUTPUT);
  Serial.print("Arduino Ready");
  Serial.print(" Waiting for configuration... ");
}


void loop() {
    if(myStrip.canStartMovement != 0) {
        ledMotion();
    }
    delay(100); // DO NOT REMOVE THIS
}


/* Sets start up variables.*/
void startUpPlug() {
    Serial.println("Setting up Initial Config");
        while (Wire.available()) {
            myStrip.relayState = Wire.read();
            myStrip.personNear = Wire.read();

            myStrip.leds_on = Wire.read();
            Serial.print("Number of Leds: ");
            Serial.print(myStrip.leds_on);
            Serial.println("");
            readDelay();
            //This is only to startup the led array (instanciate it)
            ledPointer= new SINGLE_LED_PARAM[myStrip.leds_on];
            myStrip.ledsConfigured = 0;
        }
        digitalWrite(HEART_BEAT_PIN, LOW);
    Serial.println("Initial Config ready");
}


/* Gets the values from I2C and reads them.*/
void I2CValueRead(int howMany){
 int pos;
 int startByte = Wire.read();
 Serial.print("This is my start byte");
 Serial.println(startByte);
 switch (startByte) {
   case (0):
     startUpPlug();
     break;
   case (1):
      POWER power;
     for(int i = 0; i <= 3; i++){
      power.bytes[i] = Wire.read();
     }
     colorChanger(power.value);
     break;
   case (2):
         myStrip.relayState = Wire.read(); // receive byte
      break;
   case (3):
       readDelay();
      break;
   case(4):
          if(myStrip.ledsConfigured <= myStrip.leds_on){

             (*(ledPointer + myStrip.ledsConfigured)).inital_position = Wire.read();
             (*(ledPointer + myStrip.ledsConfigured)).rotation = Wire.read();
             (*(ledPointer + myStrip.ledsConfigured)).myColor.r = Wire.read();
             (*(ledPointer + myStrip.ledsConfigured)).myColor.g = Wire.read();
             (*(ledPointer + myStrip.ledsConfigured)).myColor.b = Wire.read();
             (*(ledPointer + myStrip.ledsConfigured)).current_position = (*(ledPointer + myStrip.ledsConfigured)).inital_position;
             (*(ledPointer + myStrip.ledsConfigured)).isSelected = 0;

             //Serial.println((*(ledPointer + myStrip.ledsConfigured)).inital_position );
             //Serial.println((*(ledPointer + myStrip.ledsConfigured)).rotation);
             //Serial.println((*(ledPointer + myStrip.ledsConfigured)).myColor.r);
             //Serial.println((*(ledPointer + myStrip.ledsConfigured)).myColor.g);

              myStrip.ledsConfigured += 1;
              if(myStrip.ledsConfigured >= myStrip.leds_on){
                  myStrip.canStartMovement = 1;
              }
          }
          break;
    case(5):
        myStrip.personNear = Wire.read();
        break;
    case(6):
        //Change the color of one of the leds when its selected.
        pos = Wire.read();
        (*(ledPointer + pos)).myColor = (RGB){0,0,255};
        (*(ledPointer + pos)).isSelected = 1;
        break;
    case(7):
        //Change the Led color back to normal after it's been selected for a while.
        pos = Wire.read();
        (*(ledPointer + pos)).myColor = myStrip.generalColor;
        (*(ledPointer + pos)).isSelected = 0;
        break;
    case (8):
        //When socket is disconnected
          stripOff();
          for(byte i = 0; i < myStrip.leds_on; i++){
              (*(ledPointer + i)).myColor = (RGB){0,0,0};
          }
          break;
    default:
      ignoreSerie();
      break;
  }
  return;
 }

 /*Changes the color of the leds.*/
void colorChanger(int power){
  if(myStrip.personNear == 1 && myStrip.relayState == 0){
    myStrip.generalColor = (RGB) {102, 0 ,102};
  }else  if(power < THRESHOLD_GREEN &&  myStrip.personNear == 1){
    myStrip.generalColor = (RGB){0, 255, 0};
  }
  else if( power < THRESHOLD_YELLOW &&  myStrip.personNear == 1){
    myStrip.generalColor = (RGB){255, 255, 0};
  }
  else if(power > THRESHOLD_RED &&  myStrip.personNear == 1){
    myStrip.generalColor = (RGB){255, 0, 0};
  }
  else {
      myStrip.generalColor = (RGB) {0, 0, 0};
  }
  //Changes the color in all Leds.
  for(byte i = 0; i < myStrip.leds_on; i++){
      if(!(*(ledPointer + i)).isSelected) { //Blocks color change on leds that are selected.
          (*(ledPointer + i)).myColor = myStrip.generalColor;
      }
  }
}

/* Starts the motion */
void ledMotion(){
      for(byte i = 0; i < myStrip.leds_on; i++){
          if( (*(ledPointer + i)).rotation == 1){
              //Resets Position
              if((*(ledPointer + i)).current_position > LED_NUM - 1){
                  (*(ledPointer + i)).current_position = 0;
                  //HeartBeat - when first led passes in position 0
              }
              if((*(ledPointer + 0)).current_position == 0 ){
                  digitalWrite(HEART_BEAT_PIN, HIGH);
              }
              //Write The colors
              strip.setPixelColor((LED_NUM + (*(ledPointer + i)).current_position - 1)%LED_NUM,strip.Color(0,0,0));
              strip.setPixelColor((LED_NUM + (*(ledPointer + i)).current_position)%LED_NUM,strip.Color((*(ledPointer + i)).myColor.r,(*(ledPointer + i)).myColor.g,(*(ledPointer + i)).myColor.b));
              //Updates Current Position
              (*(ledPointer + i)).current_position = (*(ledPointer + i)).current_position + 1;
          }else if((*(ledPointer + i)).rotation == 2){
              if((*(ledPointer + i)).current_position <= 0){
                  (*(ledPointer + i)).current_position = 12;
                  //HeartBeat
              }
              if((*(ledPointer + 0)).current_position == 0 ){
                  digitalWrite(HEART_BEAT_PIN, HIGH);
              }
              //Write The colors
              strip.setPixelColor(((*(ledPointer + i)).current_position + 1)%LED_NUM,strip.Color(0,0,0));
              strip.setPixelColor(((*(ledPointer + i)).current_position)%LED_NUM,strip.Color((*(ledPointer + i)).myColor.r,(*(ledPointer + i)).myColor.g,(*(ledPointer + i)).myColor.b));

              //Updates Current Position
              (*(ledPointer + i)).current_position = (*(ledPointer + i)).current_position - 1;
          }
      }
      strip.show();
      delay(myStrip.delay.value);
      digitalWrite(HEART_BEAT_PIN, LOW);
      //heartBeatSent = false;
}

/*Clean the LED color*/
void stripOff(){
  for(int i = 0; i < LED_NUM; i++){
    strip.setPixelColor(i, 0, 0, 0);
    strip.show();
    }
}

/*Test the LED Colors*/
void blinkFourTime(){
  RGB color;
  for(int j =0; j < 3; j++){
    if(j == 0){
      color = (RGB){255, 0, 0};
    }
    else if( j == 1)
    {
      color = (RGB){255, 255, 0};
    }
    else{
      color = (RGB){0, 255, 0};
    }
    for(int k =0; k <= 1; k++){
      for(int i = 0; i < LED_NUM; i++){
        strip.setPixelColor(i, color.r, color.g, color.b);
       }
       strip.show();
       delay(500);
       stripOff();
       delay(500);
    }
   }
 }

/*Read Garbadge Value FROM I2C*/
void ignoreSerie() {
 while (Wire.available()) { // loop through all
   Wire.read(); // receive byte
 }
}

/*Reads Delay Values */
void readDelay(){
    for(int i = 0; i <= 3; i++){
        myStrip.delay.bytes[i] = Wire.read();
    }
    Serial.print("The delay is ");
    Serial.print(myStrip.delay.value);
}