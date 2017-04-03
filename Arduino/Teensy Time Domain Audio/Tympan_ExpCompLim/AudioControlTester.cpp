
#include "AudioControlTester.h"

void AudioTestSignalGenerator_F32::update(void) {
  //receive the input audio data
  audio_block_f32_t *in_block = AudioStream_F32::receiveReadOnly_f32();
  if (!in_block) return;

  //if we're not testing, just transmit the block
  if (!is_testing) {
    AudioStream_F32::transmit(in_block); // send the FIR output
    AudioStream_F32::release(in_block);
    return;
  }

  //if we are testing, we're going to substitute a new signal,
  //so we can release the block for the original signal
  AudioStream_F32::release(in_block);

  //now generate the new siginal
  sine_gen.begin(); record_queue.begin();  //activate
  sine_gen.update();
  record_queue.update();
  audio_block_f32_t *out_block = record_queue.getAudioBlock();
  AudioStream_F32::transmit(out_block); // send the FIR output
  record_queue.freeAudioBlock();
  sine_gen.end(); record_queue.end();  //put them to sleep again

}


void AudioTestSignalMeasurement_F32::update(void) {
  
  //if we're not testing, just return
  if (!is_testing) {
    return;
  }

//  if (Serial) {
//    Serial.print("AudioTestSignalMeasurement_F32: update(): is_testing ");
//    Serial.println(is_testing);
//  }

  //receive the input audio data...the baseline and the test
  audio_block_f32_t *in_block_baseline = AudioStream_F32::receiveReadOnly_f32(0);
  if (!in_block_baseline) return;
  audio_block_f32_t *in_block_test = AudioStream_F32::receiveReadOnly_f32(1);
  if (!in_block_test) {
    AudioStream_F32::release(in_block_baseline);
    return;
  }

  //compute the rms of both signals
  float baseline_rms = computeRMS(in_block_baseline->data, in_block_baseline->length);
  float test_rms = computeRMS(in_block_test->data, in_block_test->length);

  //notify controller
  if (testController != NULL) testController->transferRMSValues(baseline_rms, test_rms);
}
