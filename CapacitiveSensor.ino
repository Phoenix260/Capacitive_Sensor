/* This sketch is created for use of a GSM board with an arduino. The sensors utilized are 1x DHT11 + 2BOCHEN 3386 Potentiometer + 5k resistor +
5V solar Array + 2 aluminum foils connected to 10k OHM resistors. 
Copyright: Bobby Lumia, may be used and modified for personal use only. No resale.
Please reference my project blog: http://bobbobblogs.blogspot.ca/ if useing this sketch in your work.
*/

// Include Standard Library
#include <stdlib.h>

#include <CapacitiveSensor.h> // Library Needed for our capacitance readings
#include <dht11.h> // Library Needed for the tempeature readings

#include <Wire.h> // all 3 are for the LCD
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 (SCL) and 5 (SDA) so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

byte none[8] = {
  B00000,
  B01101,
  B10010,
  B10101,
  B01001,
  B10110,
  B00000,
};

dht11 DHT11; // Declare our sensor, I bid thee dht11 ... the first

//============================= \/ DEFINE PINS \/ ==============

//Do Not use Pins A4 & A5 ... reserved for LCD
//Do not use Pins 0 & 1 (digital pins) these are used for GSM data transmission

int LED_1 = 12;                 // BABY Led
int LED_2 = 13;                 // DAD Led

int sol = A3;         // analogue pin
#define DHT11PIN 2   // temperature pin
  
CapacitiveSensor   cs_DAD = CapacitiveSensor(7,6);    // 10M resistor between pins 7 & 6, pin 6 is sensor pin, add a wire and or foil if desired
CapacitiveSensor   cs_BABY = CapacitiveSensor(9,8);    // 10M resistor between pins 9 & 8, pin 8 is sensor pin, add a wire and or foil if desired

//============================== \/ THRESHOLD VALUES \/ ===============

int HIGH_Thresh = 750;             // Threshold to trigger Capacitace 1 HIGH
int LOW_Thresh = 600;              // Threshold to trigger Capacitace 1 LOW
int HIGH_Thresh_2 = HIGH_Thresh;   // Threshold to trigger Capacitace 2 HIGH
int LOW_Thresh_2 = LOW_Thresh;     // Threshold to trigger Capacitace 2 LOW
int Solar_Thresh = 3;              // Solar threshold in volts (anything above 3 volts of solar power)
int Temp_Thresh = 38;              // Temperature threshold in Celcius

//============================= \/ DEFINE VARIABLES \/ ==============

float Time_until_DAD_notified = 120;    // Time in seconds to wait before sending text to dad that he forgot baby (default 120 = 2 min)
float Interval_keep_notify = 300;       // Send him text every so many seconds (default 300 = 5 min)
int Sun_Power = 15;                      // Ratio of degrees rise in temp compared to voltage output (Sun_Power = 5 --> SolVolt = 5 --> sun_power/SolVolt = GIVES oC rise per min = 1 oC/min ... if sol Volt = 3 --> GIVES oC rise per min = 0.6 oC/min
int skip = 0;                           // skip = 1 goes to menu and makes all the buttons more responsiv
int menu = 0;

int const SAMPLES = 5;                         // number of samples taken to average out capacitance
int s_val[SAMPLES];                            //array to store the 5 samples to average
int const Initial_SAMPLES = 20;                // number of samples to initilize capacitance with
float RAW_initializer_VALS[Initial_SAMPLES];   // Array to hold initial Capacitance Values

int i;              // loop counter   
int triggered = 0;  // Baby text trigger ... don't want to send out too many texts

long start;                   // Start the timer for performance monitoring
float Where_DAD;              // Start the Where's DAD counter
float Where_DAD_interval;     // Start the Where's DAD counter for the interval
float Cap_1;                  // Capacitance Sensor - 1
float Cap_2;                  // Capacitance Sensor - 2
float SolVolt;                // Solar Voltage value (0-5 Volt)
float Amb_Temp;               // Ambient temperature Variable
int chk;                      // Checks the Temperature sensor status and possible faults
float time_to_crit;             // Variable to store the estimated time left until vehicle reaches critical temp

char str1[]={'A','T','+','C','S','C','S','=','"','G','S','M','"'};  // AT+CSCS="GSM"  -- This sets:  GSM default alphabet
char str2[]={'A','T','+','C','M','G','S','=','"','1','7','7','8','8','8','2','1','8','6','7','"'};  // AT+CMGS="17788821867"  -- This sets Phone number to Text
char hex1[]={0x1A};  // Stop Character used to signify the end of a Text Msg

void setup()                    
{
   Serial.begin(115200);
   lcd.begin(16, 2);
   lcd.setBacklight(WHITE);
   lcd.setCursor(0, 1);
   lcd.createChar(0, none);
   
   pinMode(LED_1, OUTPUT);           // set pin to output
   pinMode(LED_2, OUTPUT);           // set pin to output
   pinMode(LED_1+1, OUTPUT);           // set pin to output
   pinMode(LED_2+1, OUTPUT);           // set pin to output
   
   Serial.println();
   Serial.println("Starting GSM Communication..."); //Just an FYI ... not really chacking anything
   lcd.print("Starting GSM ...");
   Serial.println();
}

void loop()                    
{  
  uint8_t buttons = lcd.readButtons(); //declare LCD buttons
  
    if (buttons) {
      lcd.clear();
      lcd.setCursor(0,0);
      if (buttons & BUTTON_UP) {
      }
      if (buttons & BUTTON_DOWN) {
      }
      if (buttons & BUTTON_LEFT) {
      }
      if (buttons & BUTTON_RIGHT) { 
      }
      if (buttons & BUTTON_SELECT) {
        skip = 1;
        lcd.print("MENU Loading");
        delay(500);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("select option");
        lcd.setCursor(0,1);
        lcd.print("1) change thresh");
        Menu();
      }
    }
  
    if (skip != 1){
      ReadSensors();               // All sensor readings grouped in a function
      Print_Sensor_Vals_Serial();  // Now print these values on the screen for all to see!
      
      if (Cap_2 > HIGH_Thresh_2) LED_1_ON();
      else LED_1_OFF();
      
      if (Cap_1 > HIGH_Thresh) LED_2_ON();
      else LED_2_OFF();
      
      //delay(150); // no use reading less than half a second ... it just confuses the sensors (mostly temp)
      
      if (Cap_2 > HIGH_Thresh_2){ // If Baby is in seat
        if (Cap_1 < LOW_Thresh) { // If daddy in not there
           if (triggered == 0){
             Where_DAD = millis(); // DAD, your timer has begin
             triggered = 1; // Level 1 trigger
             Serial.println();Serial.println();Serial.println("Countdown Begins! Where are you dad?");Serial.println();
           }
           
           if ((millis() - Where_DAD > (Time_until_DAD_notified * 1000.0)) && triggered == 1){ // If daddy is still not there after 120 seconds
             Serial.println();
             SendTXT();  // Send dad a text to tell him that something is wrong
             triggered = 2; // Level 2 trigger
             Serial.println();Serial.println();Serial.println("Baby on Board! Where are you dad? THIS IS AN ALARM");Serial.println();
           }
        }
        
        if (triggered == 1){
           Serial.print((millis() - Where_DAD)/1000.0,1);Serial.print(" Seconds .. ");Serial.println();
           delay(100);
           LED_1_OFF();
         }
         
         if ((millis() - Where_DAD > (Time_until_DAD_notified * 1000.0)) && triggered == 1){ // If daddy is still not there after 120 seconds
           Serial.println();
           SendTXT();  // Send dad a text to tell him that something is wrong
           triggered = 2; // Level 2 trigger
           Serial.println();Serial.println();Serial.println("Baby on Board! Where are you dad? THIS IS AN ALARM");Serial.println();
         }
         
      }
      
      if ((Cap_2 < LOW_Thresh_2 && triggered > 0) || (Cap_1 > HIGH_Thresh && triggered > 0)){ // Stop panic if, Baby is removed, or Dad has arrived ... after an even was triggered
        triggered = 0;
        Serial.println();Serial.println();Serial.println("Baby SAFE!");Serial.println();
      }
      
      // Next 3 blocks for second timer
      
      if ((Cap_2 > LOW_Thresh_2 && (triggered == 2)) || (Cap_1 < HIGH_Thresh&& (triggered == 2))){ // Stop panic if, Baby is removed, or Dad has arrived ... after an even was triggered
        Where_DAD_interval = millis();  // DAD, your second timer timer has begin --- Go get your baby
        Serial.println();Serial.println();Serial.println("DAD, your second timer timer has begin --- Go get your baby!");Serial.println();
        triggered = 3;
      }
      
      if (triggered == 3){
           Serial.print((millis() - Where_DAD_interval)/1000.0,1);Serial.print(" Seconds .. ");Serial.println();
           LED_1_OFF();
      }
      
      if ((millis() - Where_DAD_interval > (Interval_keep_notify * 1000.0)) && triggered == 3){ // If daddy is still not there after (default 300) seconds
         Serial.println();
         SendTXT();  // Send dad a text to tell him that something is wrong
         triggered = 2; // Level 2 trigger
         Serial.println();Serial.println();Serial.println("Baby on Board! Where are you dad? THIS IS AN ALARM");Serial.println();
      }
    }
}

void ReadSensors()
{
    start = millis();    
    
    Cap_1 = avg(); // DAD Read capacitor time constant 1
    Cap_2 = avg2(); // BABY Read capacitor time constant 2
    
    SolVolt = analogRead(sol) * (5.0 / 1023.0); // Read output voltage created by the SUN!
    time_to_crit = ((Temp_Thresh - Amb_Temp)*Sun_Power)/SolVolt;
      
    chk = DHT11.read(DHT11PIN);
    Amb_Temp = DHT11.temperature;    
}

void Print_Sensor_Vals_Serial()
{
  if(Cap_1 < HIGH_Thresh) {
    lcd.noDisplay();
    lcd.setBacklight(0);
  }
  if(Cap_1 > HIGH_Thresh) {
    lcd.display();
    lcd.setBacklight(WHITE);
  }
  
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print("Dvr:");
  
    if (Cap_1 > HIGH_Thresh) 
      lcd.print("ON"); 
    else 
      lcd.print("off"); 
      
  //lcd.print(Cap_1,0);
  lcd.print("<Rear:");
  
    if (Cap_2 > HIGH_Thresh_2) 
      lcd.print("ON"); 
    else  
      lcd.print("off");
      
  //lcd.print(Cap_2,0);
  lcd.setCursor(0, 1);
  lcd.print("T:");lcd.print(Amb_Temp,0);lcd.print("oC ");
  lcd.print("T2haz");
  if(SolVolt < 5){
    lcd.write(byte(0));
  }
  else{
    lcd.print(time_to_crit,0);lcd.print("m");
  }
  
  if (triggered != 1 && triggered != 3){
    Serial.print("performance:   ");
    Serial.print(millis() - start);        // check on performance in milliseconds
    Serial.print("\t");Serial.print("\t"); // tab character for debug windown spacing

    Serial.print("C1: ");
    Serial.print(Cap_1);Serial.print("/"); // print sensor output 1 / Threshold
    Serial.print(HIGH_Thresh);
    if (Cap_1 > HIGH_Thresh) Serial.print("*PROXI*");
    else Serial.print("       ");
    Serial.print("\t");                    // tab character for debug windown spacing
    
    Serial.print("C2: ");
    Serial.print(Cap_2);Serial.print("/"); // print sensor output  / Threshold
    Serial.print(HIGH_Thresh_2);
    if (Cap_2 > HIGH_Thresh_2) Serial.print("*PROXI*");
    else Serial.print("       ");
    Serial.print("\t");                    // tab character for debug windown spacing
    
    switch (chk){
      case 0: Serial.print("Temperature: "); Serial.print(Amb_Temp); Serial.print(" oC"); if (Amb_Temp > Temp_Thresh) Serial.println("*TEMP*"); break;
      case -1: Serial.print("Checksum ERR: data was received but may not be correct"); break;
      case -2: Serial.print("Timeout ERR: communication has failed"); break;
      default: Serial.print("Unknown ERR: this is the catch-all bucket ... S.O.L. buddy"); break;
    }

    Serial.print("\t");Serial.print("\t");                    // tab character for debug windown spacing
    Serial.print("Solar Voltage: ");
    Serial.print(SolVolt, 2);
    if (SolVolt > Solar_Thresh) Serial.print("*SUN*");
    Serial.println();
  }
}

float avg()
{
  float sampleSum = 0;
  for(int i = 0; i < SAMPLES; i++) {
    s_val[i] = cs_DAD.capacitiveSensor(15);
    sampleSum += s_val[i];
    delay(1); // set this to whatever you want
  }
  float meanSample = sampleSum/float(SAMPLES);
  return meanSample;
}

float avg2()
{
  float sampleSum = 0;
  for(int i = 0; i < SAMPLES; i++) {
    s_val[i] = cs_BABY.capacitiveSensor(15);
    sampleSum += s_val[i];
    delay(1); // set this to whatever you want
  }
  float meanSample = sampleSum/float(SAMPLES);
  return meanSample;
}

void SendTXT()
{  
      for(i=0;i<13;i++)
        Serial.print(str1[i]);//AT+CSCS="GSM"  ==> It must be done this way due to double quotes
      Serial.println();
      
      for(i=0;i<21;i++)
        Serial.print(str2[i]);//AT+CMGS="17788821867"  ==> It must be done this way due to double quotes
      Serial.println();
      
      Serial.print("Baby may be in danger! Temperature is: ");
      Serial.print(Amb_Temp);Serial.print(" oC. "); 
      Serial.print("With current solar output, Temperature will be critical in: ");
      Serial.print(((Temp_Thresh - Amb_Temp)*Sun_Power)/SolVolt); Serial.print(" minutes");                   // This outputs the estimated time for the vehicle to reach critical temp
      delay(1000);
      Serial.print(hex1);
      delay(1000);
      Serial.println();
}

void LED_1_ON(){
  digitalWrite(LED_1, HIGH);       // turn on
  digitalWrite(LED_1+1, LOW);       // 
}

void LED_1_OFF(){
  digitalWrite(LED_1, LOW);       // turn off
  digitalWrite(LED_1+1, LOW);       // 
}

void LED_2_ON(){
  digitalWrite(LED_2, HIGH);       // turn on
  digitalWrite(LED_2+1, LOW);       // 
}

void LED_2_OFF(){
  digitalWrite(LED_2, LOW);       // turn off
  digitalWrite(LED_2+1, LOW);       // 
}

void Menu(){
  while (skip == 1){
    lcd.display();
    lcd.setBacklight(WHITE);
    
    uint8_t buttons = lcd.readButtons(); //declare LCD buttons
    
     if (buttons) {
        lcd.clear();
        lcd.setCursor(0,0);
        if (buttons & BUTTON_UP) {
          menu = menu - 1;
          if (menu<0) menu=3;
          switch(menu){
            case 0:
              lcd.setCursor(0,0);
              lcd.print("select option");
              lcd.setCursor(0,1);
              lcd.print("1)ChangeSensTrip");
              break;
            case 1:
              lcd.setCursor(0,0);
              lcd.print("select option");
              lcd.setCursor(0,1);
              lcd.print("2)ShowSensorVals");
              break;
            case 2:
              lcd.setCursor(0,0);
              lcd.print("select option");
              lcd.setCursor(0,1);
              lcd.print("3)GetTextMSG");
              break;
            case 3:
              lcd.setCursor(0,0);
              lcd.print("select option");
              lcd.setCursor(0,1);
              lcd.print("4)EXIT");
              break;
          }
        }
        if (buttons & BUTTON_DOWN) {
          menu = menu + 1;
          if (menu>3) menu=0;
          switch(menu){
            case 0:
              lcd.setCursor(0,0);
              lcd.print("select option");
              lcd.setCursor(0,1);
              lcd.print("1)ChangeSensTrip");
              break;
            case 1:
              lcd.setCursor(0,0);
              lcd.print("select option");
              lcd.setCursor(0,1);
              lcd.print("2)ShowSensorVals");
              break;
            case 2:
              lcd.setCursor(0,0);
              lcd.print("select option");
              lcd.setCursor(0,1);
              lcd.print("3)GetTextMSG");
              break;
            case 3:
              lcd.setCursor(0,0);
              lcd.print("select option");
              lcd.setCursor(0,1);
              lcd.print("4)EXIT");
              break;
          }
        }
        if (buttons & BUTTON_LEFT) {
        }
        if (buttons & BUTTON_RIGHT) { 
        }
        if (buttons & BUTTON_SELECT) {
          switch(menu){
            case 0:
              
              break;
            case 1:
              
              break;
            case 2:
              
              break;
            case 3:
              skip = 0;
              menu = 0;
              lcd.print("Returning to"); 
              lcd.setCursor(0,1); 
              lcd.print("reading sensors");
              delay(500);
              break;
            }
         }
      }   
   }
}
