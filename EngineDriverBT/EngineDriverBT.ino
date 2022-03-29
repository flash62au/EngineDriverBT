/**
 * This app turns the ESP32 into a Bluetooth LE keyboard that is intended to act as a dedicated
 * gamepad for the JMRI Engine Driver Throttle (Engine Driver) Android app.
 */
#define BOUNCE_WITH_PROMPT_DETECTION    // Make button state changes available immediately

#include <Keypad.h> // https://www.arduinolibraries.info/libraries/keypad
#include <BleKeyboard.h>          // https://github.com/T-vK/ESP32-BLE-Keyboard
#include <AiEsp32RotaryEncoder.h> // https://github.com/igorantolic/ai-esp32-rotary-encoder
// un-comment if you wish to use discrete buttons
//#include <Bounce2.h>              // https://github.com/thomasfredericks/Bounce2

BleKeyboard bleKeyboard("EngineDriver BT", "DIY", 100); ;

#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // four columns
char keys[ROW_NUM][COLUMN_NUM] = {
  {KEY_F1, KEY_F2, KEY_F3, KEY_UP_ARROW},
  {KEY_F4, KEY_F5, KEY_F6, KEY_DOWN_ARROW},
  {KEY_F7, KEY_F8, KEY_F9, KEY_LEFT_ARROW},
  {KEY_F11, KEY_F10, KEY_F12, KEY_RIGHT_ARROW}
};

byte pin_rows[ROW_NUM]      = {19, 18, 5, 17}; // GIOP19, GIOP18, GIOP5, GIOP17 connect to the row pins
byte pin_column[COLUMN_NUM] = {16, 4, 0, 2};   // GIOP16, GIOP4, GIOP0, GIOP2 connect to the column pins

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

// un-comment and alter if you wish to use discrete buttons
//int noButtons = 13;
//#define DEBOUNCE_TIME 50 // the debounce time in millisecond, increase this time if it still chatters
//Bounce debouncers[13];
// //                           0     1     2     3     4     5     6     7     8     9     10    11    12
//boolean buttonIsPressed[] = {false,false,false,false,false,false,false,false,false,false,false,false,false};
//char buttonChar[] =         {KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,KEY_F6,KEY_F7,KEY_F8,KEY_F9,KEY_UP_ARROW,KEY_DOWN_ARROW,KEY_LEFT_ARROW,KEY_RIGHT_ARROW};
//byte buttonPins[] =         { 2,   4,    5,    15,   16,   17,   18,   19,   21,   22,   23,   12,   13};

#define ROTARY_ENCODER_A_PIN 32
#define ROTARY_ENCODER_B_PIN 25
#define ROTARY_ENCODER_BUTTON_PIN 26
#define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */

//depending on your encoder - try 1,2 or 4 to get expected behaviour
//#define ROTARY_ENCODER_STEPS 1
#define ROTARY_ENCODER_STEPS 2
//#define ROTARY_ENCODER_STEPS 4

bool circleValues = true;
int encoderValue = 0;
int lastEncoderValue = 0;

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

// ----------------------------------------------------------------
void IRAM_ATTR readEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}

// ----------------------------------------------------------------

void rotary_onButtonClick() {
  static unsigned long lastTimePressed = 0;
  //ignore multiple press in that time milliseconds
  if (millis() - lastTimePressed < 500) {
    Serial.print(millis());
    Serial.println(" milliseconds after restart");
    return;
  }
  lastTimePressed = millis();
  bleKeyboard.press(KEY_HOME);
  delay(20);
  bleKeyboard.release(KEY_HOME);
  Serial.print("encoder button pressed ");
//  Serial.print(millis());
//  Serial.println(" milliseconds after restart");
}

void rotary_loop() {
  //dont print anything unless value changed
  if (rotaryEncoder.encoderChanged()) {
    encoderValue = rotaryEncoder.readEncoder();
    if (abs(encoderValue-lastEncoderValue) > 700) { // must have passed through zero
      Serial.print("Encoder Value: ");
      Serial.print(encoderValue);
      Serial.print(" Last Encoder Value: ");
      Serial.print(lastEncoderValue);
      Serial.println(" ");
      if (encoderValue > 700) {
        lastEncoderValue = lastEncoderValue + 1001; 
      } else {
        lastEncoderValue = lastEncoderValue - 1001; 
      }
    }
    if (encoderValue > lastEncoderValue) {
        if (abs(encoderValue-lastEncoderValue)<50) {
          bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
        } else {
          bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
        }
    } else {
        if (abs(encoderValue-lastEncoderValue)<50) {
          bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        } else {
          bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
        }
    } 
    Serial.print("New Encoder Value: ");
    Serial.print(encoderValue);
    Serial.print(" Last Encoder Value: ");
    Serial.print(lastEncoderValue);
    Serial.println(" ");
    lastEncoderValue = encoderValue;
  }
  
  if (rotaryEncoder.isEncoderButtonClicked()) {
    rotary_onButtonClick();
  }
}

// ----------------------------------------------------------------
void keypadEvent(KeypadEvent key){
    switch (keypad.getState()){
    case PRESSED:
        bleKeyboard.press(key);
         Serial.print("Button ");
         Serial.print(String(key - '0'));
         Serial.println(" pushed.");
        break;
    case RELEASED:
        bleKeyboard.release(key);
         Serial.print("Button ");
         Serial.print(String(key - '0'));
         Serial.println(" released.");
        break;
//    case HOLD:
//        break;
    }
}

// ----------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleKeyboard.begin();
  keypad.addEventListener(keypadEvent); // Add an event listener for this keypad

// uncomment if you wish to use discrete buttons
//  for (byte currentPinIndex = 0 ; currentPinIndex < noButtons ; currentPinIndex++) {
//    pinMode(buttonPins[currentPinIndex], INPUT_PULLUP);
//    debouncers[currentPinIndex] = Bounce();
//    debouncers[currentPinIndex].attach(buttonPins[currentPinIndex]);      // After setting up the button, setup the Bounce instance :
//    debouncers[currentPinIndex].interval(5);        
//  }

  //we must initialize rotary encoder
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  //set boundaries and if values should cycle or not
  //in this example we will set possible values between 0 and 1000;
  rotaryEncoder.setBoundaries(0, 1000, circleValues); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)

  //rotaryEncoder.disableAcceleration(); //acceleration is now enabled by default - disable if you dont need it
  rotaryEncoder.setAcceleration(250); //or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration
}

// ----------------------------------------------------------------
void loop() {
  if(bleKeyboard.isConnected()) {
    char key = keypad.getKey();

// uncomment if you wish to use discrete buttons
//    for (byte currentIndex = 0 ; currentIndex < noButtons ; currentIndex++) {
//      debouncers[currentIndex].update();
//      if (debouncers[currentIndex].fell()) {
//        bleKeyboard.press(buttonChar[currentIndex]);
// //        Serial.print("Button ");
// //        Serial.print(buttonChar[currentIndex]);
// //        Serial.println(" pushed.");
//      }
//      else if (debouncers[currentIndex].rose()) {
//        bleKeyboard.release(buttonChar[currentIndex]);
// //        Serial.print("Button ");
// //        Serial.print(buttonChar[currentIndex]);
// //        Serial.println(" released.");
//      }
//    } 

    rotary_loop();
    delay(100);
  }
}
