/* Audio Logger for Teensy 3.6
 * Copyright (c) 2018, Walter Zimmer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MPROCESS_H
#define MPROCESS_H

#include "kinetis.h"
#include "core_pins.h"

#include "AudioStream.h"

// adjust the following two definitions
#define MIN_BLOCKS 3 // defines min number of blocks to be send after detection
#define MIN_DELAY 30 // defines min number of blocks between two detections

//
int32_t aux[AUDIO_BLOCK_SAMPLES];

class mProcess: public AudioStream
{
public:

  mProcess(void) : AudioStream(2, inputQueueArray) {begin();}
  void begin(void);
  virtual void update(void);
  void setThreshold(int32_t val) {threshold=val;}
  int16_t getHaveSignal(void) {return haveSignal;}
  
protected:  
  audio_block_t *inputQueueArray[2];

private:
  int16_t haveSignal;
  int32_t maxVal, threshold;
  
  audio_block_t *out1, *out2;

  uint32_t blockCount;
};
 
void mProcess::begin(void)
{
  haveSignal=0;
  threshold=-1;
  out1=NULL;
  out2=NULL;
  blockCount=0;
}

inline void mDiff(int32_t *aux, int16_t *inp, int16_t ndat, int16_t old)
{
  aux[0]=(inp[0]-old);
  for(int ii=1; ii< ndat; ii++) aux[ii]=(inp[ii] - inp[ii-1]);  
}

inline int32_t mSig(int32_t *aux, int16_t ndat, int32_t maxVal)
{
  for(int ii=0; ii< ndat; ii++)
  { aux[ii] *= aux[ii];
    if(aux[ii]>maxVal) maxVal=aux[ii];
  }
  return maxVal;
}

void mProcess::update(void)
{
  audio_block_t *inp1, *inp2, *tmp1, *tmp2;
  inp1=receiveReadOnly(0);
  inp2=receiveReadOnly(1);
  if(!inp1 && !inp2) return;
  blockCount++;
  
  tmp1=allocate();
  tmp2=allocate();

  // store data and release input buffers
  for(int ii=0; ii< AUDIO_BLOCK_SAMPLES; ii++)
  {
    tmp1->data[ii]=inp1->data[ii];
    tmp2->data[ii]=inp2->data[ii];
  }
  release(inp1);
  release(inp2);

  // do here something useful with data 
  // example is a simple thresold detector on both channels
  // simple high-pass filter (6 db/octave)
  // followed by threshold detector

  int16_t ndat = AUDIO_BLOCK_SAMPLES;
  //
  // first channel
  mDiff(aux, tmp1->data, ndat, out1? out1->data[ndat-1]: tmp1->data[0]);
  maxVal = mSig(aux, ndat, 0);

  // second channel
  mDiff(aux, tmp2->data, ndat, out2? out2->data[ndat-1]: tmp2->data[0]);
  maxVal = mSig(aux, ndat, maxVal);

  // if threshold detector fires, the open transmisssion of input data for Min_Blocks
  // due to use of temprary storage, the block before detection will be transmitted first
  // that means 
  // MIN_BLOCKS ==2 means both, pre-trigger and trigger blocks are transmitted
  // MIN_BLOCKS ==3 means pre-trigger, trigger and a post-trigger blocks are transmitted
  // MIN_BLOCKS ==4 means pre-trigger, trigger and 2 post-trigger blocks are transmitted
  //
  // Re-triggering is possible
  // any detection during transmissions of blocks extends transmission
  //
  // 
  static uint32_t watchdog=0;
  static int16_t isFirst = 1;
  if((haveSignal<=MIN_DELAY) && (maxVal>=threshold))
  { haveSignal=MIN_BLOCKS;
  }
  if(haveSignal>0)
  { if(isFirst) // flag new data block (corrupting the first two words with blockCount)
    { if(out1) {out1->data[0]=-1; out1->data[1]=-1;}
      if(out2) {*(uint32_t*)(out2->data) = blockCount;}
      isFirst=0;
    }
    if(out1) transmit(out1,0);
    if(out2) transmit(out2,1);
  }
  // transmit anyway a single buffer
  if((haveSignal<0) && ((watchdog % MIN_DELAY)==0))
  { if(out1) {out1->data[0]=-1; out1->data[1]=-1;}
    if(out2) {*(uint32_t*)(out2->data) = blockCount;}
    if(out1) transmit(out1,0);
    if(out2) transmit(out2,1);
  }
  
  // reduce haveSignal to a minimal value providing the possibility of a guard window
  // between two detections
  // increment a watchdog counter that allows regular transmisson of noise
  // flag that next triggered transmission is first
  haveSignal--;
  if(haveSignal< -MIN_DELAY) { haveSignal = -MIN_DELAY; watchdog++; isFirst=1;}
  
  if(out1) release(out1);
  if(out2) release(out2);
  out1=tmp1;
  out2=tmp2;
}

 #endif