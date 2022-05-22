/* 
    C Program for Arduino to demonstrate how to collect 'small data' to see how quickly my cat can run.

    This example uses two PIR sensors to track:
  - When my cat's auto feeder begins dispensing a meal
  - When my cat arrives at his bowl to eat (he DARTS to the kitchen as soon as he hears it)

   Finally, these events are timestamped and saved to an SD card as a CSV for further analysis. ----

    ### - The Circuit - ###

    ### SD Card (built into Arduino Ethernet Shield 2) ###
    SDO -> pin 11

    SDI -> pin 12

    CLK -> pin 13

    CS -> pin 4

    ###Connection with DS3231####
    VCC -> 5V

    Gnd -> Gnd

    SCL -> pin A5

    SDA -> pin A4 (white

    ###Connection with foodSensor PIR (+ LED Indicator)###
    VCC -> 5V
    Gnd -> Gnd
    Signal -> pin 2

    LED(cathode) -> 330Ohm -> GND
    LED(anode) -> pin 3

    ###Connection with catSensor PIR SR602 (+ LED Indicator)###
    VCC -> 5V
    Gnd -> Gnd
    Signal -> pin 6

    LED(cathode) -> 330Ohm -> GND
    LED(anode) -> pin 7

    created 19 May 2022

    by Derek DiLeo

    This code is in the public domain

*/
//#include <Wire.h>        // Library for I2C/TWI devices (SDA<data> = A4, SCL<clk> = A5)

#include <DS3231.h>        // Library for RTC module (Downloaded from RinkyDink Electronics)
#include <SPI.h>           // Library for SPI communication (Pre-Loaded into Arduino)
#include <SD.h>            // Library for SD card (Pre-Loaded into Arduino)
//#include <TimeLib.h>       // Library for DateTime

DS3231  rtc(SDA, SCL);        // Init the DS3231 using the hardware interface

bool foodDispensed = false;    // catSensor only important if this is true

const int foodSensor = 2;            // the pin that the food sensor is attached to
const int foodLED = 3;               // the pin that the foodLED is attached to
const int chipSelect = 4;      // SD card CS pin connected to pin 4 of Arduino
const int catSensor = 6;             // the pin that the cat sensor is attached to
const int catLED = 7;                // the pin that the catLED is attached to
int foodSensorState = LOW;     // by default, no motion detected
int catSensorState = LOW;      // by default, no motion detected
int foodSensorVal = 0;         // variable to store the food sensor status (value)
int catSensorVal = 0;          // variable to store the cat sensor status (value)

// variable to store millis() when food drops and is later
// subracted from catSensor time to determine cat's reaction speed
unsigned long foodTime;        

/*

  During testing, file will be written to when either sensor is tripped. After testing is completed, only
  write to SD when cat arrives. This will make creating the bar graph and calculating average time much easier!

*/


/*****************************************************************************************************
   Setup
    - Open Serial and Wire connection
    - Set input/output pins
    - Explain to the user how to use the program
 *****************************************************************************************************/
void setup() {

  Serial.begin(9600);            // initialize serial (USB) connection
  
  delay(2000);                 
  // wait for Serial Monitor to connect. Needed for native USB port boards only:
//  while (!Serial);

  Serial.print("Initializing RTC...");
  
  Initialize_RTC();

  delay(2000);

  Serial.println("Initializing SD card...");

  Initialize_SDcard();

  Serial.println("initialization done.");

  pinMode(foodLED, OUTPUT);      // initalize foodLED as an output
  pinMode(foodSensor, INPUT);    // initialize foodSensor as an input
  pinMode(catLED, OUTPUT);       // initalize catLED as an output
  pinMode(catSensor, INPUT);     // initialize catSensor as an input

  Serial.println("Calling timeIsValid() from setup()");
  timeIsValid();

}

/*****************************************************************************************************
   Loop
    - Check sensors for movement
    - When foodSensor is tripped, write event with timestamp to SD
    - If catSensor is tripped after foodSensor, write event with timestamp to SD
 *****************************************************************************************************/

// TODO: Figure out how to avoid false and repeat sensor trips. 
// We only want ONE trigger per meal and do not want Roomba or Juno licking bowl(yes, he does that) to 
// trigger a false sensor event. (timer?)
// TODO: Set up and test with actual food dispenses
void loop() {

  foodSensorVal = digitalRead(foodSensor);   // read foodSensor value
  catSensorVal = digitalRead(catSensor);     // read catSensor value

  if (foodSensorVal == HIGH && !foodDispensed) {   // check food being dispensed for first time
    digitalWrite(foodLED, HIGH);             // turn foodLED ON
    delay(100);                              // delay 100 milliseconds

    if (foodSensorState == LOW) {
//      Serial.println("Motion Detected on Food Sensor!");
      
      foodDispensed = true;
      
      foodTime = millis();                  // Capture current time (used later for diff)
      
      foodSensorState = HIGH;                // update foodSensorState to HIGH
    }
  } else {
    digitalWrite(foodLED, LOW);            // turn foodLED OFF
    delay(100);                            // delay 200 milliseconds

    if (foodSensorState == HIGH) {
//      Serial.println("Food Sensor Motion stopped!");
      foodSensorState = LOW;               // update foodSensorState to LOW
    }
  }

  if (catSensorVal == HIGH
      && foodDispensed) {                  // check catSensor motion and food dispensed
    digitalWrite(catLED, HIGH);            // turn catLED ON
    delay(100);                            // delay 100 milliseconds

    if (catSensorState == LOW) {
      
//      Serial.println("Motion Detected on Cat Sensor after Food Dispensed!");
      
      Write_SDcard((millis() - foodTime));  // Write split time SD 
      
      catSensorState = HIGH;               // update catSensorState to HIGH
    }
  } else {
    digitalWrite(catLED, LOW);             // turn catLED OFF
    delay(100);                            // delay 200 milliseconds

    if (catSensorState == HIGH) {
//      Serial.println("Cat Sensor Motion stopped!");
      catSensorState = LOW;                // update catSensorState to LOW
      foodDispensed = false;               // reset foodDispensed
    }
  }
}

/*****************************************************************************************************
   timeIsValid
    - This function is used to prevent false data entry by checking current time against
        set times when his food is dispensed (6:30 & 18:00)
    - Reasons: Juno likes to lick his bowl when he's hungry (all the time) 
        and his food bowl is near the Roomba dock.
 *****************************************************************************************************/
bool timeIsValid() {

  Time t = rtc.getTime();
  int currentHour = t.hour;
  
  Serial.println("Current time: ");
  Serial.println(t.hour);
 
   if((currentHour > 6 && currentHour < 7) || (currentHour > 18 && currentHour < 19)) {
   Serial.println("Current time == 10!");
    return true;
   } else {
    Serial.println("Current time != 10");
    return false;
   } 
  
}


/*****************************************************************************************************
   Initialize_RTC
    - Initialize the RTC object
    - Set date, if needed
 *****************************************************************************************************/
void Initialize_RTC() {

  rtc.begin();           // Initialize the rtc object

  //#### The following lines can be [un]commented to set the current date and time###
 
  rtc.setTime(12, 30, 45);     // Set the time (HH:mm:ss) (24hr format)

  Serial.print("     Time read from rtc: ");
  Serial.print(rtc.getTimeStr());


  Serial.print("\nSetting date to 5/22/2022: ");
  
  rtc.setDate(22, 05, 2022);   // Set the date to May 5th, 2022

  Serial.print("   Date read from rtc: ");
  Serial.print(rtc.getDateStr());
  Serial.println(" ");
}

/*****************************************************************************************************
   Initialize_SDcard
    - Check if SD card is present and can be initialized (if not, return)
    - Open a new file on the SD card and write first row of excel file
 *****************************************************************************************************/
void Initialize_SDcard() {

  // check if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {

    Serial.println("Card failed, or not present");

    Serial.println("1. is a card inserted?");

    Serial.println("2. is your wiring correct?");

    Serial.println("3. did you change the chipSelect pin to match your shield or module?");

    Serial.println("Note: press reset or reopen this serial monitor after fixing your issue!");

    while (true);

  }

  // open the file. (note: only one file can be open at a time,

  // so you have to close this one before opening another).

  File dataFile = SD.open("LoggerCD.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {

    dataFile.println("Date,Time,Cat Response Time(ms)"); //Write the first row of the excel file

    dataFile.close();

  }

  Serial.println("Initialize_SDcard() complete!");

}

/*****************************************************************************************************
   Write_SDcard
    - whichSensor is used to indicate if writing data about foodSensor or catSensor
    - isFood determines whether '0' or catTime-foodTime (in millis()) is written to CSV
    - Open the file on SD (if available)
    - Write date,whichSensor,currentTime & splitTime to CSV
 *****************************************************************************************************/
// void Write_SDcard(long foodDropTime) {
void Write_SDcard(unsigned long timeDiff) {

  // open the file. note that only one file can be open at a time,

  // so you have to close this one before opening another.

  File dataFile = SD.open("LoggerCD.txt", FILE_WRITE);

  // if the file is available, write to it:

  if (dataFile) {

    Serial.println("Cat sensor caused Write_SDcard call!");  
    
    dataFile.print(rtc.getDateStr()); //Store date on SD card

    dataFile.print(","); //Move to next column using a ","


    dataFile.print(rtc.getTimeStr()); //Store time on SD card

    dataFile.print(","); //Move to next column using a ","


    dataFile.print(String(timeDiff));    // Store food dispense time - cat arrival time to SD card

    dataFile.print(","); //Move to next column using a ","


    dataFile.println(); //End of Row move to next row

    dataFile.close(); //Close the file

  }

  else

    Serial.println("OOPS!! SD card writing failed...\nFile not available!");

}
