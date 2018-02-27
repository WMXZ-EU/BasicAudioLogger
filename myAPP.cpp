/*
 * Basic High speed Stereo ADC logger
 * using Bill Greimans's SdFs on Teensy 3.6 
 * which must be downloaded from https://github.com/greiman/SdFs 
 * and installed as local library
 * 
 * uses Teensy Audio library for acquisition queuing 
 * audio tool ADC is modified to sample at other than 44.1 kHz
 * can use Stereo ADC (i.e. both ADCs of a Teensy)
 * single ADC may operate in differential mode
 * 
 * audio tool I2S is modified to sample at other than 44.1 kHz
 * can use quad I2S
 *
 * allow scheduled acquisition with hibernate during off times
 * 
 */
#include "core_pins.h"
#define F_SAMP 384000 // desired sampling frequency

// possible ACQ interfaces
#define _ADC_0		0	// single ended ADC0
#define _ADC_D		1	// differential ADC0
#define _ADC_S		2	// stereo ADC0 and ADC1
#define _I2S		3	// I2S (stereo audio)
#define _I2S_QUAD	4	// I2S (quad audio)

#define ACQ _I2S_QUAD	// selected acquisition interface

// For ADC SE pins can be changed
#if ACQ == _ADC_0
	#define ADC_PIN A2 // can be changed
	#define DIFF 0
#elif ACQ == _ADC_D
    #define ADC_PIN A10 //fixed
	#define DIFF 1
#elif ACQ == _ADC_S
	#define ADC_PIN1 A2 // can be changed
	#define ADC_PIN2 A3 // can be changed
	#define DIFF 0
#endif

// scheduled acquisition
// T1 to T3 are increasing hours, T4 can be before or after midnight
// choose for continuous recording {0,12,12,24}
typedef struct
{	uint16_t on;	// acquisition on time in seconds
	uint16_t ad;	// acquisition file size in seconds
	uint16_t ar;	// acquisition rate, i.e. every ar seconds
	uint16_t T1,T2; // first aquisition window (from T1 to T2) in Hours of day
	uint16_t T3,T4; // second aquisition window (from T1 to T2) in Hours of day
} ACQ_Parameters_s;

// Example
//ACQ_Parameters_s acqParameters = {120, 60, 180, 0, 12, 12, 24};
// acquire 2 files each 60 s long (totalling 120 s)
// sleep for 60 s (to reach 180 s acquisition interval)
// acquire whole day (from midnight to noon and noot to midnight)
//
ACQ_Parameters_s acqParameters = { 120, 60, 180, 0, 12, 12, 24 };

//==================== Audio interface ========================================
/*
 * standard Audio Interface
 * to avoid loading stock SD library
 * NO Audio.h is called but required header files are called directly
 * the custom multiplex object expects the 'link' to queue update function
 */
#define MQUEU 550
#if (ACQ == _ADC_0) | (ACQ == _ADC_D)
  #include "input_adc.h"
  AudioInputAnalog    acq(ADC_PIN);
  #include "m_queue.h"
  mRecordQueue<int16_t, MQUEU> queue1;
  AudioConnection     patchCord1(acq, queue1);
#elif ACQ == _ADC_S
  #include "input_adcs.h"
  AudioInputAnalogStereo  acq(ADC_PIN1,ADC_PIN2);
	#include "m_queue.h"
	mRecordQueue<MQUEU> queue1;
	#include "audio_multiplex.h"
  static void myUpdate(void) { queue1.update(); }
  AudioStereoMultiplex    mux1((Fxn_t)myUpdate());
  AudioConnection     patchCord1(acq,0, mux1,0);
  AudioConnection     patchCord2(acq,1, mux1,1);
  AudioConnection     patchCord3(mux1, queue1);
#elif ACQ == _I2S
  #include "input_i2s.h"
  AudioInputI2S         acq;

	#include "m_queue.h"
	mRecordQueue<int16_t, MQUEU> queue1;
	#include "audio_multiplex.h"
  static void myUpdate(void) { queue1.update(); }
  AudioStereoMultiplex  mux1((Fxn_t)myUpdate());
  AudioConnection     patchCord1(acq,0, mux1,0);
  AudioConnection     patchCord2(acq,1, mux1,1);
  AudioConnection     patchCord3(mux1, queue1);
#elif ACQ == _I2S_QUAD

  #include "input_i2s_quad.h"
  AudioInputI2SQuad     acq;
  #include "m_queue.h"
  mRecordQueue<int16_t, MQUEU> queue1;
  #include "audio_multiplex.h"
  static void myUpdate(void) { queue1.update(); }

  AudioQuadMultiplex    mux1((Fxn_t)myUpdate);
  AudioConnection     patchCord1(acq,0, mux1,0);
  AudioConnection     patchCord2(acq,1, mux1,1);
  AudioConnection     patchCord3(acq,2, mux1,2);
  AudioConnection     patchCord4(acq,3, mux1,3);
  AudioConnection     patchCord5(mux1, queue1);
#else
  #error "invalid acquisition device"
#endif

#include "audio_mods.h"
#include "audio_logger_if.h"
#include "audio_hibernate.h"

extern "C"
{ void setup(void);
  void loop(void);
}

// led only allowed if NO I2S
void ledOn(void)
{
  #if (ACQ == _ADC_0) | (ACQ == _ADC_D) | (ACQ == _ADC_S)
    pinMode(13,OUTPUT);
    digitalWriteFast(13,HIGH);
  #endif
}
void ledOff(void)
{
  #if (ACQ == _ADC_0) | (ACQ == _ADC_D) | (ACQ == _ADC_S)
    digitalWriteFast(13,LOW);
  #endif
}

//__________________________General Arduino Routines_____________________________________
void setup() {
  // put your setup code here, to run once:

	AudioMemory (600);
  // check is it is our time to record
//  checkDutyCycle(&acqParameters, -1); // will not return if if sould not continue with acquisition 
  ledOn();
  while(!Serial && (millis()<3000));
  ledOff();
  //
  uSD.init();

  #if (ACQ == _ADC_0) | (ACQ == _ADC_D) | (ACQ == _ADC_S)
    ADC_modification(F_SAMP,DIFF);
  #elif (ACQ == _I2S)
    I2S_modification(F_SAMP,32);
  #elif (ACQ == _I2S_QUAD)
    I2S_modification(F_SAMP,16); // I2S_Quad not modified for 32 bit
  #endif

  queue1.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
 static int16_t state=0; // 0: open new file, -1: last file

 if(queue1.available())
 {
    if ((checkDutyCycle(&acqParameters, state))<0)  // this also triggers closing files and hibernating, if planned
    { uSD.setClosing();
//      Serial.println(state);
    }
   if(state==0)
   { // generate header before file is opened
      uint32_t *header=(uint32_t *) headerUpdate();
      uint32_t *ptr=(uint32_t *) outptr;
      // copy to disk buffer
      for(int ii=0;ii<128;ii++) ptr[ii] = header[ii];
      outptr+=256; //(512 bytes)
      state=1;
   }
  // fetch data from queue
  int32_t * data = (int32_t *)queue1.readBuffer();
  //
  // copy to disk buffer
  uint32_t *ptr=(uint32_t *) outptr;
  for(int ii=0;ii<64;ii++) ptr[ii] = data[ii];
  queue1.freeBuffer(); 
  //
  // adjust buffer pointer
  outptr+=128; // (128 shorts)
  //
  // if necessary reset buffer pointer and write to disk
  // buffersize should be always a multiple of 512 bytes
  if(outptr == (diskBuffer+BUFFERSIZE))
  {
    outptr = diskBuffer;

    // write to disk ( this handles also opening of files)
    if(state>=0)
      state=uSD.write(diskBuffer,BUFFERSIZE); // this is blocking
  }
//  if(!state) Serial.println("closed");
 }
 
 // some statistics on progress
 static uint32_t t0;
 if(millis()>t0+1000)
 {  Serial.printf("loop: %4d %5d;",uSD.getNbuf(),AudioMemoryUsageMax());
  #if (ACQ==_ADC_0) | (ACQ==_ADC_D) | (ACQ==_ADC_S)
    Serial.printf("%5d %5d",PDB0_CNT, PDB0_MOD);
  #endif
    Serial.println();
    AudioMemoryUsageMaxReset();
    t0=millis();
 }
}
