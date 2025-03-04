//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

//=====[Declaration and initialization of public global objects]===============

DigitalIn enterButton(BUTTON1);
DigitalIn gasDetector(D2);
DigitalIn overTempDetector(D3);
DigitalIn aButton(D4);
DigitalIn bButton(D5);
DigitalIn cButton(D6);
DigitalIn dButton(D7);

DigitalOut alarmLed(LED1);
DigitalOut incorrectCodeLed(LED3);
DigitalOut systemBlockedLed(LED2);

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

//=====[Declaration and initialization of public global variables]=============

bool alarmState = OFF;
int numberOfIncorrectCodes = 0;

//=====[Declarations (prototypes) of public functions]=========================

void inputsInit();
void outputsInit();

void alarmActivationUpdate();
void alarmDeactivationUpdate();

void uartTask();
void availableCommands();
void resetSystem();
void sendContinuousData();  //New function to send the alarm state to serial monitor

//Ticker created so alarm states can be sent back regularly
Ticker alarmStateTicker;

//=====[Main function, the program entry point after power on or reset]========

int main()
{
    inputsInit();
    outputsInit();
    
    // Set the ticker to call data every 2 seconds
    //2 seconds ensures for continuos updates
    alarmStateTicker.attach(&sendContinuousData, 2.0);  
    
    while (true) {
        alarmActivationUpdate();
        alarmDeactivationUpdate();
        uartTask();
    }
}

//=====[Implementations of public functions]===================================

void inputsInit()
{
    gasDetector.mode(PullDown);
    overTempDetector.mode(PullDown);
    aButton.mode(PullDown);
    bButton.mode(PullDown);
    cButton.mode(PullDown);
    dButton.mode(PullDown);
}

void outputsInit()
{
    alarmLed = OFF;
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF;
}

void alarmActivationUpdate()
{
    if ( gasDetector || overTempDetector ) {
        alarmState = ON;
    }
    alarmLed = alarmState;
}

void alarmDeactivationUpdate()
{
    if ( numberOfIncorrectCodes < 5 ) {
        if ( aButton && bButton && cButton && dButton && !enterButton ) {
            incorrectCodeLed = OFF;
        }
        if ( enterButton && !incorrectCodeLed && alarmState ) {
            if ( aButton && bButton && !cButton && !dButton ) {
                alarmState = OFF;
                numberOfIncorrectCodes = 0;
                uartUsb.write("Alarm deactivated successfully.\r\n", 32);
            } else {
                incorrectCodeLed = ON;
                numberOfIncorrectCodes += 1;
                uartUsb.write("Incorrect code attempt. Try again.\r\n", 39);
            }
        }
    } else {
        systemBlockedLed = ON;
        uartUsb.write("System is blocked due to 5 incorrect attempts.\r\n", 48);
    }
}

void uartTask()
{
    char receivedChar = '\0';
    if( uartUsb.readable() ) {
        uartUsb.read( &receivedChar, 1 );
        
        switch(receivedChar) {
            case '1':
                // Report the alarm state
                if ( alarmState ) {
                    uartUsb.write( "The alarm is activated.\r\n", 26);
                } else {
                    uartUsb.write( "The alarm is not activated.\r\n", 30);
                }
                break;
            
            case '2':
                // Report the state of the gas detector
                if (gasDetector) {
                    uartUsb.write("Gas detector is triggered.\r\n", 28);
                } else {
                    uartUsb.write("Gas detector is not triggered.\r\n", 32);
                }
                break;

            case '3':
                // Report the state of the over-temperature detector
                if (overTempDetector) {
                    uartUsb.write("Over temperature detector is triggered.\r\n", 43);
                } else {
                    uartUsb.write("Over temperature detector is not triggered.\r\n", 47);
                }
                break;

            case 'r':
                // Reset the system (clear incorrect attempts and reset alarm)
                resetSystem();
                uartUsb.write("System has been reset.\r\n", 24);
                break;

            default:
                // Show the available commands if any other key is pressed
                availableCommands();
                break;
        }
    }
}

void availableCommands()
{
    uartUsb.write( "Available commands:\r\n", 21 );
    uartUsb.write( "Press '1' to get the alarm state\r\n", 36 );
    uartUsb.write( "Press '2' to get the gas detector state\r\n", 42 );
    uartUsb.write( "Press '3' to get the over-temperature detector state\r\n", 58 );
    uartUsb.write( "Press 'r' to reset the system\r\n", 32 );
}

void resetSystem()
{
    alarmState = OFF;
    numberOfIncorrectCodes = 0;
    alarmLed = OFF;
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF;
}

//Function that allows user to send data continuously
void sendContinuousData()
{
    //Clear the current line and move to a new line
    uartUsb.write("\033[2K\r", 4); //ANSI escape sequence to clear line
    uartUsb.write("\r\n", 2);       //Ensure new line

    //Send alarm state
    if (alarmState) {
        uartUsb.write("The alarm is activated.\r\n", 26);  
    } else {
        uartUsb.write("The alarm is not activated.\r\n", 30);  
    }

    //Check the state of the gas detector and send its message
    if (gasDetector) {
        uartUsb.write("Gas detector is triggered.\r\n", 30); 
        //Write a warning to serrial monitor
        uartUsb.write("WARNING: GAS LEVEL TOO HIGH!\r\n", 32); 
    } else {
        uartUsb.write("Gas detector is not triggered.\r\n", 32);  
    }

    //Check the state of the over-temperature detector and send its message
    if (overTempDetector) {
        uartUsb.write("Over temperature detector is triggered.\r\n", 43);
        //Write a warning to serrial monitor
        uartUsb.write("WARNING: TEMPERATURE IS TOO HIGH!\r\n", 37);  
    } else {
        uartUsb.write("Over temperature detector is not triggered.\r\n", 47);  
    }
}
