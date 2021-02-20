#pragma once

#include <string>

double getTimeSec();

int toInt(const std::string& _string);
float toFloat(const std::string& _string);

float saturate(float value);
void hue(float hue, unsigned char& _r, unsigned char& _g, unsigned char& _b );
int mod( int i, int base );

enum WindowFunctionType {
    NO_WINDOW = 0,      // no window (crappy frequency spillover)
    HANN_WINDOW = 1,    // Hann window (C^1 cont, so third-order tails)
    TRUNCATED_GAUSSIAN_WINDOW = 2 // truncated Gaussian window (Gaussian tails + exp small error)
};

void fillWindowFunc(WindowFunctionType _type, float *w, int N);