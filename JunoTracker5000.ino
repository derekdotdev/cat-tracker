/*
    C Program for Arduino to demonstrate how to collect 'small data' to see how quickly my cat can run.

    This example uses two PIR sensors to track:
  - When my cat's auto feeder begins dispensing a meal
  - When my cat arrives at his bowl to eat (he DARTS to the kitchen as soon as he hears it)

   Finally, these events are timestamped and saved to an SD card as a CSV for further analysis. ----

    ### - The Circuit - ###

    ### Multi-purpose RGB LED ###

      (cathode)     ->  330 Ohm -> GND
      (Red anode)   ->  pin 8 [Sensors Disabled]
      (Green anode) ->  pin 9 [Power ON & Sensors Enabled]
      (Blue anode)  ->  pin 7 [Cat Sensor Indicator]

    ### Food Sensor LED ###
      (cathode)     ->  330 Ohm -> GND
      (anode)       ->  pin 3 [Food Sensor Indicator]

    ### SD Card (built into Arduino Ethernet Shield 2) ###
    SDO -> pin 11
    SDI -> pin 12
    CLK -> pin 13
    CS -> pin 4

    ### DS3231 Real Time Clock (RTC) Module ####
    VCC -> 5V

    Gnd -> Gnd

    SCL -> pin A5

    SDA -> pin A4

    ### FainWan AVR PIC Sound Sensor (foodSensor) ###
    VCC -> 5V
    Gnd -> GND
    Dout -> pin 2

    ### PIR SR602 Sensor (catSensor) ###
    VCC -> 5V
    Gnd -> Gnd
    Signal -> pin 6

    Imagined: 16 May 2022
    Created:  19 May 2022

    by Derek DiLeo

    This code is in the public domain

*/

#include <DS3231.h>        // Library for RTC module (Downloaded from RinkyDink Electronics)
#include <SPI.h>           // Library for SPI communication (Pre-Loaded into Arduino)
#include <SD.h>            // Library for SD card (Pre-Loaded into Arduino)

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

/*****************************************************************************************************
   Setup
    - Open Serial connection
    - Set input/output pins
 *****************************************************************************************************/
void setup() {

  Serial.begin(9600);            // initialize serial (USB) connection

  delay(2000);
  // wait for Serial Monitor to connect. Needed for native USB port boards only:
  while (!Serial);

  Serial.print("Initializing RTC...");

  Initialize_RTC();

  delay(2000);

  Serial.println("Initializing SD card...");

  Initialize_SDcard();

  Serial.println("initialization done.");

  // initialize program started / sensors enabled LED
  pinMode(9, OUTPUT);
  pinMode(8, OUTPUT);            // initialize sensors disabled LED
  digitalWrite(9, HIGH);         // indicate setup() running

  pinMode(foodLED, OUTPUT);      // initalize foodLED as an output
  pinMode(foodSensor, INPUT);    // initialize foodSensor as an input
  pinMode(catLED, OUTPUT);       // initalize catLED as an output
  pinMode(catSensor, INPUT);     // initialize catSensor as an input

  delay(4000);                   // wait 4 seconds
  digitalWrite(9, LOW);          // turn off LED to indicate setup() done

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

  if (feedingWindow()) {
    // Indicate time is valid within 10-minute feeding window (5 min before and after)
    digitalWrite(9, HIGH);  // GREEN ON == Sensors Hot!
    digitalWrite(8, LOW);   // RED OFF

    // check for first sensor trip during this feed window
    if (foodSensorVal == HIGH && !foodDispensed) {
      // disable (RED,GREEN) sensor LEDs so 
      // foodLED (YELLOW) and catLED (BLUE) are more pronounced
      digitalWrite(9, LOW);
      digitalWrite(8, LOW);

      digitalWrite(foodLED, HIGH);              // turn foodLED (YELLOW) ON
      delay(60);                                // delay 60 milliseconds

      if (foodSensorState == LOW) {             // verify new sensor trip event
        Serial.println("Food has started dispensing!");

        foodDispensed = true;

        foodTime = millis();                    // Capture current time (used later for diff)

        foodSensorState = HIGH;                 // update foodSensorState to HIGH
      }
    } else {
      digitalWrite(foodLED, LOW);               // turn foodLED (YELLOW) OFF

      delay(60);                                // delay 60 milliseconds

      if (foodSensorState == HIGH) {
        foodSensorState = LOW;                  // update foodSensorState to LOW
      }
    }

    // check catSensor trip after food has begun dispensing
    if (catSensorVal == HIGH && foodDispensed) {
      digitalWrite(catLED, HIGH);               // turn catLED (BLUE) ON

      delay(60);                                // delay 60 milliseconds

      if (catSensorState == LOW) {              // verify new sensor trip event

        Write_SDcard((millis() - foodTime));    // Write split time SD

        catSensorState = HIGH;                  // update catSensorState to HIGH
      }
    } else {
      digitalWrite(catLED, LOW);                // turn catLED (BLUE) OFF

      delay(60);                                // delay 60 milliseconds

      if (catSensorState == HIGH) {
        catSensorState = LOW;                   // update catSensorState to LOW

        // reset circuit for next feeding window
        foodDispensed = false;
        foodSensorVal = LOW;
        foodSensorState = LOW;                  

        // Set RED LED to HIGH to indicate sensors disabled
        // This may be redundant due to else statement code on line 208
        digitalWrite(8, HIGH);                  

        // Pause program for 30 minutes to avoid multiple SD writes during this feed window
        delay(1800000);

      }
      
    }

  } else {
    digitalWrite(9, LOW);   // GREEN OFF
    digitalWrite(8, HIGH);  // RED ON == Sensors Blocking

  }

}

/*****************************************************************************************************
   timeIsValid
    - This function is used to prevent false data entry by checking current time against
        set 'feeding window' times. His food is dispensed at 6:15 & 18:15, so sensors
        are only watched for five minutes prior and after each (6:10-6:20, 18:10-18:20)
    - Reasons: 
      - Juno likes to lick his bowl when he's hungry (all the time)
      - Food bowl is near the Roomba dock and, despite numerous attempts to program it,
          that thing has a mind of its own!
 *****************************************************************************************************/
bool feedingWindow() {

  int hourNow = rtc.getTime().hour;         // Get current hour
  int minuteNow = rtc.getTime().min;        // Get current minute

  if ((hourNow == 6  || hourNow == 18) && (minuteNow >= 10 && minuteNow <= 20)) {

    return true;

  } else {

    return false;

  }

}

/*****************************************************************************************************
   Initialize_RTC
    - Initialize the RTC object
    - Set date, if necessary
 *****************************************************************************************************/
void Initialize_RTC() {

  rtc.begin();           // Initialize the rtc object

  //#### The following lines can be [un]commented to set the current date and time ###

  rtc.setTime(16, 30, 15);     // Set the time (HH:mm:ss) (24hr format)

  Serial.print("     Time read from rtc: ");
  Serial.print(rtc.getTimeStr());

  rtc.setDate(26, 05, 2022);   // Set the date to May 26th, 2022

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

  // Check if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {

    Serial.println("Card failed, or not present");

    Serial.println("1. is a card inserted?");

    Serial.println("2. is your wiring correct?");

    Serial.println("3. did you change the chipSelect pin to match your shield or module?");

    Serial.println("Note: press reset or reopen this serial monitor after fixing your issue!");

    while (true);

  }

  // Open the file. (note: only one file can be open at a time,

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
    - timeDiff is difference (in ms) between foodSensor and catSensor trips
    - Open the file on SD (if available)
    - Write date,currentTime & splitTime to CSV
 *****************************************************************************************************/
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
