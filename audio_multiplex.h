#ifndef _AUDIO_MULTIPLEX_H
#define _AUDIO_MULTIPLEX_H
#include "kinetis.h"
#include "core_pins.h"
#include "AudioStream.h"

typedef void(*Fxn_t)(void);

class AudioStereoMultiplex: public AudioStream
{
public:
	AudioStereoMultiplex(Fxn_t Fx) : AudioStream(2, inputQueueArray) { fx = Fx; }
  virtual void update(void);
private:
	Fxn_t fx;
  audio_block_t *inputQueueArray[2];
};

void AudioStereoMultiplex::update(void)
{
  audio_block_t *inp1, *inp2, *out1, *out2;
  inp1=receiveReadOnly(0);
  inp2=receiveReadOnly(1);
  out1=allocate();
  out2=allocate();

  int jj=0;
  for(int ii=0; ii< AUDIO_BLOCK_SAMPLES/2; ii++)
  {
    out1->data[jj++]=inp1->data[ii];
    out1->data[jj++]=inp2->data[ii];
  }
  
  jj=0;
  for(int ii=AUDIO_BLOCK_SAMPLES/2; ii< AUDIO_BLOCK_SAMPLES; ii++)
  {
    out2->data[jj++]=inp1->data[ii];
    out2->data[jj++]=inp2->data[ii];
  }

  release(inp1);
  release(inp2);
  
  transmit(out1);
  release(out1); fx();
  transmit(out2);
  release(out2);
}

class AudioQuadMultiplex: public AudioStream
{
public:
	AudioQuadMultiplex(Fxn_t Fx) : AudioStream(4, inputQueueArray) { fx = Fx; }
	virtual void update(void);
private:
	Fxn_t fx;
  audio_block_t *inputQueueArray[4];
};

void AudioQuadMultiplex::update(void)
{
  audio_block_t *inp1, *inp2, *inp3, *inp4;
  audio_block_t *out1, *out2, *out3, *out4;
  inp1=receiveReadOnly(0);
  inp2=receiveReadOnly(1);
  inp3=receiveReadOnly(2);
  inp4=receiveReadOnly(3);
  out1=allocate();
  out2=allocate();
  out3=allocate();
  out4=allocate();

  int jj=0;
  for(int ii=0; ii< AUDIO_BLOCK_SAMPLES/4; ii++)
  {
    out1->data[jj++]=inp1->data[ii];
    out1->data[jj++]=inp2->data[ii];
    out1->data[jj++]=inp3->data[ii];
    out1->data[jj++]=inp4->data[ii];
  }
  jj=0;
  for(int ii=AUDIO_BLOCK_SAMPLES/4; ii< AUDIO_BLOCK_SAMPLES/2; ii++)
  {
    out2->data[jj++]=inp1->data[ii];
    out2->data[jj++]=inp2->data[ii];
    out2->data[jj++]=inp3->data[ii];
    out2->data[jj++]=inp4->data[ii];
  }
  jj=0;
  for(int ii=AUDIO_BLOCK_SAMPLES/2; ii< 3*AUDIO_BLOCK_SAMPLES/4; ii++)
  {
    out3->data[jj++]=inp1->data[ii];
    out3->data[jj++]=inp2->data[ii];
    out3->data[jj++]=inp3->data[ii];
    out3->data[jj++]=inp4->data[ii];
  }
  jj=0;
  for(int ii=3*AUDIO_BLOCK_SAMPLES/4; ii< AUDIO_BLOCK_SAMPLES; ii++)
  {
    out4->data[jj++]=inp1->data[ii];
    out4->data[jj++]=inp2->data[ii];
    out4->data[jj++]=inp3->data[ii];
    out4->data[jj++]=inp4->data[ii];
  }

  release(inp1);
  release(inp2);
  release(inp3);
  release(inp4);
  
  transmit(out1);
  release(out1); fx();
  transmit(out2);
  release(out2); fx();
  transmit(out3);
  release(out3); fx();
  transmit(out4);
  release(out4);
}
#endif
