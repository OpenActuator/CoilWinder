#include <Stepper.h>

const int MOTOR_CW = 1;
const int MOTOR_STOP = 0;
const int MOTOR_CCW = -1;

const int WINDER_MOTOR_STEPS = 48;
const int SLIDER_MOTOR_STEPS = 20;

const int C_MODE = 11;
const int N_MODE = 12;

byte btCommand;
int iDirection;
int iDirectionBackup;
int iMode;

bool bVelocityBit;
bool bLayerBit;
bool bTurnsOfOneLayerBit;
bool bCoilHeightBit;
bool bPauseBit;
byte nNumberDataCount;
int iNumberData;

float fCoilHeight;
int iLayers;
int iTurnsPerOneLayer;
int iWindingSpeed;

int iWinderStepPerOneLayer;
int iSliderStepPerOneLayer;

int iWindingCount;
float fWindingError;
int iDelayParameter;

int nCount;
int nCountOfStep;
int nCountOfLayers;
float fWindingErrorSum;

Stepper stepperWinder(WINDER_MOTOR_STEPS, 8, 9, 10, 11);
Stepper stepperSilder(SLIDER_MOTOR_STEPS, 4, 5, 6, 7);

void setup()
{
  
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // set the speed of the motor to 30 RPMs
  stepperSilder.setSpeed(200);
  stepperWinder.setSpeed(200);

  fCoilHeight = 2;
  iTurnsPerOneLayer = 20;
  iLayers = 2;
  iWindingSpeed = 200;       // This speed is RPM of the winder motor

  // To Initialize variables
  iWindingCount = 0;
  fWindingError = 0;
  iDelayParameter = 0;
  iWinderStepPerOneLayer = 0;
  iSliderStepPerOneLayer = 0;  
  nCount = 0;
  nCountOfStep = 0;
  nCountOfLayers = 0;
  fWindingErrorSum = 0;

  iDirection = MOTOR_STOP;
  iDirectionBackup = MOTOR_STOP;
  iMode = C_MODE;

  bVelocityBit = false;
  bLayerBit = false;
  bTurnsOfOneLayerBit = false;
  bCoilHeightBit = false;
  bPauseBit = false;
  
  nNumberDataCount = 0;
  iNumberData = 0;
 
}

void resetMotors()
{
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
}

void loop() {

  float fDistancePerOneTurnOfSlider = 0.5;
  float fStepRatio;
  int iStepRatioQuotient;
  float fStepRatioRemainder;
  int i;
  int iTempData;

  byte btReceiveByte = 0;
  String strTemp = "";

  //----------------------------------------------------------------------------
  // Handling communiction commands
  //----------------------------------------------------------------------------
  if (Serial.available() > 0)
  {
    btReceiveByte = Serial.read();

    if(isDigit(btReceiveByte))
    {
      if(iMode == N_MODE)
      {
        strTemp = (char)btReceiveByte;
        switch (nNumberDataCount)
        {
          case 0:
            iTempData = strTemp.toInt() * 1000;
            break;
          case 1:
            iTempData = strTemp.toInt() * 100;
            break;
          case 2:
            iTempData = strTemp.toInt() * 10;
            break;
          case 3:
            iTempData = strTemp.toInt();
            break;
          default:
            Serial.println("Count Error of a Number Data");
            break;
        }
        
        iNumberData += iTempData;       
        nNumberDataCount ++;

        // Check the NuberDataCount after increasing.
        if( nNumberDataCount >= 4)
        {
          switch (btCommand)
          {
            case 'v':
              iWindingSpeed = iNumberData;
              bVelocityBit = true;
              break;
            case 'y':
              iLayers = iNumberData;
              bLayerBit = true;
              break;
            case 't':
              iTurnsPerOneLayer = iNumberData;
              bTurnsOfOneLayerBit = true;
              break;
            case 'h':
              fCoilHeight = (float)iNumberData / 100;
              bCoilHeightBit = true;
              break;
            default:
              Serial.println("Command Error while a Number Data is saving");
              break;
          }
          nNumberDataCount = 0;
          iMode = C_MODE;
     
        }
      }
      else
      {
        Serial.println("Received a Number Data during C_MODE");
      }
    }
    else
    {
      switch (btReceiveByte)
      {
        case 's':

          // To calcurate the motor step of a layer
          iWinderStepPerOneLayer = iTurnsPerOneLayer * WINDER_MOTOR_STEPS;
          iSliderStepPerOneLayer = (int)(fCoilHeight/fDistancePerOneTurnOfSlider * SLIDER_MOTOR_STEPS);

          if(iWinderStepPerOneLayer < iSliderStepPerOneLayer) 
          {
            Serial.println("The Winder Step is less than Slider Step.");
            break;
          }        

          // To calcurate the ratio between a slider motor and a winder motor
          fStepRatio = (float)iWinderStepPerOneLayer / iSliderStepPerOneLayer;
          iStepRatioQuotient = (int)fStepRatio;
          fStepRatioRemainder= fStepRatio - iStepRatioQuotient;

          // To calcurate numbers for winding
          iWindingCount = iStepRatioQuotient + 1;
          fWindingError = (1 - fStepRatioRemainder) / fStepRatio;
          iDelayParameter = 60000/(iWindingSpeed*WINDER_MOTOR_STEPS) - (1 + 1);   // step() 시간 : 2ms, Loop 시간 : 2ms

          if(iDelayParameter < 1) 
          {
            Serial.println("The Delay Parameter is 0.");
            break;
          }

          if(bPauseBit == true)
          {
            iDirection = iDirectionBackup;
            bPauseBit = false;            
          }

          if( bVelocityBit == true && bLayerBit == true && bTurnsOfOneLayerBit == true && bCoilHeightBit == true)
          {
            if(iDirection != MOTOR_STOP)
              Serial.println("Can't start the winder because the winder already is operating.");
            else
              iDirection = MOTOR_CW;              // normal starting
            
            bVelocityBit = false;
            bLayerBit = false;
            bTurnsOfOneLayerBit = false;
            bCoilHeightBit = false;        
          }
          else
            Serial.println("Data is not enough for starting.");            
          break;

        case 'v':
        case 'y':
        case 't':
        case 'h':

          btCommand = btReceiveByte;
          iMode = N_MODE;
          iNumberData = 0;
          nNumberDataCount = 0;
          break;

        case 'l':
          stepperSilder.step(-SLIDER_MOTOR_STEPS*2*2);    // SLIDER_MOTOR_STEPS : 0.5mm, X2 : 1mm, X2 : 2mm
          break;

        case 'r':
          stepperSilder.step(SLIDER_MOTOR_STEPS*2*2);     // SLIDER_MOTOR_STEPS : 0.5mm, X2 : 1mm, X2 : 2mm
          break;

        case 'm':
          stepperSilder.step(-8);    // SLIDER_MOTOR_STEPS (20) : 0.5mm, 8 : 0.2mm
          break;

        case 'n':
          stepperSilder.step(8);     // SLIDER_MOTOR_STEPS (20) : 0.5mm, 8 : 0.2mm
          break;

        case 'p':
          if(iDirection != MOTOR_STOP)
          {
            iDirectionBackup = iDirection;            
            iDirection = MOTOR_STOP;
            bPauseBit = true;
            resetMotors();          // To privent entering the power during stop period.
          }            
          break;

        case 'q':
          iDirection = MOTOR_STOP;
          nCountOfLayers = 0;
          nCountOfStep = 0;
          fWindingErrorSum = 0;
          nCount =0;
          resetMotors();            // To privent entering the power during stop period.
          break;

        default:
          Serial.println("Received a wrong command.");
          break;
      }
    }
  }
  //----------------------------------------------------------------------------  

  //----------------------------------------------------------------------------
  // Motor Rotateing
  //----------------------------------------------------------------------------
  if( iDirection == MOTOR_CW || iDirection == MOTOR_CCW )
  {  
    stepperWinder.step(-1);
    nCount ++;

    if(nCount >= iWindingCount)
    { 
      if( iDirection == MOTOR_CW ) stepperSilder.step(1);
      if( iDirection == MOTOR_CCW ) stepperSilder.step(-1);
      fWindingErrorSum += fWindingError;
      nCount = 0;
    }

    if(fWindingErrorSum >= 1)
    {
      if( iDirection == MOTOR_CW ) stepperSilder.step(1);
      if( iDirection == MOTOR_CCW ) stepperSilder.step(-1);
      fWindingErrorSum = 0;
    }

    delay(iDelayParameter);
    nCountOfStep ++;

    if(nCountOfStep >= iWinderStepPerOneLayer)
    {
      nCountOfLayers ++;

      if(nCountOfLayers >= iLayers)
      {
        iDirection = MOTOR_STOP;
        nCountOfLayers = 0;
        resetMotors();
      }
      else
      {
        if( iDirection == MOTOR_CW ) iDirection = MOTOR_CCW;
        else if( iDirection == MOTOR_CCW ) iDirection = MOTOR_CW;
      }

      nCountOfStep = 0;
      fWindingErrorSum = 0;
      nCount =0;
    }
  } 
  //----------------------------------------------------------------------------  
  
}


