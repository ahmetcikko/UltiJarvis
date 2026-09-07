#pragma once
#include <string>

void whisper_init();
bool whisper_ready();
std::string transcribe(const float *samples, int n_samples);
