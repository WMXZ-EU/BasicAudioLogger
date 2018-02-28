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

#define MIN_BLOCKS 2 // defines min number of blocks to be send after detection

class mProcess: public AudioStream
{
public:

  mProcess(void) : AudioStream(2, inputQueueArray) {begin();}
  void begin(void);
  virtual void update(void);
  
protected:  
  audio_block_t *inputQueueArray[2];

private:
uint16_t haveSignal;
};
 
void mProcess::begin(void)
{
  haveSignal=0;
}

void mProcess::update(void)
{
  audio_block_t *inp1, *inp2, *out1, *out2;
  inp1=receiveReadOnly(0);
  inp2=receiveReadOnly(1);
  out1=allocate();
  out2=allocate();

  // do here something usefull with data
  for(int ii=0; ii< AUDIO_BLOCK_SAMPLES; ii++)
  {
    out1->data[ii]=inp1->data[ii];
    out2->data[ii]=inp2->data[ii];
  }
  release(inp1);
  release(inp2);

  // simulate detection
  if(1)
   haveSignal=MIN_BLOCKS;
  
  if(haveSignal>=0)
  { haveSignal--;
    transmit(out1,0);
    transmit(out2,1);
  }
    release(out1);
    release(out2);
}

 #endif
