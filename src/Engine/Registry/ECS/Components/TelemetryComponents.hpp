/* TelemetryComponents.hpp - Defines components that are solely used for telemetry display purposes. These components should not be relied on in physical calculations!
*/

#pragma once

#include <Platform/External/GLM.hpp>

namespace TelemetryComponent {
	struct RenderTransform {
		glm::dvec3 position;
		glm::dquat rotation;
		double visualScale;
	};
}