#include <SPI.h>
#include <OneButton.h>
#include <adf4350.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define I2C_ADDRESS 0x3c
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO

#define LED_PIN 2
#define LASER_PIN 15
#define FN_BTN_PIN 16 

#define COM_PIN 26 //32 // sets pin 26 to be the slave-select pin for PLL
#define ADC_PIN 35

ADF4350 PLL(COM_PIN, ADC_PIN); // declares object PLL of type ADF4350. Will initialize it below.
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OneButton fn_btn = OneButton(FN_BTN_PIN, true, true);

int MINVAL_ADC = 1750;
int MAXVAL_ADC = 1500; // moved to adf4350.h
int VAL_DIV = 11;
int CALIBRATION_COUNT = 10;
int CUM_PLOT_SCALE = 1375; // cumulative plot scaling base
int CUM_PLOT_DIV = 3;//1.5; // cumulative plot divider 
int PT_OFFSET = 7;

int prev_val = 0;
int cum_ctr = 0; // cumulative counter


enum AppState { SENSOR, COMPUTER, RNG, MENU };
AppState currentState = SENSOR; // MENU;
int menuIndex = 0;
const char* menuItems[] = { "Quantum sensor", "Quantum computer", "Quantum RNG" };
const char* title = "UncutGem 1.0";
const int menuLength = 3;

void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(2, 2);
  display.println(title);

  for (int i = 0; i < menuLength; i++) {
    display.setCursor(5, 20 + i * 12);
    if (i == menuIndex) {
      display.print("> ");
    } else {
      display.print("  ");
    }
    display.println(menuItems[i]);
  }
  display.display();
}

static void handleClick() {
  Serial.println("Clicked!");

  if(currentState == MENU) {
    menuIndex = (menuIndex + 1) % menuLength;
  }
}

static void handleDoubleClick() {
  Serial.println("Double clicked!");

}

static void handleLongClick() {
  Serial.println("Long clicked!");

  if(currentState == MENU) {
    currentState = static_cast<AppState>(menuIndex);
    Serial.println(currentState);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.print("Starting ");
    display.println(menuItems[menuIndex]);
    display.display();
    delay(1000);
  } else {
    currentState = MENU;
  }
}

void setup(){
  delay(250); // wait for the OLED to power up
  display.begin(I2C_ADDRESS, true); // Address 0x3C default
  pinMode(LED_PIN, OUTPUT);
  pinMode(LASER_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH);
  digitalWrite(LASER_PIN, HIGH);
  
  Serial.begin(115200) ;


  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(2, 2);
  display.println(title);
  display.display();
  Serial.println(title);

  fn_btn.attachClick(handleClick);
  fn_btn.attachDoubleClick(handleDoubleClick);
  fn_btn.attachLongPressStop(handleLongClick);

   while(!Serial){};
  delay(250); // give it a sec to warm up
  Serial.println("PPL initialization...");
  PLL.init(2870, 10);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(1000);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("PPL sending 2870...");
  PLL.send_2870();
  digitalWrite(LED_PIN, LOW);
  delay(2000);
  Serial.println("PPL sending sweep...");
  PLL.send_sweep();
  Serial.println("PPL initialized.");
  calibrate();
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  fn_btn.tick();

  switch (currentState) {
    case MENU:
      drawMenu();
      // delay(3000);
      // display.clearDisplay();
      // currentState = SENSOR;
      break;

    case SENSOR:
      loopSensor();
      break;

    case COMPUTER:
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Quantum computer");
      display.display();
      delay(2000);
      currentState = MENU;
      break;

    case RNG:
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("RNG");
      display.display();
      delay(2000);
      currentState = MENU;
      break;
  }
}


void loopSensor() {
  // digitalWrite(LASER_PIN, HIGH);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(2, 2);
  display.println("Sensing...");

  int cum_avg = 0; // cumulative average
  for(int i = 0; i < 128; i++){
    int ADC_out = PLL.send_sweep_step(i);
    display.drawLine(i, display.height(), i, 25, SH110X_BLACK); // clear the existing line
    int display_val = (((ADC_out - MINVAL_ADC )/ VAL_DIV) + prev_val)/2; // interpolate the previous value
    display.drawLine(i, display.height(), i, display.height() - min(display_val, 40), SH110X_WHITE);
    display.display();
    //Serial.println(display_val);
    cum_avg += ADC_out;
    prev_val = display_val; // using this prev_val gives some interpolation to make the graph clearer
  }
  cum_avg /= 128;
  // display.setTextSize(1);
  // display.setTextColor(SH110X_WHITE);
  // display.setCursor(100, 0);
  // display.println(cum_avg);
  // display.display();

  // do a small 25px cumulative average plot
  if (cum_ctr > 127){ cum_ctr = 0; display.fillRect(0, 0, 128, 64, SH110X_BLACK);}
  float scaled_val = ((cum_avg - (float)CUM_PLOT_SCALE)/(float)CUM_PLOT_DIV);
  int dot_val = scaled_val < 0 ? floor(scaled_val)/2 : scaled_val; // if dot_val is negative, scale it to 'small near zero'
  dot_val = (dot_val - PT_OFFSET) * 1.5;
  display.drawPixel(cum_ctr, dot_val, SH110X_WHITE);
  display.display();
  // Serial.println(scaled_val);
  cum_ctr++;

  // digitalWrite(LASER_PIN, LOW);
  delay(100);
}

void calibrate(){
  // digitalWrite(LASER_PIN, HIGH);
  // warm up the microwave generator and it's gain stage,
  // then characterize it and calibrate the values for scaling for display.
  Serial.println("Calibrating...");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(2, 2);
  display.println("Calibrating...");
  display.display();
  int ctr = 0;
  int cum_avg = 0;
  int max_ca = 0;
  do{ // warm-up run
    for(int i = 0; i < 128; i++){
      int ADC_out = PLL.send_sweep_step(i);
    }
    ctr++;
  }while(ctr < CALIBRATION_COUNT);
  ctr = 0;
  do {
    int cum_avg = 0;
    for(int i = 0; i < 128; i++){
      int ADC_out = PLL.send_sweep_step(i);
      cum_avg += ADC_out;
      if (ADC_out > MAXVAL_ADC){
        MAXVAL_ADC = ADC_out;
      }
      if (ADC_out < MINVAL_ADC && ADC_out > 1100){ 
        // reduce it, but not below 1.1V...
        MINVAL_ADC = ADC_out-25;
      }
    }
    ctr++;
    cum_avg /= 128;
    if (cum_avg < CUM_PLOT_SCALE) {CUM_PLOT_SCALE = cum_avg + 25;}
    if (cum_avg > max_ca) {max_ca = cum_avg;}
  }while(ctr < CALIBRATION_COUNT);
  CUM_PLOT_DIV = ((max_ca - CUM_PLOT_SCALE) / 25) + 2;
  VAL_DIV = (MAXVAL_ADC - MINVAL_ADC)/40; 
  
  // if (VAL_DIV == 0)
  //   VAL_DIV = 1;

  display.clearDisplay();
  // digitalWrite(LASER_PIN, LOW);
}
