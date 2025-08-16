/* SPICEUtils - SPICE API wrappers because their function names are so goddamn cryptic.
*/

#pragma once

#include <External/SPICE.hpp>


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
}