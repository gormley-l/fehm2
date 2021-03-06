#include <FEHLCD.h>
#include <FEHUtility.h>
#include <FEHIO.h>
#include <FEHMotor.h>
#include <FEHServo.h>
#include <FEHRPS.h>
#include <FEHSD.h>
#include <math.h>
#include <FEHBattery.h>

//Defining pi for consistency and ease of use
#define PI 3.1415
/*Definition for a standard power for use with IGWAN motor movement.
 Useful because it allows universal changes with one adjustment. Must be a value between -100 and 100.*/
#define MOVE 50
#define TURN 25
//Definition for wheel diameter in inches
#define WHEEL 2.5
//Definition for distance between wheels (Wheel to Wheel) in inches
#define W2W 7.5
//Definition for a rest period to be used to ensure robot makes complete stops. Defined so it can be optimized with ease later.
#define REST 0.1

//Definition for line following switch cases
#define ON_LINE 0
#define LINE_ON_RIGHT 1
#define LINE_ON_LEFT 2
#define LINE_FAR_RIGHT 3
#define LINE_FAR_LEFT 4
#define OFF_LINE 5

//Break values for the optosensors to change line following states
#define LEFT_BREAK 1.0 //formerly 2.4 from exploration 2
#define CENTER_BREAK 1.27 //formerly 2.0 from exploration 2
#define RIGHT_BREAK 2.68 //formerly 2.7 from exploration 2

//Defining color integers for CdS readings
#define CDSRED 0
#define CDSBLUE 1
#define NO_COLOR 2
#define NO_READING 3

//Defining min and max for servos
#define SERVO_ARM_MAX 2350
#define SERVO_ARM_MIN 500
#define SERVO_FORK_MAX 2350
#define SERVO_FORK_MIN 500

//Declarations for IGWAN motors with their max voltage of 9V
FEHMotor leftMotor(FEHMotor::Motor3,9);
FEHMotor rightMotor(FEHMotor::Motor2,9);

//Declarations for the shaft encoders on the IGWAN motors
DigitalEncoder leftEncoder(FEHIO::P0_0);
DigitalEncoder rightEncoder(FEHIO::P0_1);

//Declaration for the CdS sensor
AnalogInputPin CdS(FEHIO::P0_2);

//Declaration of optosensors for line following
AnalogInputPin leftLine(FEHIO::P1_0);
AnalogInputPin centerLine(FEHIO::P1_1);
AnalogInputPin rightLine(FEHIO::P1_2);

//Declaration of digital inputs for the microswitches
DigitalInputPin frontLeftSwitch(FEHIO::P2_0);
DigitalInputPin frontRightSwitch(FEHIO::P2_1);
DigitalInputPin backLeftSwitch(FEHIO::P2_3);
DigitalInputPin backRightSwitch(FEHIO::P2_4);
DigitalInputPin forkSwitch(FEHIO::P2_2);

//Delcaring servos
FEHServo servo_arm(FEHServo::Servo0);
FEHServo servo_fork(FEHServo::Servo1);

//Function prototype for moving a linear distance, returns nothing, accepts a distance in inches
void linearMove(float distance, float speed);

/*Function prototype for pivoting on a spot, returns nothing.
 Accepts a degree amount to turn from -360 to 360, with negative numbers turning left and positive turning right.*/
void pivot(float degrees, float speed);

//Function prototype for checking what color the CdS cell sees, returns 0 for red, 1 for blue, 2 for black/no color
//If it returns a 3, it could not get any reading from the CdS cell, or some other error has occured
int cdsColor();

//Function prototype for testing the CdS cell readings
void testCdS();

//Function prototype for line following, accepts integers to determine its end conition, 0 is indefinite, 1 is microswitches, 2 is screen touch
void lineFollow(int condition);

//Function prototype for testing line following end conditions
bool checkCondition(int end);

//Function prototype for finding a line assuming the robot is near one but not quite on it
void findLine();

//Function prototype for checking if a side's microswitches are pressed, accepts an int, 0 for front side, 1 for back side microswitches to be checked
bool microSwitchCheck(int side);

//Function prototype for pressing the jukebox button
void jukebox();

//Function prototype for dumping the tray into the sink
void tray();

//Function prototype for moving the ticket
void ticket();

//Function prototype for burger flip
void burger();

//Function prototype for the first icecream flip
void icecream_1(int lever);

//Function prototype for the second icecream flip (back up)
void icecream_2(int lever);

//Function prototype for moving from jukebox to final button
void jBox2Final();

//Function prototype for performance tests
void p1();
void p2();
void p3();

int main(void)
{
    //Variable declarations
    float x,y;
    float bat_v = 0, m = 0;
    int icecreamLever;

    //Setting up RPS
    RPS.InitializeTouchMenu();

    //Setting servo max and mins
    servo_arm.SetMax(SERVO_ARM_MAX);
    servo_arm.SetMin(SERVO_ARM_MIN);
    servo_fork.SetMax(SERVO_FORK_MAX);
    servo_fork.SetMin(SERVO_FORK_MIN);

    //Pre-run setup
    //Resetting servo positions
    servo_arm.SetDegree(0.0);
    servo_fork.SetDegree(0.0);

    //Waiting for a touch input
    LCD.Clear(FEHLCD::Black);
    LCD.SetFontColor(FEHLCD::White);
    LCD.WriteLine("Waiting for touch input to continue");
    LCD.WriteAt("BATT:        V", 0, 222);
    while(true)
    {
        //Writing battery voltage to screen
        bat_v = ((bat_v*m)+Battery.Voltage());
        bat_v = bat_v/(++m);
        LCD.WriteAt(bat_v, 72, 222);

        if(microSwitchCheck(0))
        {
            LCD.Clear(FEHLCD::Black);
            break;
        }
    }

    //Waiting for start light
    LCD.WriteLine("Waiting for light to continue");
    while(true)
    {
        if(cdsColor() == CDSRED)
        {
            break;
        }
    }

    //Obtaining ice cream lever value now that the run has started.
    icecreamLever = RPS.GetIceCream();

    //Course functions
    tray();
    lineFollow(2);
    icecream_1(icecreamLever);
    burger();
    icecream_2(icecreamLever);
    ticket();
    jukebox();
    jBox2Final();


    //Printing statement to show code completion
    LCD.Clear(FEHLCD::Black);
    LCD.WriteLine("Done.");
    return 0;
}

//Function definition for moving a linear distance, input is a linear distance
void linearMove(float distance, float speed)
{
    //Temp variable
    float x;
    //Converts distance input into the number of counts for the shaft encoder to move for
    x = (318.0/(WHEEL*PI))*abs(distance);
    //Reset counts for safety
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    //Displaying goal
    LCD.Clear(FEHLCD::Black);
    LCD.WriteLine("Moving");
    LCD.WriteLine(distance);
    //Checks if forward or backwards distance is requested
    if (distance>0)
    {
        //Move forward for number of counts
        rightMotor.SetPercent(speed);
        leftMotor.SetPercent(speed);
        while(leftEncoder.Counts() < x)
        {
        }
    }else if (distance<0)
    {
        //Move backward for number of counts
        rightMotor.SetPercent(-speed);
        leftMotor.SetPercent(-speed);
        while(leftEncoder.Counts() < x)
        {
        }
    }
    //Stop motors
    leftMotor.Stop();
    rightMotor.Stop();
    //Reset counts
    LCD.WriteLine("Actual movement:");
    LCD.WriteLine(leftEncoder.Counts()/(318.0/(WHEEL*PI)));
    LCD.WriteLine(rightEncoder.Counts()/(318.0/(WHEEL*PI)));
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    //Rest to ensure momentum stops
    Sleep(REST);
}

//Function definition for pivoting
void pivot(float degrees, float speed)
{
    //Temp variable
    float x;
    //Converts degree input to number of counts the motors need to turn in opposite directions for
    x = ((318*W2W)/(360*WHEEL))*abs(degrees);
    //Reset counts for safety
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    //Displaying goal
    LCD.Clear(FEHLCD::Black);
    LCD.WriteLine("Turning");
    LCD.WriteLine(degrees);
    //Checks for negative (left) or positive (right) turn
    if (degrees > 0)
    {
        //Turn right for the number of counts
        rightMotor.SetPercent(-speed);
        leftMotor.SetPercent(speed);
        while ((leftEncoder.Counts()) <= x)
        {
        }
    } else if (degrees < 0)
    {
        //Turn left for the number of counts
        rightMotor.SetPercent(speed);
        leftMotor.SetPercent(-speed);
        while ((rightEncoder.Counts()) <= x)
        {
        }
    }
    //Stop motors
    leftMotor.Stop();
    rightMotor.Stop();
    //Reset counts
    LCD.WriteLine("Actual turn:");
    LCD.WriteLine(leftEncoder.Counts()/((318*W2W)/(360*WHEEL)));
    LCD.WriteLine(rightEncoder.Counts()/((318*W2W)/(360*WHEEL)));
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    //Rest to insure momentum stops
    Sleep(REST);
}

//Function definition for checking the color that the CdS cell sees
int cdsColor()
{
    /*Simple if checks to determine what color the CdS cell sees based off of measured ranges.
    Will change the LCD display to match the color it detects*/
    if (CdS.Value() > 0 && CdS.Value() <= 0.90)
    {
        LCD.Clear(FEHLCD::Red);
        return CDSRED;
    } else if (CdS.Value() > 0.90 && CdS.Value() <= 1.8)
    {
        LCD.Clear(FEHLCD::Blue);
        return CDSBLUE;
    } else if (CdS.Value() > 1.80 && CdS.Value() <= 3.3)
    {
        LCD.Clear(FEHLCD::Black);
        LCD.Write("No colored light detected");
        return NO_COLOR;
    } else
    {
        return NO_READING;
    }
}

//Funtion for testing the CdS cell values
void testCdS()
{
    float x,y;
    LCD.Clear(FEHLCD::Black);
        LCD.WriteLine("Checking CdS Cell function");
        while(true)
        {
            LCD.WriteLine(CdS.Value());
            if(LCD.Touch(&x,&y))
            {
                LCD.WriteLine("Ending CdS reading...");
                Sleep(1.0);
                break;
            }
            Sleep(REST);
        }
}

//Function definition for following a line
void lineFollow(int condition)
{
    //initilizing state variable
    int state, time = 0;

    //print statement to show what robot is doing
    LCD.Clear(FEHLCD::Black);
    LCD.WriteLine("Looking for line");
    findLine();
    LCD.Clear(FEHLCD::Black);
    LCD.WriteLine("Following Line");

    //This loop will call the checkCondition function to determine when to break, with condition 0 running indefinitly, 1 running until a microswitch input, 2 a touchscreen input
    while(checkCondition(condition))
    {
        //Updating state to turn based on optosensor inputs. > means that the sensor is seeing dark, < is the sensor seeing light
        if(rightLine.Value() < RIGHT_BREAK && centerLine.Value() > CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
            state = ON_LINE;
            if(time == 0)
            {
                time = TimeNow();
            }
        } else if (rightLine.Value() > RIGHT_BREAK && centerLine.Value() > CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
            state = LINE_ON_RIGHT;
            time = 0;
        }else if (rightLine.Value() < RIGHT_BREAK && centerLine.Value() > CENTER_BREAK && leftLine.Value() > LEFT_BREAK)
        {
            state = LINE_ON_LEFT;
            time = 0;
        }else if (rightLine.Value() > RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
            state = LINE_FAR_RIGHT;
            time = 0;
        }else if (rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() > LEFT_BREAK)
        {
            state = LINE_FAR_LEFT;
            time = 0;
        }else if (rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
            state = OFF_LINE;
            time = 0;
        }

        //Setting state of line based on where sensors are located in relationship to the line
        switch(state)
        {
        case ON_LINE:
            if(TimeNow() - time < 2.0)
            {
                rightMotor.SetPercent(-10);
                leftMotor.SetPercent(-10);
            }else if (TimeNow() - time >= 2.0)
            {
                rightMotor.SetPercent(-20);
                leftMotor.SetPercent(-20);
            }
            break;
        case LINE_ON_RIGHT:
            rightMotor.SetPercent(-20);
            leftMotor.SetPercent(-10);
            break;
        case LINE_ON_LEFT:
            rightMotor.SetPercent(-10);
            leftMotor.SetPercent(-20);
            break;
        case LINE_FAR_RIGHT:
            rightMotor.SetPercent(-30);
            leftMotor.SetPercent(-10);
            break;
        case LINE_FAR_LEFT:
            rightMotor.SetPercent(-10);
            leftMotor.SetPercent(-30);
            break;
        case OFF_LINE:
            rightMotor.Stop();
            leftMotor.Stop();
            break;
        default:
            rightMotor.Stop();
            leftMotor.Stop();
            break;
        }
    }
}

//Function definition for checking if the desired end condition for the line following is met
bool checkCondition(int end)
{
    float x,y;
    switch(end)
    {
    case 0:
        //No end condition
        return true;
    case 1:
        //Microswitch end condition
        if (microSwitchCheck(1))
        {
            return false;
        } else
        {
            return true;
        }
    case 2:
        //Off of the line
        if (rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
            return false;
        }else
        {
            return true;
        }
    default:
        LCD.WriteLine("No ending condition for line following entered");
        LCD.WriteLine("Skipping line following");
        return false;
    }
}

//Function definition that turns the robot around looking for a line
void findLine()
{
    int i;
    for (i = 1; i < 7; i++)
    {
        if (rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
            if (i % 2 == 0)
            {
                pivot(-10*i, TURN);
            } else
            {
                pivot(10*i, TURN);
            }
        }else
        {
            i = 6;
        }
    }
}

//Function definition that checks for microswitch values on the front or back of the robot
bool microSwitchCheck(int side)
{
    //Case 0 checks the front microswitches, case 1 checks the back sensors
    //Returning true means both sensors are NOT pressed
    //Returning false means both sensors ARE pressed (or an inproper number was checked and the switchcase defaulted)
    switch(side)
    {
    case 0:
        if (frontLeftSwitch.Value() == false && frontRightSwitch.Value() == false)
        {
            return false;
        } else
        {
            return true;
        }
    case 1:
        if (backLeftSwitch.Value() == false && backRightSwitch.Value() == false)
        {
            return false;
        } else
        {
            return true;
        }
    default:
        LCD.WriteLine("Improper integer input for checking microswitches");
        Sleep(3.0);
        return false;
    }
}

void jukebox()
{
    //Moving unill the robot runs into the wall
    leftMotor.SetPercent(MOVE);
    rightMotor.SetPercent(MOVE);
    while(microSwitchCheck(0))
    {
    }
    leftMotor.Stop();
    rightMotor.Stop();
    //Backing up off the wall
    linearMove(-4, MOVE);
    //Turning to face the jukebox
    pivot(-90, TURN);
    //Moving forward in small increments untill the CdS cell reads a red or blue light
    do
    {
        linearMove(0.1, MOVE);
        Sleep(REST);
    } while(cdsColor() == NO_COLOR);
    //Switch case for red and blue lights
    switch(cdsColor())
    {
    case CDSRED:
        //Turn left a bit
        pivot(45, TURN);
        //Move forward an inch
        linearMove(1, MOVE);
        //Re-align with the jukebox's red button
        pivot(-45, TURN);
        //Run into red button
        linearMove(3, MOVE);
        //Back off of button
        linearMove(-6, MOVE);
        //Turn towards the final zone
        pivot(-90, TURN);
        //Move towards the final zone
        linearMove(-6, MOVE);
    case CDSBLUE:
        //Turn right a bit
        pivot(-45, TURN);
        //Move for an inch
        linearMove(1, MOVE);
        //Re-align with the jukebox's blue button
        pivot(45, TURN);
        //Run into blue button
        linearMove(3, MOVE);
        //Back off of button
        linearMove(-6, MOVE);
        //Turn towards final zone
        pivot(-90, TURN);
        //Move towards final zone (red and move movements are different because the blue is further away from the final zone)
        linearMove(-8, MOVE);
    default:
        //default is blue
        //Turn right a bit
        pivot(-45, TURN);
        //Move for an inch
        linearMove(1, MOVE);
        //Re-align with the jukebox's blue button
        pivot(45, TURN);
        //Run into blue button
        linearMove(3, MOVE);
        //Back off of button
        linearMove(-6, MOVE);
        //Turn towards final zone
        pivot(-90, TURN);
        //Move towards final zone (red and move movements are different because the blue is further away from the final zone)
        linearMove(-8, MOVE);
    }
}

//Function definition for dumping the tray
void tray()
{
    //This float is exclusively required to go up the ramp at a high speed but slow down to a slower speed without stopping to prevent the tray from flying off, the value in abs() is the distance being traveled
    float start = (318.0/(WHEEL*PI))*abs(2);
    float ramp = (318.0/(WHEEL*PI))*abs(22);
    //This function is set up to start at the beginning of the course and move the robot up the ramp and dump the tray at the sink
    //Going up ramp from starting position
    rightMotor.SetPercent(0.65*MOVE);
    leftMotor.SetPercent(0.65*MOVE);
    while(leftEncoder.Counts() < start)
    {
    }
    linearMove(8, MOVE);
    pivot(45, TURN);
    //A chunk of the linearMove function is used here in order to move a set distance to get up the ramp, however, it is modified to not stop the motors so that the robot can
    //instead switch to a slower speed without stopping in order to prevent the tray from flying off of the robot
    rightMotor.SetPercent(1.5*MOVE);
    leftMotor.SetPercent(1.5*MOVE);
    while(leftEncoder.Counts() < ramp)
    {
    }
    linearMove(9, MOVE);
    //Turning towards the sink
    pivot(-90, 0.75*TURN);
    //Running into the side wall next to the sink, checking for front microswitch inputs
    leftMotor.SetPercent(0.75*MOVE);
    rightMotor.SetPercent(0.75*MOVE);
    while(microSwitchCheck(0))
    {
    }
    leftMotor.Stop();
    rightMotor.Stop();
    //Backing up to align with the sink
    linearMove(-3, 0.75*MOVE);
    //Turning to face sink
    pivot(-90, TURN);
    //Running into sink at a high speed in order to dump tray by checking for front microswitch inputs
    leftMotor.SetPercent(1.5*MOVE);
    rightMotor.SetPercent(1.5*MOVE);
    while(microSwitchCheck(0))
    {
    }
    leftMotor.Stop();
    rightMotor.Stop();
    Sleep(REST);
}

//Function definition for moving the ticket using the servo arm
void ticket()
{
    //Moving untill the robot hits the wall by the ticket
    leftMotor.SetPercent(MOVE);
    rightMotor.SetPercent(MOVE);
    while(microSwitchCheck(0))
    {
    }
    leftMotor.Stop();
    rightMotor.Stop();
    //Backing up off the wall
    linearMove(-6.5, MOVE);
    //Turning to position the servo arm
    pivot(90, TURN);
    linearMove(13, MOVE);
    pivot(45, TURN);
    //Deploying the servo arm
    servo_arm.SetDegree(100);
    //Inserting the servo arm into the ticket slot
    pivot(30, TURN);
    linearMove(-2,MOVE);
     pivot(10, TURN);
    //Moving forward with the ticket
    linearMove(5, MOVE);
    //Removing the servo arm from the ticket slot
    pivot(-45, TURN);
    //Reseting the servo arm
    servo_arm.SetDegree(0);
    //Re-aligning the robot
    pivot(45, TURN);
    //Moving towards the ramp
    linearMove(8, MOVE);
    //Turning to face the ramp
    pivot(90, TURN);
    //Going down the ramp
    linearMove(22, MOVE);
}

void burger()
{
    float t;
    //Backing off of the wall
    linearMove(-2.2,MOVE);
    //Turning so that the back of the robot is facing the burger station
    pivot(90, TURN);
    //Moving the robot up to the burger staton
    //Slowly moving forward until the microswitch on the fork is activated or it times out at 5 seconds, indicating that the fork is inserted into the wheel
    t = TimeNow();
    rightMotor.SetPercent(-10);
    leftMotor.SetPercent(-10);
    while((forkSwitch.Value() == true) && (TimeNow() - t < 5.0))
    {
    }
    rightMotor.Stop();
    leftMotor.Stop();
    Sleep(REST);
    //Rotating the fork and wheel
    servo_fork.SetDegree(95);
    Sleep(2.0);
    //Resetting the hotplate position
    servo_fork.SetDegree(0);
    //Backing away from the hotplate
    linearMove(0.5, MOVE);
    //Turning to face the line that leads to the icecream machine
    pivot(-90, TURN);
    //Following line over to ice cream machine
    lineFollow(2);
}

//Function definition for flipping the proper lever down
void icecream_1(int lever)
{
    //Depending on the lever reading from rps, the robot will navigate to the correct line to then run into the lever
    if (lever == 0)
    {
        //Already in line for the first lever, so turn to face it
        pivot(90, TURN);
        //Run into the lever
        linearMove(12, MOVE);
        Sleep(1.0);
        //Back off of the lever using line following
        lineFollow(2);
        //Turn to face the burger area
        pivot(-90, TURN);
        //Move to the second icecream line
        rightMotor.SetPercent(-20);
        leftMotor.SetPercent(-20);
        while(rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
        }
        rightMotor.Stop();
        leftMotor.Stop();
        //Move to the third icecream line
        rightMotor.SetPercent(-20);
        leftMotor.SetPercent(-20);
        while(rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
        }
        rightMotor.Stop();
        leftMotor.Stop();
    }else if (lever == 1)
    {
        //Move from the first icecream line to the second
        rightMotor.SetPercent(-20);
        leftMotor.SetPercent(-20);
        while(rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
        }
        rightMotor.Stop();
        leftMotor.Stop();
        //Turn to face the lever
        pivot(90, TURN);
        //Run into the lever
        linearMove(12, MOVE);
        Sleep(1.0);
        //Back off of the lever using line following
        lineFollow(2);
        //Turn to face the burger area
        pivot(-90, TURN);
        //Move to the third icecream line
        rightMotor.SetPercent(-20);
        leftMotor.SetPercent(-20);
        while(rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
        }
        rightMotor.Stop();
        leftMotor.Stop();
    }else if (lever == 2)
    {
        //Move from the first icecream line to the second
        rightMotor.SetPercent(-20);
        leftMotor.SetPercent(-20);
        while(rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
        }
        rightMotor.Stop();
        leftMotor.Stop();
        //Move to the third icecream line
        rightMotor.SetPercent(-20);
        leftMotor.SetPercent(-20);
        while(rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
        }
        rightMotor.Stop();
        leftMotor.Stop();
        //Turn and face the lever
        pivot(90, TURN);
        //Run into the lever
        linearMove(12, MOVE);
        Sleep(1.0);
        //Back off of the lever using line following
        lineFollow(2);
        //Turn and face the burger area
        pivot(-90, TURN);
    }
    //Follow the line unill the microswitches are pressed
    lineFollow(1);
}

//Function definition for flipping the proper lever back up
void icecream_2(int lever)
{
    if (lever == 0)
    {
        //Move from the third icecream line to the second
        rightMotor.SetPercent(-20);
        leftMotor.SetPercent(-20);
        while(rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
        }
        rightMotor.Stop();
        leftMotor.Stop();
        //Move to the first icecream line
        rightMotor.SetPercent(-20);
        leftMotor.SetPercent(-20);
        while(rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
        }
        rightMotor.Stop();
        leftMotor.Stop();
        //Turn and face the lever
        pivot(-90, TURN);
        //Run into the lever
        linearMove(12, MOVE);
        Sleep(1.0);
        //Back off of the lever using line following
        lineFollow(2);
        //Move to line up with the wall by the ticket
        linearMove(-6, MOVE);
        //Turn to have the bumper face the wall
        pivot(135, TURN);
    }else if (lever == 1)
    {
        //Move from the first icecream line to the second
        rightMotor.SetPercent(-20);
        leftMotor.SetPercent(-20);
        while(rightLine.Value() < RIGHT_BREAK && centerLine.Value() < CENTER_BREAK && leftLine.Value() < LEFT_BREAK)
        {
        }
        rightMotor.Stop();
        leftMotor.Stop();
        //Turn to face the lever
        pivot(90, TURN);
        //Run into the lever
        linearMove(12, MOVE);
        Sleep(1.0);
        //Back off of the lever using line following
        lineFollow(2);
        //Move to line up with the wall by the ticket
        linearMove(-11, MOVE);
        //Turn to have the bumper face the wall
        pivot(135, TURN);
    }else if (lever == 2)
    {
        //Already in line for the third lever, so turn to face it
        pivot(90, TURN);
        //Run into the lever
        linearMove(12, MOVE);
        Sleep(1.0);
        //Back off of the lever using line following
        lineFollow(2);
        //Move to line up with the wall by the ticket
        linearMove(-15, MOVE);
        //Turn to have the bumper face the wall
        pivot(135, TURN);
    }
}

//Function definition for hitting final button
void jBox2Final()
{
    //Turn and align with the final button
    pivot(45, TURN);
    //Run into the finish button
    linearMove(13, MOVE);
    Sleep(REST);
}

//Functions for performance tests
void p1()
{
    jukebox();
    pivot(90, TURN);
    linearMove(20, 1.5*MOVE);
}

void p2()
{
    tray();
    //Backing up off of the sink in order to allow for easy movement
    linearMove(-10, MOVE);
    pivot(-90, TURN);
    leftMotor.SetPercent(-MOVE);
    rightMotor.SetPercent(-MOVE);
    while(microSwitchCheck(1))
    {
    }
    leftMotor.Stop();
    rightMotor.Stop();

    ticket();
    rightMotor.SetPercent(-MOVE);
    leftMotor.SetPercent(-MOVE);
    while(backLeftSwitch.Value() == true)
    {
    }
    rightMotor.Stop();
    leftMotor.Stop();
    linearMove(2,MOVE);
    pivot(90, TURN);
    linearMove(20,MOVE);
}

void p3()
{
    //This float is exclusively required to go up the ramp at a high speed but slow down to a slower speed without stopping to prevent the tray from flying off, the value in abs() is the distance being traveled
    float start = (318.0/(WHEEL*PI))*abs(2);
    float ramp = (318.0/(WHEEL*PI))*abs(22);
    //Performance test 3 code
    //This function is set up to start at the beginning of the course and move the robot up the ramp and dump the tray at the sink
    //Going up ramp from starting position
    rightMotor.SetPercent(0.65*MOVE);
    leftMotor.SetPercent(0.65*MOVE);
    while(leftEncoder.Counts() < start)
    {
    }
    linearMove(8, MOVE);
    pivot(45, TURN);
    //A chunk of the linearMove function is used here in order to move a set distance to get up the ramp, however, it is modified to not stop the motors so that the robot can
    //instead switch to a slower speed without stopping in order to prevent the tray from flying off of the robot
    rightMotor.SetPercent(1.5*MOVE);
    leftMotor.SetPercent(1.5*MOVE);
    while(leftEncoder.Counts() < ramp)
    {
    }
    linearMove(18, MOVE);
    //Turning towards ticket
    pivot(90, TURN);
    //Moving until the front microswitches activate off of the wall
    leftMotor.SetPercent(MOVE);
    rightMotor.SetPercent(MOVE);
    while(microSwitchCheck(0))
    {
    }
    leftMotor.Stop();
    rightMotor.Stop();
    burger();
    pivot(-135, TURN);
    linearMove(12, MOVE);
}
