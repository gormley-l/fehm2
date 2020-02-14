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
//Definition for wheel diameter in inches
#define WHEEL 2.5
//Definition for distance between wheels (Wheel to Wheel) in inches
#define W2W 7.625
//Definition for a rest period to be used to ensure robot makes complete stops. Defined so it can be optimized with ease later.
#define REST 0.1

//Declarations for IGWAN motors with their max voltage of 9V
FEHMotor leftMotor(FEHMotor::Motor0,9);
FEHMotor rightMotor(FEHMotor::Motor1,9);
//Declarations for the shaft encoders on the IGWAN motors
DigitalEncoder leftEncoder(FEHIO::P0_0);
DigitalEncoder rightEncoder(FEHIO::P0_1);
//Declaration for the CdS sensor
AnalogInputPin CdS(FEHIO::P0_2);

//Function prototype for moving a linear distance, returns nothing, accepts a distance in inches
void linearMove(float distance);

/*Function prototype for pivoting on a spot, returns nothing.
 Accepts a degree amount to turn from -360 to 360, with negative numbers turning left and positive turning right.*/
void pivot(float degrees);

//Function prototype for checking what color the CdS cell sees, returns 0 for red, 1 for blue, 2 for black/no color
//If it returns a 3, it could not get any reading from the CdS cell, or some other error has occured
int cdsColor();

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
            LCD.WriteLine("Starting...");
            break;
        }
    }

    linearMove(12);
    pivot(90);
   /* //Printing CdS reading to screen untill screen is pressed again
    LCD.Clear(FEHLCD::Black);
    LCD.WriteLine("Checking CdS Cell function");
    while(true)
    {
        LCD.Clear(FEHLCD::Black);
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
void linearMove(float distance)
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
            rightMotor.SetPercent(MOVE);
            leftMotor.SetPercent(MOVE);
        }
    }else if (distance<0)
    {
        //Move backward for number of counts
        while(leftEncoder.Counts() < x)
        {
            rightMotor.SetPercent(-MOVE);
            leftMotor.SetPercent(-MOVE);
        }
    }
    //Stop motors
    leftMotor.Stop();
    rightMotor.Stop();
    //Reset counts
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    LCD.Clear(FEHLCD::Black);
    //Rest to ensure momentum stops
    Sleep(REST);
}

//Function definition for pivoting
void pivot(float degrees)
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
        while (leftEncoder.Counts() < x)
        {
            rightMotor.SetPercent(-MOVE);
            leftMotor.SetPercent(MOVE);
        }
    } else if (degrees < 0)
    {
        //Turn left for the number of counts
        while (rightEncoder.Counts() < x)
        {
            rightMotor.SetPercent(MOVE);
            leftMotor.SetPercent(-MOVE);
        }
    }
    //Stop motors
    leftMotor.Stop();
    rightMotor.Stop();
    //Reset counts
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    //Rest to insure momentum stops
    LCD.Clear(FEHLCD::Black);
    Sleep(REST);
}

//Function definition for checking the color that the CdS cell sees
int cdsColor()
{
    LCD.Clear(FEHLCD::Black);
    LCD.Write("Checking color from CdS Cell");
    /*Simple if checks to determine what color the CdS cell sees based off of measured ranges.
    Will change the LCD display to match the color it detects*/
    if (CdS.Value() > 0 && CdS.Value() <= 1.2)
    {
        LCD.Clear(FEHLCD::Red);
        return 0;
    } else if (CdS.Value() > 1.2 && CdS.Value() <= 2.4)
    {
        LCD.Clear(FEHLCD::Blue);
        return 1;
    } else if (CdS.Value() > 2.4 && CdS.Value() <= 3.3)
    {
        LCD.Clear(FEHLCD::Black);
        LCD.Write("No colored light detected");
        return 2;
    } else
    {
        return 3;
    }
}
