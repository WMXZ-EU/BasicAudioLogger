#ifndef M_DELAY_H
#define M_DELAY_H

#include "AudioStream.h"

template <int MQ>
class mDelay : public AudioStream
{
public:

  mDelay(int del) : AudioStream(2, inputQueueArray), head(0), tail(0), numDelay(0){ numDelay=del; }
  void setDelay(int16_t ndel) {numDelay=ndel;}
  virtual void update(void);
  
protected:  
  audio_block_t *inputQueueArray[2];

private:
  audio_block_t * volatile queue1[MQ];
  audio_block_t * volatile queue2[MQ];
  volatile uint8_t head, tail, numDelay;

};

template <int MQ>
void mDelay<MQ>::update(void)
{
  audio_block_t *block1, *block2;
  uint32_t h;

  if(numDelay > MQ) return; // error don't do anything
  //
  block1 = receiveReadOnly(0);
  block2 = receiveReadOnly(1);
  if (!block1 || !block2) return; // expect both data channels
  
  h = (head + 1) % MQ;
  if (h == tail) {  // should ot happen
    release(block1);
    release(block2);
  } else {
    queue1[h] = block1;
    queue2[h] = block2;
    head = h;
  }

  uint8_t index = (head - numDelay + MQ) % MQ;

  if(queue1[index] && queue2[index])
  {
      transmit(queue1[index],0);
      transmit(queue2[index],1);
      release(queue1[index]);
      release(queue2[index]);
      queue1[index]=NULL;
      queue2[index]=NULL;    
  }
}

#endif
