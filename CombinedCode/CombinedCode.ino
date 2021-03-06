#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Vcc.h>
#include <binary.h>
#include <Adafruit_LSM303.h>

#define PIN            6
#define NUMPIXELS      2
#define NIGHTLEVEL    10
#define ledPin        12
#define pulsePin       0
#define lightpin       3

#define aref_voltage 3.3 

//Accelerometer
Adafruit_LSM303 lsm;

//Temp sensor
int tempPin = 9;

//Pulse sensor
//int ledPin = 10;
int dPin = 12;
int lightledpin;

//Light sensor
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

//Battery life
const float VccMin   = 0.0;
const float VccMax   = 4.5;
const float VccCorrection = 1.0/1.0;
Vcc vcc(VccCorrection);

//Byte output
byte data = 0;
boolean byte_flag = true;
boolean step_taken;
int body_temp;
boolean danger_location;
boolean danger_hit;

volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded! 
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat". 
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

volatile int rate[10];                    // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find IBI
volatile int P =512;                      // used to find peak in pulse wave, seeded
volatile int T = 512;                     // used to find trough in pulse wave, seeded
volatile int thresh = 525;                // used to find instant moment of heart beat, seeded
volatile int amp = 100;                   // used to hold amplitude of pulse waveform, seeded
volatile boolean firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = false;      // used to seed rate array so we startup with reasonable BPM

//Light sensor
void configureSensor(void)
{
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
}

void setup(void) 
{
  Serial.begin(9600);
  
  //Accelerometer
  lsm.begin();
  
  //Temp sensor
  analogReference(EXTERNAL);
  
  //Pulse sensor
  pinMode(ledPin, INPUT);
  pinMode(dPin, OUTPUT);
  
  //Light sensor
  if (!tsl.begin())
    while(1);
  pixels.begin();
  configureSensor();
  
  TCCR2A = 0x02;     // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
  TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER 
  OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
  sei();             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED    
  
}

void loop(void) 
{ 
  /****************************************************************************
   Accelerometer Code
   ****************************************************************************/   
    
  //Read the acceleration data from accelerometer   
  int y_accel = analogRead(ledPin);
  int z_accel = analogRead(ledPin);
  
  //Threshold to predict whether a step has been taken
  if (y_accel > 270 && z_accel < 380)
  {
    step_taken = 1;
  }
  else
  {
    step_taken = 0;
  }
  
  /***************************************************************************
   Temperature Sensor Code
  ****************************************************************************/
  int tempReading = analogRead(tempPin);  
  // converting that reading to voltage, which is based off the reference voltage
  float voltage = tempReading * aref_voltage;
  voltage /= 1024.0; 
  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
                                               //to degrees ((volatge - 500mV) times 100)
  float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
  
  body_temp = (int) (10 * (temperatureF - 95.0));
  
  /*********************************** 
   Pulse Sensor Code
  ************************************/
  boolean pulse;
  int x = analogRead(ledPin);
  if (x == 525)
  {
    pulse = true;
    digitalWrite(dPin, HIGH);
  }
  else if (x <= 525)
  {
    pulse = false;
    digitalWrite(dPin, LOW);
  }
  
  /***********************************
   Byte output
   ***********************************/
  data = 0;
  // Transfer the byte with BPM information
  if (byte_flag) {
    data = data | step_taken | (body_temp << 1) | (danger_hit << 6) | (danger_location << 7);
  }
  else {
  // Transfer the byte with temperature information
    data = data | step_taken | (BPM << 1) | (danger_hit << 6) | (danger_location << 7);
  }
  // Flip byte flag
  byte_flag = !byte_flag;
  // Print to console
  for (int i = 7; i >= 0; i--) {
    Serial.print(bitRead(data, i)); 
  }
  Serial.println();
  // Alternate byte every half second
  //delay(500);
  
  /***********************************
   Light Sensor/Battery Life Code
  ************************************/
  
  /*
  sensors_event_t event;
  tsl.getEvent(&event);
  
  float p = vcc.Read_Perc(VccMin, VccMax);
  if (event.light > NIGHTLEVEL)
  {
    pixels.setPixelColor(0, (0, 0, 0));
  }
  else
  {
    pixels.setPixelColor(0, (255, 255, 255));
  }
  pixels.setPixelColor(1,(p/100 * 255, p/100 * 255, p/100 * 255));
  pixels.show();
  */
  /*
  float p = vcc.Read_Perc(VccMin, VccMax);
  int light_val = analogRead(lightpin);
  
  if (event.light > 100)
  {
    pixels.setPixelColor(0, (0, 0, 0));
  }
  else
  {
    pixels.setPixelColor(0, (255, 255, 255))
  }
  pixels.setPixelColor(1,(p/100 * 255, p/100 * 255, p/100 * 255));
  pixels.show(); 
  */
  
}

/********************************************************************
                    Interrupt service routine 
*********************************************************************/
// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE. 
// Timer 2 makes sure that we take a reading every 2 miliseconds
ISR(TIMER2_COMPA_vect){                         // triggered when Timer2 counts to 124
  cli();                                      // disable interrupts while we do this
  Signal = analogRead(pulsePin);              // read the Pulse Sensor 
  sampleCounter += 2;                         // keep track of the time in mS with this variable
  int N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to avoid noise

    //  find the peak and trough of the pulse wave
  if(Signal < thresh && N > (IBI/5)*3){       // avoid dichrotic noise by waiting 3/5 of last IBI
    if (Signal < T){                        // T is the trough
      T = Signal;                         // keep track of lowest point in pulse wave 
    }
  }

  if(Signal > thresh && Signal > P){          // thresh condition helps avoid noise
    P = Signal;                             // P is the peak
  }                                        // keep track of highest point in pulse wave

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
  if (N > 250){                                   // avoid high frequency noise
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) ){        
      Pulse = true;                               // set the Pulse flag when we think there is a pulse
      digitalWrite(ledPin,HIGH);                // turn on pin 13 LED

      IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
      lastBeatTime = sampleCounter;               // keep track of time for next pulse

      if(secondBeat){                        // if this is the second beat, if secondBeat == TRUE
        secondBeat = false;                  // clear secondBeat flag
        for(int i=0; i<=9; i++){             // seed the running total to get a realisitic BPM at startup
          rate[i] = IBI;                      
        }
      }

      if(firstBeat){                         // if it's the first time we found a beat, if firstBeat == TRUE
        firstBeat = false;                   // clear firstBeat flag
        secondBeat = true;                   // set the second beat flag
        sei();                               // enable interrupts again
        return;                              // IBI value is unreliable so discard it
      }   


      // keep a running total of the last 10 IBI values
      word runningTotal = 0;                  // clear the runningTotal variable    

      for(int i=0; i<=8; i++){                // shift data in the rate array
        rate[i] = rate[i+1];                  // and drop the oldest IBI value 
        runningTotal += rate[i];              // add up the 9 oldest IBI values
      }

      rate[9] = IBI;                          // add the latest IBI to the rate array
      runningTotal += rate[9];                // add the latest IBI to runningTotal
      runningTotal /= 10;                     // average the last 10 IBI values 
      BPM = 60000/runningTotal;               // how many beats can fit into a minute? that's BPM!
      QS = true;                              // set Quantified Self flag 
      // QS FLAG IS NOT CLEARED INSIDE THIS ISR
      Serial.println(BPM);

    }                       
  }

  if (Signal < thresh && Pulse == true){   // when the values are going down, the beat is over
//    digitalWrite(blinkPin,LOW);            // turn off pin 13 LED
    Pulse = false;                         // reset the Pulse flag so we can do it again
    digitalWrite(ledPin, LOW);
    amp = P - T;                           // get amplitude of the pulse wave
    thresh = amp/2 + T;                    // set thresh at 50% of the amplitude
    P = thresh;                            // reset these for next time
    T = thresh;
  }

  if (N > 2500){                           // if 2.5 seconds go by without a beat
    thresh = 512;                          // set thresh default
    P = 512;                               // set P default
    T = 512;                               // set T default
    lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date        
    firstBeat = true;                      // set these to avoid noise
    secondBeat = false;                    // when we get the heartbeat back
  }


  sei();                                   // enable interrupts when youre done!
}// end isr
