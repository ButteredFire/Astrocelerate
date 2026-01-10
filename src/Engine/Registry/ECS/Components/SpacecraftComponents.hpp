/* SpacecraftComponents.hpp - Components pertaining to spacecraft and satellites.
*/

#pragma once

#include <Platform/External/GLM.hpp>


namespace SpacecraftComponent {
	/* Properties of spacecraft/satellites. */
	struct Spacecraft {
		// ----- Perturbations -----
		double dragCoefficient;              // Drag coefficient
		double referenceArea;				 // Frontal area for drag/SRP (m^2)
		double reflectivityCoefficient;      // Reflectivity coefficient (for Solar Radiation Pressure)
	};


	/* Properties of spacecraft thrusters. */
	struct Thruster {
		double thrustMagnitude;			// Max thrust of the main engine (N)
		double specificImpulse;			// Isp for propellant consumption (s)
		double currentFuelMass;			// Remaining fuel mass (kg)
		double maxFuelMass;				// Total fuel capacity (kg)
	};
}