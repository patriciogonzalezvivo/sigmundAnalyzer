#ifndef FNC_RAINBOW2FLOAT
#define FNC_RAINBOW2FLOAT

float rainbow2float(vec3 RGB) {
    float value = 0.0; 

    float RGBMin = min(min(RGB.r, RGB.g), RGB.b);
    float RGBMax = max(max(RGB.r, RGB.g), RGB.b);
    float RGBMaxMinDiff = RGBMax - RGBMin;
    float l = (RGBMax + RGBMin) * 0.5;
    float s = 0.0;
    if (l < 0.5)
        s = RGBMaxMinDiff / (RGBMax + RGBMin);
    else
        s = RGBMaxMinDiff / (2.0 - RGBMax - RGBMin);

    vec3 d = (((RGBMax - RGB) * .1666666) + (RGBMaxMinDiff * .5)) / RGBMaxMinDiff;

    if (RGB.r == RGBMax)
        value = d.b - d.g;
    else
        if (RGB.g == RGBMax)
            value = (1.0 / 3.0) + d.r - d.b;
        else
            if (RGB.b == RGBMax)
                value = (2.0 / 3.0) + d.g - d.r;

    return value;
}

#endif