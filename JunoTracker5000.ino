/*
    C Program for Arduino to demonstrate how to collect 'small data' to see how quickly my cat can run.

    This example uses two PIR sensors to track:
  - When my cat's auto feeder begins dispensing
  - When my cat arrives at his bowl to eat (he DARTS to the kitchen as soon as he hears it)

   Finally, these events are timestamped and saved to an SD card as a CSV for further analysis. ----

   ### Connection with SD card module###

   MISO -> pin 12

   MOSI -> pin 11

   SCK -> pin 13

   CS ->pin 4

    ###Connection with DS3231####
    VCC -> 5V

    Gnd -> Gnd

    SCL -> pin A5

    SDA -> pin A4

     ###Connection with foodSensor PIR###



     ###Connection with catSensor PIR###


*/

#include <DS3231.h>  // Library for RTC module (Downloaded from RinkyDink Electronics)

#include <Wire.h>    // Library for I2C/TWI devices 

#include <SPI.h>     //Library for SPI communication (Pre-Loaded into Arduino)

#include <SD.h> // Library for SD card (Pre-Loaded into Arduino)


DS3231  rtc(SDA, SCL);        // Init the DS3231 using the hardware interface

bool foodDispensed = false;    // catSensor only important if this is true
int foodLED = 3;               // the pin that the foodLED is attached to
int catLED = 7;                // the pin that the catLED is attached to
int foodSensor = 2;            // the pin that the food sensor is attached to
int catSensor = 6;             // the pin that the cat sensor is attached to
int foodSensorState = LOW;     // by default, no motion detected
int catSensorState = LOW;      // by default, no motion detected
int foodSensorVal = 0;         // variable to store the food sensor status (value)
int catSensorVal = 0;          // variable to store the cat sensor status (value)

const int chipSelect = 4;      // SD card CS pin connected to pin 4 of Arduino

// Initialize time variables for SD card writes
Time foodTimeSD, catTimeSD, differenceTimeSD;

// Initialize long to keep track of milliseconds (for more precise splits)
unsigned long foodTime;


/*****************************************************************************************************
   Setup
    - Open Serial and Wire connection
    - Set input/output pins
    - Explain to the user how to use the program
 *****************************************************************************************************/
void setup() {
  Serial.begin(57600);            // initialize serial (USB) connection

  Wire.begin();                  // Start the I2C interface

  Initialize_SDcard();

  Initialize_RTC();

  pinMode(foodLED, OUTPUT);      // initalize foodLED as an output
  pinMode(foodSensor, INPUT);    // initialize foodSensor as an input
  pinMode(catLED, OUTPUT);       // initalize catLED as an output
  pinMode(catSensor, INPUT);     // initialize catSensor as an input

}

/*****************************************************************************************************
   Loop
    - Check sensors for movement
    - When foodSensor is tripped, write event with timestamp to SD
    - If catSensor is tripped after foodSensor, write event with timestamp to SD
 *****************************************************************************************************/
void loop() {

  foodSensorVal = digitalRead(foodSensor);   // read foodSensor value
  catSensorVal = digitalRead(catSensor);     // read catSensor value

  if (foodSensorVal == HIGH && !foodDispensed) {   // check food being dispensed for first time
    digitalWrite(foodLED, HIGH);             // turn foodLED ON
    delay(100);                              // delay 100 milliseconds

    if (foodSensorState == LOW) {
      Serial.println("Motion Detected on Food Sensor!");
      foodDispensed = true;
      Write_SDcard("Food Sensor", true);
      foodSensorState = HIGH;                // update foodSensorState to HIGH
    }
  }
  else {
    digitalWrite(foodLED, LOW);            // turn foodLED OFF
    delay(200);                            // delay 200 milliseconds

    if (foodSensorState == HIGH) {
      Serial.println("Food Sensor Motion stopped!");
      foodSensorState = LOW;               // update foodSensorState to LOW
    }
  }

  if (catSensorVal == HIGH
      && foodDispensed) {                  // check catSensor motion and food dispensed
    digitalWrite(catLED, HIGH);            // turn catLED ON
    delay(100);                            // delay 100 milliseconds

    if (catSensorState == LOW) {
      Serial.println("Motion Detected on Cat Sensor after Food Dispensed!");
      Write_SDcard("Cat Sensor", false);
      catSensorState = HIGH;               // update catSensorState to HIGH
    }
  } else {
    digitalWrite(catLED, LOW);             // turn catLED OFF
    delay(200);                            // delay 200 milliseconds

    if (catSensorState == HIGH) {
      Serial.println("Cat Sensor Motion stopped!");
      catSensorState = LOW;                // update catSensorState to LOW
      foodDispensed = false;               // reset foodDispensed
    }
  }
}

/*****************************************************************************************************
   Initialize_RTC
    - Initialize the RTC object
    - Set date, if needed
 *****************************************************************************************************/
void Initialize_RTC() {

  // Initialize the rtc object

  rtc.begin();


  //#### The following lines can be uncommented to set the date and time for the first time###

  rtc.setDOW("Saturday");     // Set Day-of-Week to SUNDAY

  rtc.setTime(10, 46, 45);     // Set the time (HH:mm:ss) (24hr format)

  rtc.setDate(5, 22, 2022);   // Set the date

}

/*****************************************************************************************************
   Initialize_SDcard
    - Check if SD card is present and can be initialized (if not, return)
    - Open a new file on the SD card
 *****************************************************************************************************/
void Initialize_SDcard() {

  // check if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {

    Serial.println("Card failed, or not present");

    // don't do anything else
    return;

  }

  // open the file. (note: only one file can be open at a time,

  // so you have to close this one before opening another).

  File dataFile = SD.open("LoggerCD.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {

    dataFile.println("Date,Sensor,Time,Difference (ms)"); //Write the first row of the excel file

    dataFile.close();

  }

}

/*****************************************************************************************************
   Write_SDcard
    - Open the file on SD (if available)
    - Write date,whichSensor,currentTime & splitTime to CSV
    - For splitTime, write 0 if isFood, catTime-foodTime if !isFood
 *****************************************************************************************************/
void Write_SDcard(String whichSensor, bool isFood) {

  // open the file. note that only one file can be open at a time,

  // so you have to close this one before opening another.

  File dataFile = SD.open("LoggerCD.txt", FILE_WRITE);

  // if the file is available, write to it:

  if (dataFile) {

    dataFile.print(rtc.getDateStr()); //Store date on SD card

    dataFile.print(","); //Move to next column using a ","


    dataFile.print(whichSensor); //Store sensor label on SD card

    dataFile.print(","); //Move to next column using a ","


    dataFile.print(rtc.getTimeStr()); //Store time on SD card

    dataFile.print(","); //Move to next column using a ","

    if (isFood && foodDispensed) {
      foodTime = millis();    // capture current time in milliseconds

      dataFile.print("0"); //Store difference as 0 on SD card (cat has not arrived)

      dataFile.print(","); //Move to next column using a ","
    } else {
      //Store time diff (in ms) between foodTime and now to SD card (cat is eating)
      
      dataFile.print(String(millis() - foodTime));  

      dataFile.print(","); //Move to next column using a ","

    }

    dataFile.println(); //End of Row move to next row

    dataFile.close(); //Close the file

  }

  else

    Serial.println("OOPS!! SD card writing failed...\nFile not available!");

}
