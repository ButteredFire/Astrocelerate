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


	/* Converts a TLE epoch into TDB seconds past the J2000 epoch.
		@param line1: The TLE's first line.
	
		@return TDB seconds past the J2000 epoch.
	*/
	inline double tleEpochToET(const std::string &line1) {
		// Isolate the field that contains the TLE epoch
		uint32_t targetField = 4; // The field within the first line where the epoch is stored
		uint32_t currentField = 0;
		bool startOfField = false;
		std::stringstream ss;

		for (size_t i = 1; i < line1.size(); i++) {
			if (line1[i - 1] != ' ') {
				if (i - 2 >= 0 && line1[i - 2] == ' ')
					startOfField = true;

				ss << line1[i - 1];

				if (line1[i] == ' ') {
					startOfField = false;
					currentField++;

					if (currentField == targetField)
						break;

					ss.str("");
					ss.clear();
				}
			}
		}

		if (currentField != targetField) {
			Log::Print(Log::T_ERROR, __FUNCTION__, "Cannot convert the specified TLE epoch into equivalent seconds past J2000: Cannot find TLE epoch within line 1!");
			return 0.0;
		}


		// Parse the TLE epoch
		const std::string &epoch = ss.str();

		int tleYear = std::stoi(epoch.substr(0, 2));
		double tleDayOfYear = std::stod(epoch.substr(2));



			// Determine year, days into the year, and fractional day
		int fullYear = (tleYear < 57) ? (2000 + tleYear) : (1900 + tleYear);  // The TLE format was designed such that there are only 2 columns to represent the year (suffix). By convention (to avoid the Y2K problem), if suffix < 57, the full year would be (2000 + suffix). Otherwise, it would be (1900 + suffix).
		int dayOfYear = static_cast<int>(floor(tleDayOfYear));
		double fractionalDay = tleDayOfYear - dayOfYear;

			// Convert Day of year to month/day and Fractional day to HMS
		const int daysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
		int currentDay = dayOfYear;
		int month = 1;

				// Account for leap year
		bool isLeap = (fullYear % 4 == 0 && fullYear % 100 != 0) || (fullYear % 400 == 0);
		int febDays = isLeap ? 29 : 28;

		for (month = 1; month <= 12; month++) {
			int daysInCurrentMonth = (month == 2) ? febDays : daysInMonth[month];

			if (currentDay > daysInCurrentMonth)
				currentDay -= daysInCurrentMonth;
			else
				break;
		}

				// Calculate HMS
		const double secondsInDay = 86400.0;
		double totalSeconds = fractionalDay * secondsInDay;

		int hours = static_cast<int>(floor(totalSeconds / 3600.0));
		totalSeconds -= hours * 3600.0;

		int minutes = static_cast<int>(floor(totalSeconds / 60.0));
		totalSeconds -= minutes * 60.0;

		double seconds = totalSeconds;



		// Construct SPICE-compatible UTC string
		char utcString[50];
		snprintf(utcString, 50, "%04d-%02d-%02d %02d:%02d:%010.7f UTC",
			fullYear, month, currentDay, hours, minutes, seconds);


		double epochET;
		str2et_c(utcString, &epochET);

		return epochET;
	}
}
