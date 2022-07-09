# cat-tracker
## An Arduino project which measures the time difference between my cat's automatic feeder dispensing food and him arriving at his bowl.

The task assigned for my senior project is to put together a small data collection system and visualize the data in a graphical manner.

Here's the [final presentation of the project](https://vimeo.com/726744707)

When brainstorming ideas one evening, my 15-pound cat Juno came in to let me know he was hungry. 

Now, I cut myself out of that equation *years ago* by getting him an auto-feeder, but he still insists on letting me know when it's dinner time.

What happened next was pure serendipity. The instant his feeder began to loudly dispense his food, he bolted out of the room faster than a bat out of hell!

It was then that I realized what I had to do....

## Introducing: The ~~Juno~~ Cat Tracker 5000

This feat of modern engineering is centered around an Arduino Uno Microcontroller which leverages the power of its ATMega328P to do the unthinkable.

The Cat Tracker 5000 uses the first of its **two** passive infrared (PIR) sensors to detect when your pet's food begins dispensing, and writes the 
foodEvent to a built-in SD card with a timestamp provided by its fully-programmable DS3231 real-time clock (RTC) module. 

When your pet arrives on the scene 2.2 seconds later to gobble down that hard-earned food, the second PIR sensor is triggered and the petEvent is also written to the SD.


###But wait..there's more!! 

The events are written to the SD card as comma-separated values (CSV) which can then be imported into your favorite spreadsheet program for analysis!

Pre-order this totally real product today!
