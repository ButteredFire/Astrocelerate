/* ColorUtils.hpp - Manages utilities pertaining to color correction.
*/
#pragma once

#include <Core/Constants.h>


/* Converts a color channel value in sRGB space to an equivalent value in linear space.
* @param color: The sRGB color value.
* @return An equivalent value in linear color space.
*/
static inline float sRGBToLinear(float color) {
	return (color <= Gamma::THRESHOLD) ? (color / Gamma::DIVISOR) : powf((color + Gamma::OFFSET) / Gamma::SCALE, Gamma::EXPONENT);
}


/* Converts a set of sRGB values to an equivalent set of linear color space values.
* @param r: The Red channel.
* @param g: The Green channel.
* @param b: The Blue channel.
* @param a (Default: 1.0): The Alpha channel.
* @return A 4-component ImGui Vector containing the (r, g, b, a) set in linear color space.
*/
static inline ImVec4 linearRGBA(float r, float g, float b, float a = 1.0f) {
	return ImVec4(sRGBToLinear(r), sRGBToLinear(g), sRGBToLinear(b), a);
}
