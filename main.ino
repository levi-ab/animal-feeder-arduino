#include <LiquidCrystal.h>
#include <Stepper.h>

// LCD Configuration
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Analog Keypad Definitions
#define RIGHT_KEY 60
#define UP_KEY 250
#define DOWN_KEY 450
#define LEFT_KEY 450
#define SELECT_KEY 850

const int keypadPin = A0;//test 

// Stepper Motor Configuration
const int STEPS_PER_REVOLUTION = 200;
const int stepper_pin1 = 30;
const int stepper_pin2 = 28;
const int stepper_pin3 = 26;
const int stepper_pin4 = 24;

Stepper myStepper(STEPS_PER_REVOLUTION, 24, 26, 28, 30);

// Meal Schedule Configuration
struct MealSchedule {
  int hour;
  int minute;
  int scoops;
  bool enabled;
};

const int MAX_MEALS = 4;
MealSchedule meals[MAX_MEALS] = {
  { 7, 0, 2, false },
  { 12, 0, 1, false },
  { 18, 0, 2, false },
  { 22, 0, 1, false }
};

// Create custom characters for highlighting
byte highlightChar[8] = {
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};

// UI States
enum Screen {
  MAIN_MENU,
  MAIN_MENU_SELECT,
  MEAL_SETUP,
  SYSTEM_TIME_SETUP,
  TIME_SETUP,
  CONFIRM_SETUP
};

Screen currentScreen = MAIN_MENU;
int selectedMeal = 0;
int selectedSetting = 0;

// Simulated Time Variables
unsigned long startTime = 0;

int userEnteredHour = 22;
int userEnteredMinute = 40;

int simulatedHour = userEnteredHour;
int simulatedMinute = userEnteredMinute;

// Debounce Variables
unsigned long lastPressTime = 0;
const unsigned long DEBOUNCE_DELAY = 200;

void setup() {
  lcd.begin(16, 2);
  lcd.clear();
  Serial.begin(9601);
  myStepper.setSpeed(60);

  // Initialize stepper motor pins
  pinMode(stepper_pin1, OUTPUT);
  pinMode(stepper_pin2, OUTPUT);
  pinMode(stepper_pin3, OUTPUT);
  pinMode(stepper_pin4, OUTPUT);

  // Initialize simulated time
  startTime = millis();
  Serial.println("System initialized.");
}

void loop() {
  unsigned long currentTime = millis();

  updateSimulatedTime();
  checkMealSchedule(currentTime);

  Serial.print("Simulated Time: ");
  Serial.print(simulatedHour < 10 ? "0" : "");
  Serial.print(simulatedHour);
  Serial.print(":");
  Serial.print(simulatedMinute < 10 ? "0" : "");
  Serial.print(simulatedMinute);
  Serial.print(":");
  Serial.println((currentTime / 1000) % 60);  // Log seconds for better granularity

  Serial.print("Simulated Time: ");
  Serial.print(simulatedHour < 10 ? "0" : "");
  Serial.print(simulatedHour);
  Serial.print(":");
  Serial.print(simulatedMinute < 10 ? "0" : "");
  Serial.print(simulatedMinute);
  Serial.print(":");
  Serial.println((currentTime / 1000) % 60); 

  int keyValue = readKeypad();
  if (keyValue != -1 && (currentTime - lastPressTime > DEBOUNCE_DELAY)) {
    processNavigation(keyValue);
    lastPressTime = currentTime;
  }

  updateDisplay();
  delay(100);
}

void updateSimulatedTime() {
  unsigned long elapsedTime = (millis() - startTime) / 1000;      // Elapsed time in seconds
  simulatedHour = (elapsedTime / 3600) % 24 + userEnteredHour;    // Hours part
  simulatedMinute = (elapsedTime / 60) % 60 + userEnteredMinute;  // Minutes part

  // Adjust for hour/minute overflow
  if (simulatedMinute >= 60) {
    simulatedMinute %= 60;
    simulatedHour = (simulatedHour + 1) % 24;
  }
  if (simulatedHour >= 24) {
    simulatedHour %= 24;
  }
}

int readKeypad() {
  int key = analogRead(keypadPin);

  if (key <= RIGHT_KEY) return RIGHT_KEY;
  if (key <= UP_KEY) return UP_KEY;
  if (key <= DOWN_KEY) return DOWN_KEY;
  if (key <= LEFT_KEY) return LEFT_KEY;
  if (key <= SELECT_KEY) return SELECT_KEY;

  return -1;  // No key pressed
}

void processNavigation(int key) {
  switch (currentScreen) {
    case MAIN_MENU:
      if (key == SELECT_KEY) currentScreen = MAIN_MENU_SELECT;
      break;

    case MAIN_MENU_SELECT:
      if (key == UP_KEY || key == DOWN_KEY) {
        Serial.print(key);
        // Toggle between Meals and System Time
        selectedSetting = 1 - selectedSetting;
      } else if (key == SELECT_KEY) {
        // Enter selected menu
        currentScreen = (selectedSetting == 0) ? MEAL_SETUP : SYSTEM_TIME_SETUP;
        selectedSetting = 0;
      } else if (key == LEFT_KEY) {
        // Return to main menu
        currentScreen = MAIN_MENU;
      }
      break;

    case MEAL_SETUP:
      navigateMealSetup(key);
      break;

    case SYSTEM_TIME_SETUP:
      navigateSystemTimeSetup(key);
      break;

    case CONFIRM_SETUP:
      if (key == SELECT_KEY) currentScreen = MAIN_MENU;
      break;
  }
}

void navigateMealSetup(int key) {
  MealSchedule& currentMeal = meals[selectedMeal];

  if (key == RIGHT_KEY) {
    if (selectedMeal == 3) {
      currentScreen = MAIN_MENU;
      selectedSetting = 0;
    }
    selectedMeal = (selectedMeal + 1) % MAX_MEALS;
  } else if (key == SELECT_KEY) {
    selectedSetting = (selectedSetting + 1) % 4;
  } else if (key == UP_KEY) {
    switch (selectedSetting) {
      case 0: currentMeal.enabled = !currentMeal.enabled; break;
      case 1: currentMeal.hour = (currentMeal.hour + 1) % 24; break;
      case 2: currentMeal.minute = (currentMeal.minute + 1) % 60; break;
      case 3: currentMeal.scoops = (currentMeal.scoops + 1) % 6; break;
    }
  } else if (key == DOWN_KEY) {
    switch (selectedSetting) {
      case 0: currentMeal.enabled = !currentMeal.enabled; break;
      case 1: currentMeal.hour = (currentMeal.hour + 23) % 24; break;
      case 2: currentMeal.minute = (currentMeal.minute + 59) % 60; break;
      case 3: currentMeal.scoops = max(currentMeal.scoops - 1, 0); break;
    }
  } else if (key == LEFT_KEY) {
    currentScreen = MAIN_MENU;
  }
}

void navigateSystemTimeSetup(int key) {
  if (key == UP_KEY) {
    if (selectedSetting == 0)
      userEnteredHour = (userEnteredHour + 1) % 24;
    else
      userEnteredMinute = (userEnteredMinute + 1) % 60;
  } else if (key == DOWN_KEY) {
    if (selectedSetting == 0)
      userEnteredHour = (userEnteredHour + 23) % 24;
    else
      userEnteredMinute = (userEnteredMinute + 59) % 60;
  } else if (key == SELECT_KEY) {
    selectedSetting = 1 - selectedSetting;  // Toggle between hour and minute
  } else if (key == RIGHT_KEY) {
    // Reset start time when leaving this screen
    startTime = millis();
    simulatedMinute=userEnteredMinute;
    simulatedHour=userEnteredHour;
    currentScreen = MAIN_MENU;
  }
}

void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);

  switch (currentScreen) {
    case MAIN_MENU:
      lcd.print("Cat Feeder Menu");
      lcd.setCursor(0, 1);
      lcd.print("Press Select >>");
      break;

    case MAIN_MENU_SELECT:
      lcd.print("Select:");
      lcd.setCursor(0, 1);
      lcd.print(selectedSetting == 0 ? "> Meals Setup" : "> System Time");
      break;

    case MEAL_SETUP:
      {
        MealSchedule& currentMeal = meals[selectedMeal];
        lcd.createChar(0, highlightChar);

        // First line: Meal number and time
        lcd.setCursor(0, 0);
        lcd.print("M" + String(selectedMeal + 1) + " ");

        // Highlight hours (selectedSetting == 1)
        if (selectedSetting == 1) {
          lcd.write(byte(0));  // Highlight hour
          lcd.print(String(currentMeal.hour));
          lcd.write(byte(0));
        } else {
          lcd.print(String(currentMeal.hour));
        }

        lcd.print(":");

        // Highlight minutes (selectedSetting == 2)
        if (selectedSetting == 2) {
          lcd.write(byte(0));  // Highlight minute
          lcd.print((currentMeal.minute < 10 ? "0" : "") + String(currentMeal.minute));
          lcd.write(byte(0));
        } else {
          lcd.print((currentMeal.minute < 10 ? "0" : "") + String(currentMeal.minute));
        }

        lcd.setCursor(11, 0);  // Top-right corner
        lcd.print(String(simulatedHour) + ":" + String(simulatedMinute));

        // Second line: Enabled status and scoops
        lcd.setCursor(0, 1);

        // Highlight ON/OFF (selectedSetting == 0)
        if (selectedSetting == 0) {
          lcd.write(byte(0));  // Highlight ON/OFF
          lcd.print(currentMeal.enabled ? "ON" : "OFF");
          lcd.write(byte(0));
        } else {
          lcd.print(currentMeal.enabled ? "ON" : "OFF");
        }

        lcd.print(" Sc:");

        if (selectedSetting == 3) {
          lcd.write(byte(0));  // Highlight scoops
          lcd.print(String(currentMeal.scoops));
          lcd.write(byte(0));
        } else {
          lcd.print(String(currentMeal.scoops));
        }
        break;
      }


    case SYSTEM_TIME_SETUP:
      lcd.print("Set Time:" + String(userEnteredHour) + ":" + (userEnteredMinute < 10 ? "0" : "") + String(userEnteredMinute));
      lcd.setCursor(0, 1);
      lcd.print(selectedSetting == 0 ? "Hour" : "Minute");
      break;

    case CONFIRM_SETUP:
      lcd.print("Settings Saved!");
      lcd.setCursor(0, 1);
      lcd.print("Press to exit");
      break;
  }
}

void checkMealSchedule(unsigned long currentTime) {
  for (int i = 0; i < MAX_MEALS; i++) { //its 1 am and honestly idk why do i have to check for -1 and 59, todo
    if (meals[i].enabled && simulatedHour == meals[i].hour && simulatedMinute == meals[i].minute - 1 && (currentTime / 1000) % 60 == 59) {  //check for seconds to run only once
      dispenseMeal(meals[i].scoops);
    }
  }
}

void spinStepper(int revolutions, bool clockwise) {
  int steps = STEPS_PER_REVOLUTION * revolutions;

  if (clockwise) {
    Serial.println("Spinning clockwise");
  } else {
    Serial.println("Spinning counterclockwise");
    steps = -steps;  // Reverse the direction
  }

  myStepper.step(steps);
}

void dispenseMeal(int scoops) {
  for (int s = 0; s < scoops; s++) {
    spinStepper(5, true);  //kinda spins one time
    delay(500);            // Pause between scoops
  }
}
