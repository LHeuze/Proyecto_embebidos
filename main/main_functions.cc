/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "main_functions.h"

#include "detection_responder.h"
#include "image_provider.h"
#include "model_settings.h"
#include "model_int8.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_log.h>
#include "esp_main.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;

// In order to use optimized tensorflow lite kernels, a signed int8_t quantized
// model is preferred over the legacy unsigned model format. This means that
// throughout this project, input images must be converted from unisgned to
// signed format. The easiest and quickest way to convert from unsigned to
// signed 8-bit integers is to subtract 128 from the unsigned value to get a
// signed value.

#ifdef CONFIG_IDF_TARGET_ESP32S3
constexpr int scratchBufSize = 40 * 1024;
#else
constexpr int scratchBufSize = 0;
#endif
// An area of memory to use for input, output, and intermediate arrays.
constexpr int kTensorArenaSize = 81 * 1024 + scratchBufSize;
static uint8_t *tensor_arena;//[kTensorArenaSize]; // Maybe we should move this to external
}  // namespace

// The name of this function is important for Arduino compatibility.
void setup() {
  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(model_quant_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf("Model provided is schema version %d not equal to supported "
                "version %d.", model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  if (tensor_arena == NULL) {
    tensor_arena = (uint8_t *) heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  if (tensor_arena == NULL) {
    printf("Couldn't allocate memory of %d bytes\n", kTensorArenaSize);
    return;
  }

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.
  // An easier approach is to just use the AllOpsResolver, but this will
  // incur some penalty in code space for op implementations that are not
  // needed by this graph.
  //
  // tflite::AllOpsResolver resolver;
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroMutableOpResolver<7> micro_op_resolver;
  micro_op_resolver.AddAveragePool2D();
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddDepthwiseConv2D();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddSoftmax();
  micro_op_resolver.AddMaxPool2D();
  micro_op_resolver.AddFullyConnected();

  // Build an interpreter to run the model with.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  input = interpreter->input(0);

#ifndef CLI_ONLY_INFERENCE
  // Initialize Camera
  TfLiteStatus init_status = InitCamera();
  if (init_status != kTfLiteOk) {
    MicroPrintf("InitCamera failed\n");
    return;
  }
#endif
}

#ifndef CLI_ONLY_INFERENCE
// The name of this function is important for Arduino compatibility.
void loop() {
  // Get image from provider.
  if (kTfLiteOk != GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8)) {
    MicroPrintf("Image capture failed.");
  }

  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter->Invoke()) {
    MicroPrintf("Invoke failed.");
  }

  TfLiteTensor* output = interpreter->output(0);

  // Process the inference results.
  int8_t zero_score = output->data.uint8[kZeroIndex];
  int8_t one_score = output->data.uint8[kOneIndex];
  int8_t two_score = output->data.uint8[kTwoIndex];
  int8_t three_score = output->data.uint8[kThreeIndex];
  int8_t four_score = output->data.uint8[kFourIndex];
  int8_t five_score = output->data.uint8[kFiveIndex];
  // int8_t six_score = output->data.uint8[kSixIndex];
  // int8_t seven_score = output->data.uint8[KSevenIndex];
  // int8_t eight_score = output->data.uint8[KEightIndex];
  // int8_t nine_score = output->data.uint8[KNineIndex];

  float zero_score_f = 
      (zero_score - output->params.zero_point) * output->params.scale;
  float one_score_f = 
      (one_score - output->params.zero_point) * output->params.scale;
  float two_score_f = 
      (two_score - output->params.zero_point) * output->params.scale;
  float three_score_f = 
      (three_score - output->params.zero_point) * output->params.scale;
  float four_score_f = 
      (four_score - output->params.zero_point) * output->params.scale;
  float five_score_f = 
      (five_score - output->params.zero_point) * output->params.scale;
  // float six_score_f=
  //     (six_score - output->params.zero_point) * output->params.scale;
  // float seven_score_f=
  //     (seven_score - output->params.zero_point) * output->params.scale;
  // float eight_score_f=
  //     (eight_score - output->params.zero_point) * output->params.scale;
  // float nine_score_f=
  //     (nine_score - output->params.zero_point) * output->params.scale;
  // RespondToDetection(zero_score_f, one_score_f, two_score_f, three_score_f, four_score_f, five_score_f, six_score_f, seven_score_f, eight_score_f, nine_score_f);
  RespondToDetection(zero_score_f, one_score_f, two_score_f, three_score_f, four_score_f, five_score_f);
  vTaskDelay(1); // to avoid watchdog trigger
}
#endif

#if defined(COLLECT_CPU_STATS)
  long long total_time = 0;
  long long start_time = 0;
  extern long long softmax_total_time;
  extern long long dc_total_time;
  extern long long conv_total_time;
  extern long long fc_total_time;
  extern long long pooling_total_time;
  extern long long add_total_time;
  extern long long mul_total_time;

  const float voltage = 3.3;  // Operating voltage in volts
  const float current = 0.24;  // Current draw in amps

  void log_energy_consumption(const char* task_name, long long task_time) {
    float task_time_sec = task_time / 1e6;  // Convert time to seconds
    float power = voltage * current;  // Power in watts
    float energy = power * task_time_sec;  // Energy in joules
    printf("%s time = %lld ms, energy = %f mJ\n", task_name, task_time / 1000, energy * 1000);
  }
#endif

void run_inference(void *ptr) {
  /* Convert from uint8 picture data to int8 */
  for (int i = 0; i < kNumCols * kNumRows; i++) {
    input->data.int8[i] = ((uint8_t *) ptr)[i] ^ 0x80;
  }

#if defined(COLLECT_CPU_STATS)
  long long start_time = esp_timer_get_time();
#endif
  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter->Invoke()) {
    MicroPrintf("Invoke failed.");
  }

#if defined(COLLECT_CPU_STATS)
  long long total_time = (esp_timer_get_time() - start_time);
  log_energy_consumption("Total", total_time);
  log_energy_consumption("Fully Connected", fc_total_time);
  log_energy_consumption("Depthwise Conv", dc_total_time);
  log_energy_consumption("Conv", conv_total_time);
  log_energy_consumption("Pooling", pooling_total_time);
  log_energy_consumption("Add", add_total_time);
  log_energy_consumption("Mul", mul_total_time);

  /* Reset times */
  total_time = 0;
  //softmax_total_time = 0;
  dc_total_time = 0;
  conv_total_time = 0;
  fc_total_time = 0;
  pooling_total_time = 0;
  add_total_time = 0;
  mul_total_time = 0;
#endif

  TfLiteTensor* output = interpreter->output(0);

  // Process the inference results.
  int8_t zero_score = output->data.uint8[kZeroIndex];
  int8_t one_score = output->data.uint8[kOneIndex];
  int8_t two_score = output->data.uint8[kTwoIndex];
  int8_t three_score = output->data.uint8[kThreeIndex];
  int8_t four_score = output->data.uint8[kFourIndex];
  int8_t five_score = output->data.uint8[kFiveIndex];
  // int8_t six_score = output->data.uint8[kSixIndex];
  // int8_t seven_score = output->data.uint8[KSevenIndex];
  // int8_t eight_score = output->data.uint8[KEightIndex];
  // int8_t nine_score = output->data.uint8[KNineIndex];

  float zero_score_f = 
      (zero_score - output->params.zero_point) * output->params.scale;
  float one_score_f = 
      (one_score - output->params.zero_point) * output->params.scale;
  float two_score_f = 
      (two_score - output->params.zero_point) * output->params.scale;
  float three_score_f = 
      (three_score - output->params.zero_point) * output->params.scale;
  float four_score_f = 
      (four_score - output->params.zero_point) * output->params.scale;
  float five_score_f = 
      (five_score - output->params.zero_point) * output->params.scale;
  // float six_score_f=
  //     (six_score - output->params.zero_point) * output->params.scale;
  // float seven_score_f=
  //     (seven_score - output->params.zero_point) * output->params.scale;
  // float eight_score_f=
  //     (eight_score - output->params.zero_point) * output->params.scale;
  // float nine_score_f=
  //     (nine_score - output->params.zero_point) * output->params.scale;
  // RespondToDetection(zero_score_f, one_score_f, two_score_f, three_score_f, four_score_f, five_score_f, six_score_f, seven_score_f, eight_score_f, nine_score_f);
  RespondToDetection(zero_score_f, one_score_f, two_score_f, three_score_f, four_score_f, five_score_f);

}