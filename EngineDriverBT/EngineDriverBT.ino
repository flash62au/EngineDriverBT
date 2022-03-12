/**
 * This app turns the ESP32 into a Bluetooth LE keyboard that is intended to act as a dedicated
 * gamepad for the JMRI Engine Drive Throttle (Engine Driver) Android app.
 */
#define BOUNCE_WITH_PROMPT_DETECTION    // Make button state changes available immediately

#include <BleKeyboard.h>          // https://github.com/T-vK/ESP32-BLE-Keyboard
#include <Bounce2.h>              // https://github.com/thomasfredericks/Bounce2
#include <AiEsp32RotaryEncoder.h> // https://github.com/igorantolic/ai-esp32-rotary-encoder

BleKeyboard bleKeyboard("EngineDriver BT", "DIY", 100); ;

int noButtons = 13;

#define DEBOUNCE_TIME 50 // the debounce time in millisecond, increase this time if it still chatters
Bounce debouncers[13];

//                           0     1     2     3     4     5     6     7     8     9     10    11    12
boolean buttonIsPressed[] = {false,false,false,false,false,false,false,false,false,false,false,false,false};
//char buttonChar[] =         {'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '=',  '-',  '[',  ']'};
char buttonChar[] =         {KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,KEY_F6,KEY_F7,KEY_F8,KEY_F9,KEY_UP_ARROW,KEY_DOWN_ARROW,KEY_LEFT_ARROW,KEY_RIGHT_ARROW};
byte buttonPins[] =         { 2,   4,    5,    15,   16,   17,   18,   19,   21,   22,   23,   12,   13};

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

void IRAM_ATTR readEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleKeyboard.begin();
  
  for (byte currentPinIndex = 0 ; currentPinIndex < noButtons ; currentPinIndex++) {
    pinMode(buttonPins[currentPinIndex], INPUT_PULLUP);
    debouncers[currentPinIndex] = Bounce();
    debouncers[currentPinIndex].attach(buttonPins[currentPinIndex]);      // After setting up the button, setup the Bounce instance :
    debouncers[currentPinIndex].interval(5);        
  }

  //we must initialize rotary encoder
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  //set boundaries and if values should cycle or not
  //in this example we will set possible values between 0 and 1000;
  rotaryEncoder.setBoundaries(0, 1000, circleValues); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)

  /*Rotary acceleration introduced 25.2.2021.
   * in case range to select is huge, for example - select a value between 0 and 1000 and we want 785
   * without accelerateion you need long time to get to that number
   * Using acceleration, faster you turn, faster will the value raise.
   * For fine tuning slow down.
   */
  //rotaryEncoder.disableAcceleration(); //acceleration is now enabled by default - disable if you dont need it
  rotaryEncoder.setAcceleration(250); //or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration
}

void loop() {
  if(bleKeyboard.isConnected()) {

    for (byte currentIndex = 0 ; currentIndex < noButtons ; currentIndex++) {
      debouncers[currentIndex].update();
      if (debouncers[currentIndex].fell()) {
        bleKeyboard.press(buttonChar[currentIndex]);
//        Serial.print("Button ");
//        Serial.print(buttonChar[currentIndex]);
//        Serial.println(" pushed.");
      }
      else if (debouncers[currentIndex].rose()) {
        bleKeyboard.release(buttonChar[currentIndex]);
//        Serial.print("Button ");
//        Serial.print(buttonChar[currentIndex]);
//        Serial.println(" released.");
      }
    } 

    rotary_loop();
    
    delay(100);
  }
}

// ----------------------------------------------------------------
// ----------------------------------------------------------------
// ----------------------------------------------------------------

void rotary_onButtonClick() {
  static unsigned long lastTimePressed = 0;
  //ignore multiple press in that time milliseconds
  if (millis() - lastTimePressed < 500) {
    return;
  }
  lastTimePressed = millis();
  bleKeyboard.press(KEY_HOME);
  delay(20);
  bleKeyboard.release(KEY_HOME);
//  Serial.print("button pressed ");
//  Serial.print(millis());
//  Serial.println(" milliseconds after restart");
}

void rotary_loop() {
  //dont print anything unless value changed
  if (rotaryEncoder.encoderChanged()) {
    char c;
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
      c = KEY_PAGE_DOWN;
    } else {
      c = KEY_PAGE_UP;
    } 
    bleKeyboard.press(c);
    delay(20);
    bleKeyboard.release(c);
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
