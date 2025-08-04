#include <IRremote.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

#define HEX_HOLD 0XFFFFFFFF
#define HEX_VOL_MINUS 0xFFE01F
#define HEX_VOL_PLUS 0xFFA857
#define HEX_VOL_EQ 0xFF906F
#define HEX_VOL_PLAY 0xFFC23D
#define HEX_VOL_CH 0xFF629D

const unsigned long INTERVAL_FEEDING = 60 * 60 * 2;

LiquidCrystal_I2C lcd(0x27, 16, 2);
bool noDisplay;

const int RECV_PIN = 7;
IRrecv irrecv(RECV_PIN);
decode_results results;

Servo myservo;

bool isOpen;
bool paused;
unsigned long servoInterval;
unsigned long lastLCDTime, lastServoTime, lastIRTime, lastFeedTime, pauseStartTime;

int frame;

void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn();
  irrecv.blink13(true);

  myservo.attach(9);
  myservo.write(0);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lastFeedTime = millis();
  refreshScreen();
}

void refreshScreen() {
  if (noDisplay)
    return;

  lcd.clear();
  lcd.setCursor(0, 0);

  if (!isOpen) {
    if (paused) {
      lcd.setCursor(0, 0);
      lcd.print("CatFeeder [!]");
    } else {
      unsigned long elapsedTime = millis() - lastFeedTime;
      Serial.println(elapsedTime / 1000);
      float remainingSeconds = (INTERVAL_FEEDING * 1000UL - elapsedTime) / 1000.0;

      char* unit;
      float displayValue;

      if (remainingSeconds >= 3600) {
        displayValue = remainingSeconds / 3600.0;
        unit = "H";
      } else if (remainingSeconds >= 60) {
        displayValue = remainingSeconds / 60.0;
        unit = "m";
      } else {
        displayValue = remainingSeconds;
        unit = "s";
      }

      lcd.setCursor(0, 0);
      lcd.print("CatFeeder ");
      if (unit == "s") {
        lcd.print((int)displayValue);
      } else {
        lcd.print(displayValue, 1);
      }
      lcd.print(unit);
      lcd.print("  ");
    }
  } else {
    lcd.print("Feeding time!");
  }

  lcd.setCursor(0, 1);
  if (!isOpen) {
    if (frame == 0)
      lcd.print("[WAIT] =^._.^=");
    else
      lcd.print("[WAIT] =^. .^=");
  } else {
    if (frame == 0)
      lcd.print("=^._.^=    <3");
    else
      lcd.print("=^ ._.^=   <3");
  }
}

void feed() {
  if (isOpen)
    return;

  lastFeedTime = millis();
  Serial.println("Feed time");
  isOpen = true;
  servoInterval = 10000UL;
  lastServoTime = millis();
  myservo.write(180);
  refreshScreen();
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastLCDTime >= 1000UL) {
    refreshScreen();
    frame = frame == 0 ? 1 : 0;
    lastLCDTime = currentTime;
  }

  if (currentTime - lastIRTime >= 100UL) {
    ir_receive();
    lastIRTime = currentTime;
  }

  if (isOpen) {
    if (currentTime - lastServoTime >= servoInterval) {
      isOpen = false;
      myservo.write(0);
      lastFeedTime = currentTime;
      Serial.println("Feed end");
    }
  } else {
    if (currentTime - lastFeedTime >= INTERVAL_FEEDING * 1000UL) {
      if (!paused) {
        Serial.println("Auto feed");
        feed();
      }
    }
  }
}

void ir_receive() {
  if (irrecv.decode(&results)) {
    if (results.decode_type == NEC) {
      int res = results.value;
      Serial.print("IR: ");
      Serial.println(results.value, HEX);

      switch (res) {
        case HEX_VOL_EQ:  //Manual feed
          Serial.println("Manual feed");
          feed();
          break;

        case HEX_VOL_PLAY:  // Toggle pause
          paused = !paused;
          if (paused) {
            pauseStartTime = millis();
          } else {
            lastFeedTime += (millis() - pauseStartTime);
          }
          refreshScreen();
          break;

        case HEX_VOL_CH:  // Toggle display
          noDisplay = !noDisplay;
          if (noDisplay)
            lcd.noBacklight();
          else
            lcd.backlight();
          refreshScreen();
          break;

        default:
          break;
      }
    }
    irrecv.resume();
  }
}
