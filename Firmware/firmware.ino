
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

// Doku: https://github.com/adafruit/Adafruit_SSD1306
#include <Adafruit_SSD1306.h>
//Doku: https://github.com/adafruit/Adafruit_MCP4725
#include <Adafruit_MCP4725.h>

#include <Adafruit_INA219.h>


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

// FAN regulation tries to hit that temperature
#define HEATSINKTARGETTEMPERATURE 40.0

Adafruit_MCP4725 dac;
Adafruit_INA219 ina219;


// different GLCD MENUs
typedef enum MENUS_
{
  HOME,
  DEBUG
} MENUS;


typedef struct displayData_
{
  unsigned int adcHeatSinkTemperature;
  double heatSinkTemperature;
  unsigned int adcExternalTemperature;

  int fanSpeed;
  unsigned int dacValue;
  unsigned int milliAmpsStepPerTick;
  float dacVoltage;
  float setCurrent;
  float inputVoltage;
  float realCurrent;
  float shuntVoltage;
  bool encoderPress;
  bool encoderUp;
  bool encoderDown;
  
} displayData;


static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

void printStringOnGLCD(int x, int y, String s) {

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(x*5, y * 8);     // Start at top-left corner
  display.cp437(false);         // Use full 256 char 'Code Page 437' font


  for (unsigned int i = 0; i < s.length(); i++)
  {
    display.write(s[i]);
  }

}

displayData dpData; 

void setCurrentInMilliamps(unsigned int milliAmps)
{
  dpData.dacValue = (unsigned int) ((float)milliAmps / 685e-3);
  dpData.setCurrent = milliAmps;
}

void printDataOnSerial()
{
  Serial.println(String(dpData.heatSinkTemperature) + String("\t") + String(dpData.fanSpeed/2.55) + String("\t") + String(dpData.inputVoltage) + String("\t") + String(dpData.realCurrent));

}

void PIHeatSinkRegulator(double error)
{
  static double iValue = 0;
  double ki = 0.05;

  static int oldFanValue=0;

  if ((iValue < (255 * (1/ki) * 1.02)) || (error < 0))
  {
    iValue = iValue + error;
  }

  if (iValue < 0)
  {
    iValue = 0;
  }

  double iValueKi = ki * iValue;

  /**
  if (iValueKi > (255 * ki))
  {
    iValueKi = 255;
    iValue = 255 * ki;
  }
  */

  
  dpData.fanSpeed = 10 * error + iValueKi;

  if ((dpData.heatSinkTemperature > HEATSINKTARGETTEMPERATURE) && (dpData.fanSpeed == 0))
  {
    // kick fan in
    iValue = (255 * (1/ki) * 1.02);
  }

  oldFanValue = dpData.fanSpeed;


  if (dpData.fanSpeed > 255)
  {
    dpData.fanSpeed=255;
  }

  if (dpData.fanSpeed < 50)
  {
    dpData.fanSpeed = 0;
  }

}

String addCharToString(String input, int size, char c)
{

  String append = "";
  if (input.length() >= size)
  {
    return input;
  } else {
    for (unsigned int i = input.length(); i < size; i++)
    {
      append = append + String(c);
    }
    return String(append + input); 
  }

}

uint8_t lastEncoderState = LOW;

void setup() {
  Serial.begin(115200);


  //setting default values
  dpData.fanSpeed = 0;

  // PWM FAN Output Speed
  pinMode(5,OUTPUT);

  // Encoder inputs
  pinMode(2,INPUT_PULLUP); //Press
  pinMode(3,INPUT_PULLUP); // DT
  pinMode(4,INPUT_PULLUP); // CLK
  lastEncoderState = digitalRead(4);

  dpData.milliAmpsStepPerTick = 1;

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  
  // Clear the buffer
  display.clearDisplay();

  // dac
  dac.begin(0x60);
  dac.setVoltage(0,false);

  // Draw a single pixel in white
  //display.drawPixel(10, 10, WHITE);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  //display.display();
  //delay(2000);
  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). These examples demonstrate both approaches...

  //testdrawline();      // Draw many lines

  //testdrawrect();      // Draw rectangles (outlines)

  //testfillrect();      // Draw rectangles (filled)

  //testdrawcircle();    // Draw circles (outlines)

  //testfillcircle();    // Draw circles (filled)

  //testdrawroundrect(); // Draw rounded rectangles (outlines)

  //testfillroundrect(); // Draw rounded rectangles (filled)

  //testdrawtriangle();  // Draw triangles (outlines)

  //testfilltriangle();  // Draw triangles (filled)

  //testdrawchar();      // Draw characters of the default font

  //testdrawstyles();    // Draw 'stylized' characters

  //testscrolltext();    // Draw scrolling text

  //testdrawbitmap();    // Draw a small bitmap image

  // Invert and restore display, pausing in-between
  //display.invertDisplay(true);
  //delay(1000);
  //display.invertDisplay(false);
  //delay(1000);

  //testanimate(logo_bmp, LOGO_WIDTH, LOGO_HEIGHT); // Animate bitmaps
  ina219.begin();

}



MENUS activeMenu = HOME;

unsigned long tickMillis = millis();
void loop() {

  switch(activeMenu)
  {

    case DEBUG:
      // HOME Screen
      display.clearDisplay();
      printStringOnGLCD(0,0,String("heatsink: " + String(dpData.heatSinkTemperature) + String(" (") + String(dpData.adcHeatSinkTemperature) + String(")")));
      //printStringOnGLCD(0,1,String("external " + String(dpData.inputVoltage)));
      printStringOnGLCD(0,1,String("fanspeed " + String((int)dpData.fanSpeed)));

      printStringOnGLCD(0,2,String("ina " + String(dpData.realCurrent) + String(" ") + String(dpData.shuntVoltage)));
      printStringOnGLCD(0,3,String("dac " + String(dpData.dacValue) + String(" ") + String(dpData.dacVoltage)+ String(" ") + String(dpData.setCurrent)));

      display.display();
      break;

    case HOME:
      // HOME Screen
      display.clearDisplay();
      String currentString = addCharToString(String((int)dpData.realCurrent),5,' ');
      String setcurrentString = addCharToString(String((int)dpData.setCurrent),5,' ');
      String voltageString = addCharToString(String(dpData.inputVoltage),11,' ');
      String powerString = addCharToString(String(dpData.inputVoltage * dpData.realCurrent/1000.00),11,' ');
      String heatSinkTempString = addCharToString(String(dpData.heatSinkTemperature),6,' ');
      String fanSpeedPercentString = addCharToString(String((int)(dpData.fanSpeed/2.55)),3,' ');
      printStringOnGLCD(0,0,String(currentString + String("mA") + voltageString+ String("V")));
      printStringOnGLCD(0,1,String(setcurrentString+ String("mA") + powerString + String("W")));
      printStringOnGLCD(0,2,String(String("+"+addCharToString(String(dpData.milliAmpsStepPerTick),4,' ')+ String("mA"))));
      printStringOnGLCD(0,3,String(heatSinkTempString+ String("C") + String(" Speed: ") + fanSpeedPercentString + String("%")));
      display.display();
      break;

    default:
      activeMenu = HOME;
      break;
  }

  // every 100ms we update values
  if ((millis() - tickMillis) > 100)
  {
    //100ms timebase
    tickMillis = millis();

    //set fan speed
    analogWrite(5,(int)dpData.fanSpeed);
    

    // heat sink temperature
    dpData.adcHeatSinkTemperature= analogRead(6);
    dpData.heatSinkTemperature = (838 - dpData.adcHeatSinkTemperature) * 0.11;

    // P regulation
    float error = dpData.heatSinkTemperature - HEATSINKTARGETTEMPERATURE;
    PIHeatSinkRegulator(error);

    /*
    if (error > 0) 
    {

      if (dpData.fanSpeed == 0)
      {
        // short pulse
        dpData.fanSpeed=255;
        analogWrite(5,dpData.fanSpeed);
        delay(1000);
      }


    } else {
      //dpData.fanSpeed = 0;
    }
    */


    // set dac
    //dpData.dacValue = 1000;
    dac.setVoltage(dpData.dacValue,false);
    dpData.dacVoltage = dpData.dacValue * (5.057/4096);
    //dpData.setCurrent = (dpData.dacVoltage * (8.2/108.2) ) / 0.1; 

    // readback from INA219 
    dpData.realCurrent = ina219.getCurrent_mA();
    dpData.shuntVoltage = ina219.getShuntVoltage_mV();

    // input voltage
    dpData.inputVoltage = analogRead(2) * (5.057/1024) * 5.44;

    

    printDataOnSerial();

  }


  // Encoder handling
  // Push Button
  dpData.encoderPress = false;
  if (digitalRead(2) == LOW)
  {
    while(digitalRead(2) == LOW);
    dpData.encoderPress = true;
  }
  // CLK
  dpData.encoderUp = false;
  dpData.encoderDown = false;

  uint16_t debounceValue = 0;
  uint8_t encoderClkState = digitalRead(4);

  uint8_t encoderDtState = digitalRead(3);
  if (digitalRead(4) == LOW)
  {
    while(digitalRead(4) == LOW)
    {
      debounceValue++;
      if (debounceValue >= 5000)
      {
        encoderClkState = LOW;
        encoderDtState = digitalRead(3);
      }
    }
  } else if (digitalRead(4) == HIGH)
  {
    while(digitalRead(4) == HIGH)
    {
      debounceValue++;
      if (debounceValue >= 5000)
      {
        encoderClkState = HIGH;
        encoderDtState = digitalRead(3);
      }
    }
  }



  // on each change of CLK of Encoder
  if ((encoderClkState != lastEncoderState))
  {
    if (encoderDtState == !lastEncoderState)
    {
      dpData.encoderUp = true;
    } else {
      dpData.encoderDown = true;
    }

    lastEncoderState = encoderClkState;

  }

  if (dpData.encoderPress)
  {
    dpData.milliAmpsStepPerTick += 10;
    if (dpData.milliAmpsStepPerTick == 11)
    {
      dpData.milliAmpsStepPerTick = 10;
    }
    if (dpData.milliAmpsStepPerTick > 100)
    {
      dpData.milliAmpsStepPerTick = 1;
    }
  }


  if (dpData.encoderUp)
  {
    setCurrentInMilliamps(dpData.setCurrent + dpData.milliAmpsStepPerTick);
  }

  if (dpData.encoderDown)
  {
    setCurrentInMilliamps(dpData.setCurrent - dpData.milliAmpsStepPerTick);
  }
  dpData.adcExternalTemperature = digitalRead(4);
  

}

void testdrawline() {
  int16_t i;

  display.clearDisplay(); // Clear display buffer

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, 4);
    display.display(); // Update screen with each newly-drawn line
    delay(1);
  }
  for(i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, display.height()-1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(0, display.height()-1, display.width()-1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=display.width()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, 0, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.height(); i+=4) {
    display.drawLine(display.width()-1, 0, 0, i, WHITE);
    display.display();
    delay(1);
  }
  for(i=0; i<display.width(); i+=4) {
    display.drawLine(display.width()-1, 0, i, display.height()-1, WHITE);
    display.display();
    delay(1);
  }

  delay(2000); // Pause for 2 seconds
}

void testdrawrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, WHITE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}

void testfillrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=3) {
    // The INVERSE color is used so rectangles alternate white/black
    display.fillRect(i, i, display.width()-i*2, display.height()-i*2, INVERSE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}

void testdrawcircle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=2) {
    display.drawCircle(display.width()/2, display.height()/2, i, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfillroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    // The INVERSE color is used so round-rects alternate white/black
    display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawtriangle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=5) {
    display.drawTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfilltriangle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=5) {
    // The INVERSE color is used so triangles alternate white/black
    display.fillTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawchar(void) {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  for(int16_t i=0; i<256; i++) {
    if(i == '\n') display.write(' ');
    else          display.write(i);
  }

  display.display();
  delay(2000);
}

void testdrawstyles(void) {
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Hello, world!"));

  display.setTextColor(BLACK, WHITE); // Draw 'inverse' text
  display.println(3.141592);

  display.setTextSize(2);             // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.print(F("0x")); display.println(0xDEADBEEF, HEX);

  display.display();
  delay(2000);
}

void testscrolltext(void) {
  display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println(F("scroll"));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}

void testdrawbitmap(void) {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
}

#define XPOS   0 // Indexes into the 'icons' array in function below
#define YPOS   1
#define DELTAY 2

void testanimate(const uint8_t *bitmap, uint8_t w, uint8_t h) {
  int8_t f, icons[NUMFLAKES][3];

  // Initialize 'snowflake' positions
  for(f=0; f< NUMFLAKES; f++) {
    icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
    icons[f][YPOS]   = -LOGO_HEIGHT;
    icons[f][DELTAY] = random(1, 6);
    Serial.print(F("x: "));
    Serial.print(icons[f][XPOS], DEC);
    Serial.print(F(" y: "));
    Serial.print(icons[f][YPOS], DEC);
    Serial.print(F(" dy: "));
    Serial.println(icons[f][DELTAY], DEC);
  }

  for(;;) { // Loop forever...
    display.clearDisplay(); // Clear the display buffer

    // Draw each snowflake:
    for(f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, WHITE);
    }

    display.display(); // Show the display buffer on the screen
    delay(200);        // Pause for 1/10 second

    // Then update coordinates of each flake...
    for(f=0; f< NUMFLAKES; f++) {
      icons[f][YPOS] += icons[f][DELTAY];
      // If snowflake is off the bottom of the screen...
      if (icons[f][YPOS] >= display.height()) {
        // Reinitialize to a random position, just off the top
        icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
        icons[f][YPOS]   = -LOGO_HEIGHT;
        icons[f][DELTAY] = random(1, 6);
      }
    }
  }
}