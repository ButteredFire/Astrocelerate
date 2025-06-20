/* ColorUtils.hpp - Manages utilities pertaining to color correction.
*/
#pragma once

#include <Core/Data/Constants.h>
#include <Core/Application/LoggingManager.hpp>


namespace ColorUtils {
	/* Converts a color channel value in sRGB space to an equivalent value in linear space.
		@param color: The sRGB color value.

		@return An equivalent value in linear color space.
	*/
	inline float sRGBChannelToLinearChannel(float color) {
		return (color <= Gamma::THRESHOLD) ? (color / Gamma::DIVISOR) : powf((color + Gamma::OFFSET) / Gamma::SCALE, Gamma::EXPONENT);
	}


	/* Converts a set of sRGB values to an equivalent set of linear color space values.
		@param r: The Red channel.
		@param g: The Green channel.
		@param b: The Blue channel.
		@param a (Default: 1.0): The Alpha channel.
		
		@return A 4-component ImGui Vector containing the (r, g, b, a) set in linear color space.
	*/
	inline ImVec4 sRGBToLinear(float r, float g, float b, float a = 1.0f) {
		return ImVec4(sRGBChannelToLinearChannel(r), sRGBChannelToLinearChannel(g), sRGBChannelToLinearChannel(b), a);
	}


	/* Converts a logging message type to an ImVec4 color.
		@param type: The logging message type.

		@return An ImVec4 color corresponding to the logging message type.
	*/
	inline ImVec4 LogMsgTypeToImVec4(Log::MsgType type) {
		switch (type) {
			case Log::T_VERBOSE:  return ImVec4(0.6f, 0.6f, 0.6f, 1.0f); // gray
			case Log::T_DEBUG:    return ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // light gray
			case Log::T_INFO:     return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // white
			case Log::T_WARNING:  return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // yellow
			case Log::T_ERROR:    return ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // red
			case Log::T_FATAL:    return ImVec4(0.7f, 0.04f, 0.04f, 1.0f); // deeper red
			case Log::T_SUCCESS:  return ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // green
			default:              return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // default white
		}
	}
}
