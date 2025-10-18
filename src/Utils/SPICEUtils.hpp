/* SPICEUtils - SPICE API wrappers because their function names are so goddamn cryptic.
*/

#pragma once

#include <External/SPICE.hpp>

#include <Core/Application/LoggingManager.hpp>


namespace SPICEUtils {
	/* Queries the availability of an object name.
		NOTE: This function assumes all necessary kernels have been loaded prior to calling it.

		@param name: The name of the object.

		@return True if the necessary ephemeris data for that object is available, False otherwise.
	*/
	inline bool IsObjectAvailable(const std::string &name) {
		long naifCode;
		int isAvailable;
		
		// "Body Name to Code": Translates the name of a body or object to the corresponding SPICE integer ID code.
		bodn2c_c(name.c_str(), &naifCode, &isAvailable);

		return isAvailable;
	}


	/* Checks the execution status of a SPICE function.
		@param throwException: If the SPICE function fails, should an exception be thrown?
		@param logError (default: False): If the SPICE function fails, and `throwException` is False, should an error be logged?
		@param handleFailure(errMsg) (optional): A function that, if the SPICE function fails, will handle the error. If exception throwing and/or error logging are/is enabled, this function will be run before them/it.
	*/
	inline void CheckFailure(bool throwException, bool logError = false, const std::function<void(const std::string)> handleFailure = [](const std::string _){}) {
		static constexpr SpiceInt SPICE_MAX_MSG_LEN = 1840; // Source: https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/getmsg_c.html

		if (failed_c()) {
			char explanation[SPICE_MAX_MSG_LEN + 1];

			// Get the error message
			getmsg_c("LONG", SPICE_MAX_MSG_LEN, explanation);

			std::string errMsg(explanation);

			handleFailure(std::string(explanation));

			// Reset SPICE internal error flags to prevent stale errors
			reset_c();

			if (throwException)
				throw Log::RuntimeException(__FUNCTION__, __LINE__, errMsg);

			else if (logError)
				Log::Print(Log::T_ERROR, __FUNCTION__, errMsg);
		}
	}
}