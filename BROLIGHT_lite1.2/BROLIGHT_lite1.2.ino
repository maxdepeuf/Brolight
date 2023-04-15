#include "LowPower.h"
const int BUT = 2;
const int RED = 3;
const int PWM = 11;
const int ENA = 12;
const int TEM = A3;
int readingbutton;
int buttonState;
int lastButtonState = LOW;
int StopFlash;
int StartCut = 0;
int StateLamp = 1;
int StateRedLamp = 1;
int StateCut = 0;
int StateFlash = 0;
int StateRed = 0;
int StateBattery = 0;
int S = 1;
int O = 0;
int R = 1;
int temperature_warnning_seuil = 40;
int batterie_warnning_seuil = 25;
float tension_alim;
float temperatur;
float v_led = 2.4;
long t_button_up;
long t_button_down;
long t_wakeup;
long t_flash;
long t_statebattery;
long t_sos;
long t_temp;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long t_temp_warning = 1000;
unsigned long t_red = 600;
unsigned long t_double = 150;
unsigned long t_simple = 150;
unsigned long t_button_off = 1000;
unsigned long t_button_on = 1000;
unsigned long t_flash_on = 2000;
unsigned long t_battery_warning = 600000;
unsigned long t_batterylive = 0;
void(* resetFunc) (void) = 0;
void wakeUpTime() {
  t_wakeup = millis();
}
void wakeUp() {
}
unsigned int analogReadReference(void) {
#if defined(__AVR_ATmega328P__)
  ADMUX = 0x4F;
#elif defined(__AVR_ATmega2560__)
  ADCSRB &= ~(1 << MUX5);
  ADMUX = 0x5F;
#elif defined(__AVR_ATmega32U4__)
  ADCSRB &= ~(1 << MUX5);
  ADMUX = 0x5F;
#endif
  delayMicroseconds(5);
#if defined(__AVR_ATmega328P__)
  ADMUX = 0x4E;
#elif defined(__AVR_ATmega2560__)
  ADCSRB &= ~(1 << MUX5);
  ADMUX = 0x5E;
#elif defined(__AVR_ATmega32U4__)
  ADCSRB &= ~(1 << MUX5);
  ADMUX = 0x5E;
#endif
  delayMicroseconds(200);
  ADCSRA |= (1 << ADEN);
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADCL | (ADCH << 8);
}
void setup() {
  pinMode(BUT, INPUT_PULLUP);
  pinMode(TEM, INPUT);
  pinMode(RED, OUTPUT);
  pinMode(PWM, OUTPUT);
  pinMode(ENA, OUTPUT);
  digitalWrite(ENA, HIGH);
  digitalWrite(RED, LOW);
  digitalWrite(PWM, LOW);
  delay(100);
  Serial.begin(9600);
//  Serial.println(F("BROLIGHT Lite"));
//  Serial.print(F("Battery: "));
  tension_alim = (1023 * 1.1) / analogReadReference();
//  Serial.print(tension_alim, 3);
//  Serial.print(F(" V "));
//  Serial.print((tension_alim / 4.2) * 100.0);
//  Serial.println(F(" %"));
  StateRed = 1;
  batteryState(tension_alim);
  if (tension_alim <  ((batterie_warnning_seuil - 0.0) * (4.2 - 2.96) / (100.0) + 2.96) ) {
    t_batterylive = t_battery_warning;
  }
  if (tension_alim < 2.96) {
    CutBrolight();
  }
  StateRed = 0;
  delay(100);
  t_statebattery = millis();
  t_sos = millis();
  t_temp = millis();
  t_button_up = millis();
  t_button_down = millis();
  t_flash = millis();
}
void loop() {

  if (StateCut == 1) {
    sleep();
  } else {
    BatteryLive(batterie_warnning_seuil);
    button();
    mode();
    TempLive(temperature_warnning_seuil);
  }
  delay(10);
}
void button() {
  readingbutton = digitalRead(BUT);
  if (readingbutton != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readingbutton != buttonState) {
      buttonState = readingbutton;
      if (buttonState == LOW) {
        if ( StateFlash == 1) {
          StateFlash = 0;
          StopFlash = 1;
        } else {
          t_button_up = millis();
          if ((t_button_up - t_button_down) <= t_double) {
            t_flash = millis();
            if ( StateRed == 0) {
              StateFlash = 1;
            }
          }
        }
      } else {
        t_button_down = millis();
        if (StopFlash == 1) {
          StopFlash = 0;
        } else {
          if (((t_button_down - t_button_up) >= t_simple) && ((t_button_down - t_button_up) < 2 * t_simple)) {
            if (StateRed == 0) {
              StateLamp++;
              if (StateLamp == 8) {
                R = 1;
                S = 1;
                O = 0;
                t_sos = millis();
              }
              if (StateLamp >= 9) {
                StateLamp = 1;
              }
            } else {
              StateRedLamp++;
              if (StateRedLamp >= 3) {
                StateRedLamp = 1;
              }
            }
          }      else if ((t_button_down - t_button_up) >= t_red) {
            //if (StateLamp == 1) {
            if (StateRed == 1) {
              Red(0);
              StateRed = 0;
            } else {
              StateRed = 1;
              PwmLed(0);
            }
            //}
          }
        }
      }
    }
    if (buttonState == LOW) {
      if ((millis() - t_button_up) >= t_button_off) {
        CutBrolight();
      }
    }
  }

  lastButtonState = readingbutton;
}
void mode() {
  if (StateBattery == 1) {
    batteryState(tension_alim);
    if (StartCut == 1) {
      CutBrolight();
    }

  } else {
    if (StateFlash == 1) {
      flash();
    }
    else if (StateRed == 1) {
      if (StateRedLamp == 1) {
        Red(100);
      } else if (StateRedLamp == 2) {
        CligoRed();
      }
    }
    else {
      if (StateLamp == 1) {
        PwmLed(1);//20lum
      }
      else if (StateLamp == 2) {
        PwmLed(5);//100lum
      }
      else if (StateLamp == 3) {
        PwmLed(10);//200lum
      }
      else if (StateLamp == 4) {
        PwmLed(20);//400lum
      }
      else if (StateLamp == 5) {
        PwmLed(40);//800lum
      }
      else if (StateLamp == 6) {
        PwmLed(60);//1200lum
      }
      else if (StateLamp == 7) {
        PwmLed(80);//1600lum
      }
      else if (StateLamp == 8) {
        SOS();
      }
    }
  }
}
void Red(int state) {
  if (state == 0) {
    analogWrite(RED, 0);
  } else {
    analogWrite(RED, (int)(((float)state / 100) * (map(((tension_alim - 2.96) * (((v_led / 4.2) * 100) - ((v_led / 2.96) * 100)) / (4.2 - 2.96) + ((2.4 / 2.96) * 100)), 0, 100, 0, 255))));

  }
}
void CligoRed() {
  if ((int)round(millis() / 100) % 5 == 0) {
    Red(100);
  } else {
    Red(0);
  }
}
void flash() {
  if ((millis() - t_flash) <= t_flash_on) {
    PwmLed(100);
  } else {
    StateFlash = 0;
  }
}
void PwmLed(int dutyCycle) {
  analogWrite(PWM, map(dutyCycle, 0, 100, 0, 255));
}
void CutBrolight() {
  digitalWrite(PWM, LOW);
  digitalWrite(RED, LOW);
  digitalWrite(ENA, LOW);
  StateCut = 1;
}
void BatteryLive(int bat_threshold) {
  tension_alim = (1023 * 1.1) / analogReadReference();
  if (tension_alim <  ((bat_threshold - 0.0) * (4.2 - 2.96) / (100.0) + 2.96) ) {
    if ((tension_alim > 2.96)) {
      if ((millis() - t_statebattery) > t_batterylive) {
        StateBattery = 1;
        t_statebattery = millis();
        t_batterylive = t_battery_warning;
        PwmLed(0);
        Red(0);
      }
    } else {
      StateBattery = 1;
      PwmLed(0);
      Red(0);
    }
  }
}
void batteryState(float batteryLoad) {
  if (batteryLoad < (0.8 * 3.7)) {
    batteryWarning();
  }
  else if ((batteryLoad >= (0.8 * 3.7)) && (batteryLoad < (0.867 * 3.7))) {
    batteryBlink(4);
  }
  else if ((batteryLoad >= (0.867 * 3.7)) && (batteryLoad < (0.933 * 3.7))) {
    batteryBlink(3);
  }
  else if ((batteryLoad >= (0.933 * 3.7)) && (batteryLoad < (3.7))) {
    batteryBlink(2);
  }
  else if (batteryLoad >= 3.7) {
    batteryBlink(1);
  }
  StateBattery = 0;
}
void batteryBlink(int nbrep) {
  for (int i = 1; i <= nbrep; i++) {
    for (int j = 0; j <= 25; j++) {
      if (StateRed == 1) {
        Red(4 * j);
      } else {
        PwmLed(j);
      }
      delay(((50 / nbrep) / 2));
    }
    for (int j = 24; j >= 0; --j) {
      if (StateRed == 1) {
        Red(4 * j);
      } else {
        PwmLed(j);
      }
      delay(((50 / nbrep) / 2));
    }
  }
}
void batteryWarning() {
  for (int i = 1; i <= 5; i++) {
    if (StateRed == 1) {
      Red(100);
    } else {
      PwmLed(5);
    }
    delay(100);
    if (StateRed == 1) {
      Red(0);
    } else {
      PwmLed(0);
    }
    delay(100);
  }
  StartCut = 1;
}
void sleep() {
  attachInterrupt(0, wakeUpTime, CHANGE);
  detachInterrupt(0);
  attachInterrupt(0, wakeUp, LOW);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  detachInterrupt(0);
  digitalWrite(PWM, LOW);
  digitalWrite(RED, LOW);
  digitalWrite(ENA, LOW);
  if ((millis() - t_wakeup) >= t_button_on) {
    resetFunc();
  }
}
void SOS() {
  if (R < 7) {
    if ((S > 0) && (S < 4)) {
      if ((millis() - t_sos) < 150) {
        PwmLed( 50);
      }
      else if (((millis() - t_sos) >= 150) && ((millis() - t_sos) <= 250)) {
        PwmLed(0);
      }
      else if ((millis() - t_sos) > 250) {
        t_sos = millis();
        S++;
        R++;
      }
    }
    else if (S == 4) {
      if ((millis() - t_sos) > 100) {
        t_sos = millis();
        S = 0;
        O = 1;
      }
      else {
        PwmLed(0);
      }
    }
    else if ((O > 0) && (O < 4)) {
      if ((millis() - t_sos) < 400) {
        PwmLed( 50);
      }
      else if (((millis() - t_sos) >= 400) && ((millis() - t_sos) <= 500)) {
        PwmLed(0);
      }
      else if ((millis() - t_sos) > 500) {
        t_sos = millis();
        O++;
      }
    }
    else if (O == 4) {
      if ((millis() - t_sos) > 100) {
        t_sos = millis();
        O = 0;
        S = 1;
      } else {
        PwmLed(0);
      }
    }
  }
  else if (R == 7) {
    if ((millis() - t_sos) > 1000) {
      t_sos = millis();
      O = 0;
      S = 1;
      R = 1;
    } else {
      PwmLed(0);
    }
  }
}
void TempLive(int temp_threshold) {
  temperatur = (((((float)analogRead(TEM))*tension_alim)/1024.0)-0.5)/0.01;
  Serial.print(tension_alim);
  Serial.print(",");
  Serial.println(temperatur);
  if (temperatur > temp_threshold) {
    if ((millis() - t_temp) > t_temp_warning) {
      t_temp = millis();
      if (StateFlash == 1) {
        StateFlash = 0;
      } else {
        StateLamp = StateLamp - 1;
        if ( StateLamp <= 1) {
          StateLamp = 1;
        }
      }
    }
  }
}
