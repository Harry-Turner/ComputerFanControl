/*Several DS1820 sensors connected to the 1-wire bus:*/
#include "mbed.h"
#include "DS1820.h"
#include <string.h>
#include <PwmOut.h>
#include "Adafruit_SSD1306.h"
 
#define     MAX_SENSOSRS   32   // max number of DS1820 sensors to be connected to the 1-wire bus (max 256)

//Generic Variables for board to work with sensors and fans/led's
DS1820*     ds1820[MAX_SENSOSRS];
OneWire     oneWire(D8);        // substitute D8 with the actual pin name connected to the 1-wire bus
PwmOut      led(LED1);
PwmOut      fan(D3);

//DS18 Temperature Variables
float   temperature[32]={0};
float   avgTemperatureDisplay = 0; //Needed for the display of value in screen. Does not display without this
float   avgTemperature = 0; 
float   fanSpeed = 0.01f;
int     firstRun = 1; //Loads main menu screen
int     sensorsFound = 0;   // counts the actually found DS1820 sensors
int     automaticMode = 1;
int     automaticProfileVariable = static_cast<int>(avgTemperatureDisplay); //Conversion of float to int for ranges

//Oled Screen Variables
I2C myI2C(I2C_SDA,I2C_SCL);
Adafruit_SSD1306_I2c myGUI(myI2C,D7,0x78,32,128);
//Buttons used for menu
DigitalIn b1(D6); 
DigitalIn b2(D5);
DigitalIn b3(D4);

//Oled Menu variables
int mainMenuScreen = 1; //Variable needed for auto updates on the Stats screen
int temperatureScreen, profileScreen = 0; //Variable for automatic updates on stats screen
int performance, quiet, automatic = 0; //Variables needed for selecting profile
int selectedProfile = 1; //Variable to display number for corresponding profile selected
 
Timer timer; //Timer needed for timeout on profile select screen

void mainMenu (){
    
    myGUI.clearDisplay();
    myGUI.setTextCursor(0, 0);
    myGUI.printf("1.Temperature:%2.1fC\n2.Profile: %i\n3.About", avgTemperatureDisplay, selectedProfile);
    myGUI.display();
    myGUI.display();
    ThisThread::sleep_for(50);
}
void temperatureStats (){
     mainMenuScreen =0;
     profileScreen = 0;
     myGUI.clearDisplay();
     myGUI.setTextCursor(0, 0);
     myGUI.printf("Sensors:   S1:%.1fC \nS2:%.1fC   S3:%.1fC \nS4:%.1fC   S5:%.1fC \nAverage Temp: %.1fC" , temperature[0], temperature[1], temperature[2], temperature[3], temperature[4], avgTemperatureDisplay);
     myGUI.display();
     myGUI.display(); // Has to be double otherwise it takes 2 button presses to activate the screen
     ThisThread::sleep_for(50);
}
void profileSelectScreen (){
    mainMenuScreen = 0;
    myGUI.clearDisplay();
    myGUI.setTextCursor(0, 0);
    myGUI.printf("1.Performance\n2.Quiet\n3.Automatic");
    myGUI.display();
    myGUI.display(); // Has to be repeated otherwise it takes 2 button presses to activate the screen
    ThisThread::sleep_for(50);
}

void performanceProfile(){ //performance profile for fans. Max fan RPM do not care about noise, want best results.
    fanSpeed = 1.0f;
    fan = fanSpeed;
    automaticMode = 0;
}
void quietProfile(){ //Quiet profile for fans. Minimum RPM want quiet machine.
    fanSpeed = 0.15f; //There is 'Coil Whine' up to ~0.9f. 
    fan = fanSpeed;
    automaticMode = 0;
}
void automaticProfile(){
    automaticProfileVariable = avgTemperatureDisplay;
    switch (automaticProfileVariable) {
    
        case 28 ... 29: //Ranges of Temperature 
            fanSpeed = 1.0f;
            fan = fanSpeed;
            break;
        case 22 ... 27:
            fanSpeed =  0.15f;
            fan = fanSpeed;
            break;
        default:
            fanSpeed = 1.0f;
            fan = fanSpeed;
    }
    automaticMode = 1;
}

void aboutScreen(){
    myGUI.clearDisplay();
    myGUI.setTextCursor(0, 0);
    myGUI.printf("For more info:\ngithub.coventry.ac.uk/turnerh2/FanControl");
    myGUI.display();
    myGUI.display();
    ThisThread::sleep_for(50);
}

int main()
{   
    led.period(0.01f);
    led = 0.0f;
    fan.period(0.01f);
    fan = 0.0f;
    printf("\r\n--Starting--\r\n");
    
    //Enumerate (i.e. detect) DS1820 sensors on the 1-wire bus
    for (sensorsFound = 0; sensorsFound < MAX_SENSOSRS; sensorsFound++) {
        ds1820[sensorsFound] = new DS1820(&oneWire);
        if (!ds1820[sensorsFound]->begin()) {
            delete ds1820[sensorsFound];
            break;
        }
    }
 
    switch (sensorsFound) {
        /*case 0:
            printf("No sensor found.\r\n");
            return -1;*/
 
        case 1:
            printf("One DS1820 sensor found.\r\n");
            break;
 
        default:
            printf("Found");
            printf("DS1820 sensors.\r\n");
    }
 
    myGUI.begin(); 
    myGUI.setTextSize(1);

    while (1) {
        printf("----------------\r\n");
        for (int i = 0; i < sensorsFound; i++)
            ds1820[i]->startConversion();       // start temperature conversion from analog to digital
        ThisThread::sleep_for(50);            // let DS1820 sensors complete the temperature conversion
        for (int i = 0; i < sensorsFound; i++) {
            if (ds1820[i]->isPresent())
            temperature[i] = ds1820[i]->read();
                avgTemperature = avgTemperature + temperature[i];
                printf("Temperature = %.2f \n", temperature[i]);  }
                avgTemperature = avgTemperature/sensorsFound;
                avgTemperatureDisplay = avgTemperature;
                printf("Average Temperature = %.2f \n", avgTemperature);            
                avgTemperature = 0;

        if (firstRun == 1){ //Need to have first run otherwise screen flickers due to it being run multiple times or jumps back to main menu
            mainMenu();
            firstRun = 0;
        }
        if (automaticMode == 1){    //used for 1st run through. Automaticlly chooses 'Auto mode' 
            automaticProfile();
        }
        if(profileScreen == 0 && mainMenuScreen ==0){ //Function to decide if user currently in temperature stats screen
            temperatureStats();                       // If true then will update temperature values
            }
        if (mainMenuScreen == 1) { //Used for auto-update on main menu
            mainMenu();
        }

        if (b1 == 0){
            //temperatureScreen = 1;
                temperatureStats();
                temperatureScreen = 0;
        }
        if (b2 == 0){
            profileScreen = 1;
            profileSelectScreen();
           
            while(1){
                timer.reset();
                timer.start();
                while(timer.read() < 5){
                    if (b1 == 0){ 
                        performance = 1;
                        if (performance == 1){
                            myGUI.clearDisplay();
                            myGUI.setTextCursor(0, 0);
                            myGUI.printf("Performance Mode");
                            myGUI.display();
                            myGUI.display(); // Has to be repeated otherwise it takes 2 button presses to activate the screen
                                
                            performanceProfile();

                            selectedProfile = 1;
                            ThisThread::sleep_for(500);
                            mainMenu();
                            break;
                         } 
                     }else if (b2 == 0){
                        quiet = 1;
                        if (quiet == 1){
                            myGUI.clearDisplay();
                            myGUI.setTextCursor(0, 0);
                            myGUI.printf("Quiet Mode");
                            myGUI.display();
                            myGUI.display(); // Has to be repeated otherwise it takes 2 button presses to activate the screen
                                
                            quietProfile();

                            selectedProfile = 2;
                            ThisThread::sleep_for(500);
                            mainMenu();
                            break;
                         }
                     }else if (b3 == 0){
                        automatic = 1;
                        if (automatic == 1){
                            myGUI.clearDisplay();
                            myGUI.setTextCursor(0, 0);
                            myGUI.printf("Automatic Mode");
                            myGUI.display();
                            myGUI.display(); // Has to be repeated otherwise it takes 2 button presses to activate the screen
                                
                            automaticProfile();

                            selectedProfile = 3;
                            ThisThread::sleep_for(500);
                            mainMenu();
                            break;
                         }
                     }
                 }
                break;
             }             
     }
     if (b3 == 0){

        aboutScreen();
        ThisThread::sleep_for(5s);
        mainMenuScreen = 1;
        mainMenu();
        profileScreen = 0;
     }
     }
}
