#include <Stepper.h>

const int SLIDER_MOTOR_CW = 1;
const int MOTOR_STOP = 0;
const int SLIDER_MOTOR_CCW = -1;

const int WINDER_MOTOR_STEPS = 200;
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
float fCheckValue;
int iDelayParameter;

int nCount;
int nCountOfStep;
int nCountOfLayers;
float fWindingErrorSum;

Stepper stepperWinder(WINDER_MOTOR_STEPS, 9, 10, 11, 12);
Stepper stepperSlider(SLIDER_MOTOR_STEPS, 5, 6, 7, 8);

void setup()
{  
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // set the speed of the motor
  stepperWinder.setSpeed(75);
  stepperSlider.setSpeed(75);

  // To Initialize variables
  fCoilHeight = 0;
  iTurnsPerOneLayer = 0;
  iLayers = 0;
  iWindingSpeed = 100;
  
  iWindingCount = 0;
  fWindingError = 0;
  fCheckValue = 0;
    iDelayParameter = 0;
  
  iWinderStepPerOneLayer = 0;
  iSliderStepPerOneLayer = 10;  
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
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);

  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
}

void loop() {

  float fDistancePerOneTurnOfSlider = 0.5;
  float fStepRatio;
  int iStepRatioQuotient;
  float fStepRatioRemainder;
  float fMaxSpeed;
  int i, iTempData;

  byte btReceiveByte = 0;
  String strTemp = "";

  //----------------------------------------------------------------------------
  // Handling communiction commands
  //----------------------------------------------------------------------------
  if (Serial.available() > 0)
  {
    btReceiveByte = Serial.read();

    // Number letter
    if(isDigit(btReceiveByte))
    {
      // Number letter is used only Number Mode.
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

        // Check the end of the number transfer
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
    // Command letter
    else
    {
      switch (btReceiveByte)
      {
        case 's':
          // To calcurate the motor step of a layer
          iWinderStepPerOneLayer = iTurnsPerOneLayer * WINDER_MOTOR_STEPS;
          iSliderStepPerOneLayer = (int)(fCoilHeight / fDistancePerOneTurnOfSlider * SLIDER_MOTOR_STEPS);
          
          // To calcurate the ratio between a slider motor and a winder motor
          fStepRatio = (float)iWinderStepPerOneLayer / iSliderStepPerOneLayer;
          iStepRatioQuotient = (int)fStepRatio;
          fStepRatioRemainder= fStepRatio - iStepRatioQuotient;

          // To calcurate numbers of winding
          iWindingCount = iStepRatioQuotient;
          fWindingError = fStepRatioRemainder / iStepRatioQuotient;
          fCheckValue = 1 / (float)iStepRatioQuotient;

/*---------------------- Method 2 -------------------------
          // To calcurate numbers for winding
          iWindingCount = iStepRatioQuotient + 1;
          fWindingError = (1 - fStepRatioRemainder) / fStepRatio;
-----------------------------------------------------------*/

          // Limitation of Maximum speed.
          fMaxSpeed = 60/((float)(2 + 2)* WINDER_MOTOR_STEPS) * 1000;      // Speed is RPM
          if(iWindingSpeed > fMaxSpeed) iWindingSpeed = (int)fMaxSpeed;

          // To calcurate the delay time for controling speed.
          iDelayParameter = 60/((float)iWindingSpeed * WINDER_MOTOR_STEPS) * 1000 - (2 + 2);
          if(iDelayParameter < 0) iDelayParameter = 0;          
        
          // It is important to reset the speed of the winder motor even using delay time for speed control.
          stepperWinder.setSpeed(iWindingSpeed);

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
              iDirection = SLIDER_MOTOR_CW;              // normal starting
            
            bVelocityBit = false;
            bLayerBit = false;
            bTurnsOfOneLayerBit = false;
            bCoilHeightBit = false;        
          }    
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
          stepperSlider.step(-SLIDER_MOTOR_STEPS*2*2);    // SLIDER_MOTOR_STEPS : 0.5mm, X2 : 1mm, X2 : 2mm
          break;

        case 'r':
          stepperSlider.step(SLIDER_MOTOR_STEPS*2*2);     // SLIDER_MOTOR_STEPS : 0.5mm, X2 : 1mm, X2 : 2mm
          break;

        case 'm':
          stepperSlider.step(-8);    // SLIDER_MOTOR_STEPS (20) : 0.5mm, 8 : 0.2mm
          break;

        case 'n':
          stepperSlider.step(8);     // SLIDER_MOTOR_STEPS (20) : 0.5mm, 8 : 0.2mm
          break;

        case 'p':
          if(iDirection != MOTOR_STOP)
          {
            iDirectionBackup = iDirection;            
            iDirection = MOTOR_STOP;
            bPauseBit = true;
            resetMotors();          // To prevent entering the power during stop period.
          }            
          break;

        case 'q':
          iDirection = MOTOR_STOP;
          nCountOfLayers = 0;
          nCountOfStep = 0;
          fWindingErrorSum = 0;
          nCount =0;
          resetMotors();            // To prevent entering the power during stop period.
          break;

        case 'o':
          Serial.println("o");
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
  if( iDirection == SLIDER_MOTOR_CW || iDirection == SLIDER_MOTOR_CCW )
  { 
    // Winder Motor rotates one step for every loop.
    stepperWinder.step(-1);
    
    nCount ++;
    
    // Slider Motor rotates one step at a time for each count. 
    if(nCount >= iWindingCount)
    { 
      // Wait the slilder step for compensating the error.
      if( fWindingErrorSum < fCheckValue )
      {
        if( iDirection == SLIDER_MOTOR_CW ) stepperSlider.step(1);
        if( iDirection == SLIDER_MOTOR_CCW ) stepperSlider.step(-1);

        fWindingErrorSum = fWindingErrorSum + fWindingError;    

        nCount = 0;
      }
      else
      {
        fWindingErrorSum = (fWindingErrorSum - fCheckValue);
        nCount = nCount - 1;
      }
    }
    
/*---------------------- Method 2 -------------------------
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
--------------------------------------------------------*/
    // Speed control using delay time.
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
        if( iDirection == SLIDER_MOTOR_CW ) iDirection = SLIDER_MOTOR_CCW;
        else if( iDirection == SLIDER_MOTOR_CCW ) iDirection = SLIDER_MOTOR_CW;
      }

      nCountOfStep = 0;
      fWindingErrorSum = 0;
      nCount =0;
    }
  } 
  //----------------------------------------------------------------------------  
  
}


