#ifndef M_DELAY_H
#define M_DELAY_H

#include "AudioStream.h"

template <int MQ>
class mDelay : public AudioStream
{
public:

  mDelay() : AudioStream(2, inputQueueArray), head(0), tail(0), numDelay(0){ begin(); }
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

  block1 = receiveReadOnly(0);
  block2 = receiveReadOnly(1);
  if (!block1 && !block2) return;
  
  h = head + 1;
  if (h >= MQ) h = 0;
  if (h == tail) {
    release(block1);
    release(block2);
  } else {
    queue1[h] = block1;
    queue2[h] = block2;
    head = h;
  }
}

#endif
