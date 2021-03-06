#include <FEHLCD.h>
#include <FEHUtility.h>
#include <FEHIO.h>
#include <FEHMotor.h>
#include <FEHServo.h>
#include <FEHRPS.h>
#include <math.h>

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
#define on_line 0
#define line_on_right 1
#define line_on_left 2
#define line_far_right 3
#define line_far_left 4

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

//Function prototype for moving a linear distance, returns nothing, accepts a distance in inches
void linearMove(float distance, float speed);

/*Function prototype for pivoting on a spot, returns nothing.
 Accepts a degree amount to turn from -360 to 360, with negative numbers turning left and positive turning right.*/
void pivot(float degrees, float speed);

//Function prototype for checking what color the CdS cell sees, returns 0 for red, 1 for blue, 2 for black/no color
//If it returns a 3, it could not get any reading from the CdS cell, or some other error has occured
int cdsColor();

//Function prototype for line following
void lineFollow();

//Function prototype for jukebox
void jukebox();

int main(void)
{

    float x,y;
    //Waiting for a touch input
    LCD.Clear(FEHLCD::Black);
    LCD.SetFontColor(FEHLCD::White);
    LCD.WriteLine("Waiting for touch input to continue");
    while(true)
    {
        if(LCD.Touch(&x,&y))
        {
            LCD.Clear(FEHLCD::Black);
            break;
        }
    }
    LCD.WriteLine("Waiting for light to continue");
    while(true)
    {
        if(cdsColor() == 0)
        {
            break;
        }
    }

    //Testing turning
    /*while(true)
    {
        pivot(45, TURN);
        Sleep(2.0);
        pivot(-45, TURN);
        Sleep(2.0);
    }*/

    //Testing course manuverability
    //Moving off of start
    linearMove(-11, MOVE);
    pivot(-45, TURN);
    linearMove(-18, MOVE);
    linearMove(4, MOVE);
    pivot(90, TURN);
    do
    {
        linearMove(0.1, MOVE);
        Sleep(REST);
    } while(cdsColor() == 2);

    if(cdsColor() == 0)
        {
            pivot(45, TURN);
            linearMove(1, MOVE);
            pivot(-45, TURN);
            linearMove(3, MOVE);
            linearMove(-6, MOVE);
            pivot(-90, TURN);
            linearMove(-6, MOVE);
            linearMove(13, MOVE);
            pivot(-90, TURN);
            linearMove(30, MOVE);
            linearMove(-30, MOVE);
        }else if(cdsColor()==1)
       {
            pivot(-45, TURN);
            linearMove(1, MOVE);
            pivot(45, TURN);
            linearMove(3, MOVE);
            linearMove(-6, MOVE);
            pivot(-90, TURN);
            linearMove(-8, MOVE);
            linearMove(13, MOVE);
            pivot(-90, TURN);
            linearMove(30, MOVE);
            linearMove(-30, MOVE);

       }
    Sleep(2.0);


    /*pivot(-135, TURN);
    //Moving up ramp
    LinearMove(8, MOVE);
    linearMove(30, 1.5*MOVE);
    pivot(180, TURN);
    Sleep(1.0);
    //Moving down ramp
    linearMove(26, MOVE);
    pivot(90, TURN);
    linearMove(10, MOVE);
    pivot(-90, TURN);
    linearMove(2, MOVE);
    if(cdsColor() == 0)
        {
            pivot(20, TURN);
            linearMove(1, MOVE);
            pivot(-20, TURN);
            linearMove(3, MOVE);


        }else if(cdsColor()==1)
       {
            pivot(-20, TURN);
            linearMove(1, MOVE);
            pivot(20, TURN);
            linearMove(3, MOVE);

       }
    Sleep(2.0);
    linearMove(-6, MOVE);*/

    //Returning to start
    /*pivot(-90, TURN);
    linearMove(12, MOVE);
    pivot(45, TURN);
    linearMove(14, MOVE);*/

   //Printing CdS reading to screen untill screen is pressed again
   /* LCD.Clear(FEHLCD::Black);
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
    }*/

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
        while(leftEncoder.Counts() < x)
        {
            rightMotor.SetPercent(speed);
            leftMotor.SetPercent(speed);
            Sleep(REST);
        }
    }else if (distance<0)
    {
        //Move backward for number of counts
        while(leftEncoder.Counts() < x)
        {
            rightMotor.SetPercent(-speed);
            leftMotor.SetPercent(-speed);
            Sleep(REST);
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
    x = ((318*W2W)/(360*WHEEL))*abs(degrees);//*(0.0123*log(abs(degrees))+0.9262);
    //y = ((318*W2W)/(360*WHEEL))*abs(degrees);//*(0.0117*log(abs(degrees))+0.8993);
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
        while ((leftEncoder.Counts()) <= x)
        {
            rightMotor.SetPercent(-speed);
            leftMotor.SetPercent(speed);
            Sleep(REST);
        }
    } else if (degrees < 0)
    {
        //Turn left for the number of counts
        while ((rightEncoder.Counts()) <= x)
        {
            rightMotor.SetPercent(speed);
            leftMotor.SetPercent(-speed);
            Sleep(REST);
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
    LCD.Write("Checking color from CdS Cell");
    /*Simple if checks to determine what color the CdS cell sees based off of measured ranges.
    Will change the LCD display to match the color it detects*/
    if (CdS.Value() > 0 && CdS.Value() <= 0.90)
    {
        LCD.Clear(FEHLCD::Red);
        return 0;
    } else if (CdS.Value() > 0.90 && CdS.Value() <= 1.8)
    {
        LCD.Clear(FEHLCD::Blue);
        return 1;
    } else if (CdS.Value() > 1.80 && CdS.Value() <= 3.3)
    {
        LCD.Clear(FEHLCD::Black);
        LCD.Write("No colored light detected");
        return 2;
    } else
    {
        return 3;
    }
}

//Function definition for following a line
void lineFollow()
{
    int state = line_far_left;
    LCD.Clear(FEHLCD::Black);
    LCD.WriteLine("Following Line");
    while(true)
    {
        //Setting state of line based on where sensors are located in relationship to the line
        switch(state)
        {
        case on_line:
            rightMotor.SetPercent(10);
            leftMotor.SetPercent(10);
            break;
        case line_on_right:
            rightMotor.SetPercent(10);
            leftMotor.SetPercent(20);
            break;
        case line_on_left:
            rightMotor.SetPercent(20);
            leftMotor.SetPercent(10);
            break;
        case line_far_right:
            rightMotor.SetPercent(10);
            leftMotor.SetPercent(30);
            break;
        case line_far_left:
            rightMotor.SetPercent(30);
            leftMotor.SetPercent(10);
            break;
        default:
            rightMotor.Stop();
            leftMotor.Stop();
            break;
        }
        //Updating state to turn based on optosensor inputs
        if(rightLine.Value() < 2.7 && centerLine.Value() > 2.0 && leftLine.Value() < 2.4)
        {
            state = on_line;
        } else if (rightLine.Value() > 2.7 && centerLine.Value() > 2.0 && leftLine.Value() < 2.4)
        {
            state = line_on_right;
        }else if (rightLine.Value() < 2.7 && centerLine.Value() > 2.0 && leftLine.Value() > 2.4)
        {
            state = line_on_left;
        }else if (rightLine.Value() > 2.7 && centerLine.Value() < 2.0 && leftLine.Value() < 2.4)
        {
            state = line_far_right;
        }else if (rightLine.Value() < 2.7 && centerLine.Value() < 2.0 && leftLine.Value() > 2.4)
        {
            state = line_far_left;
        }
    }
}

void jukebox()
{
    linearMove(-11, MOVE);
    pivot(-45, TURN);
    linearMove(-18, MOVE);
    linearMove(4, MOVE);
    pivot(90, TURN);
    do
    {
        linearMove(0.1, MOVE);
        Sleep(REST);
    } while(cdsColor() == 2);

    if(cdsColor() == 0)
        {
            pivot(45, TURN);
            linearMove(1, MOVE);
            pivot(-45, TURN);
            linearMove(3, MOVE);
            linearMove(-6, MOVE);
            pivot(-90, TURN);
            linearMove(-6, MOVE);
            linearMove(13, MOVE);
            pivot(-90, TURN);
            linearMove(30, MOVE);
            linearMove(-30, MOVE);
        }else if(cdsColor()==1)
       {
            pivot(-45, TURN);
            linearMove(1, MOVE);
            pivot(45, TURN);
            linearMove(3, MOVE);
            linearMove(-6, MOVE);
            pivot(-90, TURN);
            linearMove(-8, MOVE);
            linearMove(13, MOVE);
            pivot(-90, TURN);
            linearMove(30, MOVE);
            linearMove(-30, MOVE);

       }
    Sleep(2.0);

}
