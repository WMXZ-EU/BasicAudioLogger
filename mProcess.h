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

#define MIN_BLOCKS 3 // defines min number of blocks to be send after detection

int32_t aux[AUDIO_BLOCK_SAMPLES];

class mProcess: public AudioStream
{
public:

  mProcess(void) : AudioStream(2, inputQueueArray) {begin();}
  void begin(void);
  virtual void update(void);
  void setThreshold(int32_t val) {threshold=val;}
  
protected:  
  audio_block_t *inputQueueArray[2];

private:
  int16_t haveSignal;
  int32_t maxVal, threshold;
  
  audio_block_t *out1, *out2;
};
 
void mProcess::begin(void)
{
  haveSignal=0;
  threshold=-1;
  out1=NULL;
  out2=NULL;
}

void mProcess::update(void)
{
  audio_block_t *inp1, *inp2, *tmp1, *tmp2;
  inp1=receiveReadOnly(0);
  inp2=receiveReadOnly(1);
  tmp1=allocate();
  tmp2=allocate();

  // do here something usefull with data
  for(int ii=0; ii< AUDIO_BLOCK_SAMPLES; ii++)
  {
    tmp1->data[ii]=inp1->data[ii];
    tmp2->data[ii]=inp2->data[ii];
  }
  release(inp1);
  release(inp2);

  // simple high-pass filter (6 db/octave)
  // fllowed by threshold detector

  maxVal=0;
  //
  // first channel
  if(out1)
    aux[0]=(tmp1->data[0]-out1->data[AUDIO_BLOCK_SAMPLES-1]);
  else
    aux[0]=0;
  for(int ii=1; ii< AUDIO_BLOCK_SAMPLES; ii++)
  { aux[ii]=(tmp1->data[ii]-tmp1->data[ii-1]);
    aux[ii] *= aux[ii];
  }
  for(int ii=0; ii< AUDIO_BLOCK_SAMPLES; ii++)
  { if(aux[ii]>maxVal) maxVal=aux[ii];
  }
  // second channel
  if(out2)
    aux[0]=(tmp2->data[0]-out2->data[AUDIO_BLOCK_SAMPLES-1]);
  else
    aux[0]=0;
  for(int ii=1; ii< AUDIO_BLOCK_SAMPLES; ii++)
  { aux[ii]=(tmp2->data[ii]-tmp2->data[ii-1]);
    aux[ii] *= aux[ii];
  }
  for(int ii=0; ii< AUDIO_BLOCK_SAMPLES; ii++)
  { if(aux[ii]>maxVal) maxVal=aux[ii];
  }
  
  if(maxVal>=threshold) haveSignal=MIN_BLOCKS;
  if(haveSignal>=0)
  { haveSignal--;
    if(out1) transmit(out1,0);
    if(out2) transmit(out2,1);
  }
  if(out1) release(out1);
  if(out2) release(out2);
  out1=tmp1;
  out2=tmp2;
}

 #endif
