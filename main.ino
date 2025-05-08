#include "LiquidCrystal.h"
#include "LowPower.h"

LiquidCrystal lcd(12, 7, 11, 10, 9, 8);

void setLCDPinState(int state, bool forceLow) {
  pinMode(12, state);
  pinMode(7, state);
  pinMode(11, state);
  pinMode(10, state);
  pinMode(9, state);
  pinMode(8, state);

  if (forceLow) {
    digitalWrite(12, LOW);
    digitalWrite(7, LOW);
    digitalWrite(11, LOW);
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
    digitalWrite(8, LOW);
  }
}

void setup() {
  lcd.begin(16, 2);
  lcd.print("sambalika");
  Serial.begin(9600);

  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);

  setLCDPinState(OUTPUT, false);
  
  pinMode(LED_BUILTIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(2), nothing, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), nothing, CHANGE);
    
}

int timerToMs(int time) {
  return time * 900;
}

bool pins[13];

int inputMode = 1;

int lastSiram = 0;
unsigned long lastChange = 0;
int nextSiram = 10000;

int timing = 3; // in hours
int runtime = 1; // in seconds

int lastPrint = 0;
int siramTimer = 0;
int inpDebounce = 0;

void onInputLCD() {
  lcd.begin(16,2);
  lcd.setCursor(0, 0);
  if (inputMode == 1) {
    lcd.print("Water timer : ");
  } else {
    lcd.print("Pump on time : ");
  }
  lcd.setCursor(0, 1);
  char buffer[18];
  char hourStr = ' ';
  if (inputMode == 1) {
    if (timing > 1) {
      hourStr = 's';
    }
    sprintf(buffer, "%i hour%c", timing, hourStr);
  } else if (inputMode == 2) {
    if (runtime > 1) {
      hourStr = 's';
    }
    sprintf(buffer, "%i second%c", runtime, hourStr);
  }
  lcd.print(buffer);
}

void displayTimeLCD() {
  lcd.begin(16,2);
  lcd.setCursor(0, 0);
  lcd.print("Next water: ");
  lcd.setCursor(0, 1);
  int remainder = (timerToMs(timing) - siramTimer);
  int hours = remainder / 3600;
  remainder = remainder % 3600;
  int minutes = remainder / 60;
  remainder = remainder % 60;  

  char buffer[10];
  sprintf(buffer, "%02d:%02d:%02d", hours, minutes, remainder);

  lcd.print(buffer);
}

int trackPin(int inputPin) {
  int status = digitalRead(inputPin);
  if (status != pins[inputPin]) {
    pins[inputPin] = status;
    return status;
  }
  return -1;
}

void startSiram() {
  digitalWrite(6, HIGH);
  delay(runtime * 1000);
  digitalWrite(6, LOW);
}

int lastMillis = 0;

void beep() {
  digitalWrite(5, HIGH);
  delay(100);
  digitalWrite(5, LOW);
}

unsigned long interval = 200;
unsigned long lastChangePress = millis();

void onChangePinPress() {
  inpDebounce = 3;
  
  if (millis() - lastChangePress < interval) {
    return;
  }
  lastChangePress = millis();
  lastChange = lastChangePress;
  inputMode = inputMode == 1 ? 2 : 1;
  beep();
}

long long lastAddPress = millis();
void onAddPinPress() {
  inpDebounce = 3;
  if (millis() - lastAddPress < interval) {
    return;
  }
  lastAddPress = millis();
  lastChange = lastAddPress;
  if (inputMode == 1) {
      timing += 1;
      if (timing > 6) {
        timing = 1;
      }
      nextSiram = lastSiram + timerToMs(timing);
    } else if (inputMode == 2) {
      runtime += 1;
      if (runtime > 3) {
        runtime = 1;
      }
    }
    beep();
}

void nothing() {
  lastChange = millis();
}

long overflow = 0;

int addStatus = LOW;
int changeStatus = LOW;
int debounce2 = 500;

void loop() {
  unsigned long currentMillis = millis();

  lastPrint += currentMillis - lastMillis;
  lastMillis = currentMillis;

  int addPin = trackPin(2);
  int changePin = trackPin(3);
  
  if (debounce2 > 0) {
    debounce2--;
  }

  if (digitalRead(2) == HIGH && digitalRead(3) == HIGH) {
    siramTimer = 0;
    startSiram();
    return;
  }

  if (debounce2 > 0) {
    goto skip;
  }

  if (changePin == LOW) {
    onChangePinPress();
    goto updateDisplay;
  }

  if (addPin == LOW) {
    onAddPinPress();
    goto updateDisplay;
  }

  skip:

  if (currentMillis - lastChange < (int)5000 && lastPrint > 1000) {
    lastPrint = 0;
    updateDisplay:
    onInputLCD();
  }

  if (currentMillis - lastChange > (int)5000 && lastPrint > 16) {
    lastPrint = 0;
    delay(25);
    displayTimeLCD();
    delay(25);
    
    overflow++;

    if (overflow >= 20) {
      siramTimer++;
      overflow = 0;
    }

    digitalWrite(LED_BUILTIN, LOW);
    setLCDPinState(INPUT_PULLUP, true);
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_ON);
    digitalWrite(LED_BUILTIN, HIGH);
    setLCDPinState(OUTPUT, false);
    siramTimer += 1;
    
  }

  Serial.println(millis() - lastChange);

  if (siramTimer > timerToMs(timing)) {
    siramTimer = 0;
    startSiram();
  }
}
