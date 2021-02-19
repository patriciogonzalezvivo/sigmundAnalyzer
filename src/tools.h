#pragma once

#include <string>

double getTimeSec();

int toInt(const std::string& _string);
float toFloat(const std::string& _string);

float saturate(float value);
void hue(float hue, unsigned char& _r, unsigned char& _g, unsigned char& _b );