#include <FEHLCD.h>
#include <FEHUtility.h>
#include <FEHIO.h>
#include <FEHMotor.h>
#include <FEHServo.h>
#include <FEHRPS.h>
#include <math.h>

#DEFINE PI 3.1415
#DEFINE MOVE 50

//Declarations for IGWAN motors
FEHMotor leftMotor(FEHMotor::Motor0,9);
FEHMotor rightMotor(FEHMotor::Motor1,9);
//Declarations for the shaft encoders on the IGWAN motors
DigitalEncoder leftEncoder(FEHIO::P0_0);
DigitalEncoder rightEncoder(FEHIO::P0_1);
//Declaration for the CdS sensor
AnalogInputPin CdS(FEHIO::P0_2);

//Function prototype for moving a linear distance
void linearMove(float distance);

//Function prototype for checking what color the CdS cell sees
int cdsColor();

int main(void)
{

    float x,y;

    LCD.Clear(FEHLCD::Black);
    LCD.SetFontColor(FEHLCD::White);

    while(true)
    {
        if(LCD.Touch(&x,&y))
        {
            LCD.WriteLine("Hello World!");
            Sleep(100);
        }
    }

    return 0;
}

//Function definition for moving a linear distance, input is a linear distance
void linearMove(float distance)
{
    //Temp variable
    float x;
    //Converts distance input into the number of counts for the shaft encoder to move for
    x = (318.0/(2.5*PI))*abs(distance);
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
    leftEncoder.ResetCount();
}

//Function definition for checking the color that the CdS cell sees
int cdsColor()
{
    //Simple if checks to determine what color the CdS cell sees based off of measured ranges
    if (CdS.Value() > 0 && CdS.Value() <= 1.2)
    {
        LCD.Clear(FEHLCD::Black);
        LCD.Write("Red");
        return 0;
    } else if (CdS.Value() > 1.2 && CdS.Value() <= 2.4)
    {
        LCD.Clear(FEHLCD::Black);
        LCD.Write("Blue");
        return 1;
    } else if (CdS.Value() > 2.4 && CdS.Value() <= 3.3)
    {
        LCD.Clear(FEHLCD::Black);
        LCD.Write("Black");
        return 2;
    }
}
