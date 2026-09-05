#pragma once
#include <string>

void whisper_init();
std::string transcribe(const float *samples, int n_samples);