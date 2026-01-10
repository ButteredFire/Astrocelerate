/* Bodies - Common data pertaining to celestial bodies.
*/

#pragma once

#include <Core/Data/Mapping/YAMLKeys.hpp>

#include <Simulation/Bodies/Includes.hpp>


namespace Body {
	inline _Bodies::Sun			Sun;
	inline _Bodies::Mercury		Mercury;
	inline _Bodies::Venus		Venus;
	inline _Bodies::Earth		Earth;
	inline _Bodies::Moon		Moon;
	inline _Bodies::Mars		Mars;
	inline _Bodies::Jupiter		Jupiter;
	inline _Bodies::Saturn		Saturn;
	inline _Bodies::Uranus		Uranus;
	inline _Bodies::Neptune		Neptune;


	inline ICelestialBody* GetCelestialBody(const _YAMLStrType &YAMLIdentifier) {
		if (YAMLIdentifier == YAMLScene::Body_Sun)
			return &Sun;
		else if (YAMLIdentifier == YAMLScene::Body_Mercury)
			return &Mercury;
		else if (YAMLIdentifier == YAMLScene::Body_Venus)
			return &Venus;
		else if (YAMLIdentifier == YAMLScene::Body_Earth)
			return &Earth;
		else if (YAMLIdentifier == YAMLScene::Body_Moon)
			return &Moon;
		else if (YAMLIdentifier == YAMLScene::Body_Mars)
			return &Mars;
		else if (YAMLIdentifier == YAMLScene::Body_Jupiter)
			return &Jupiter;
		else if (YAMLIdentifier == YAMLScene::Body_Saturn)
			return &Saturn;
		else if (YAMLIdentifier == YAMLScene::Body_Uranus)
			return &Uranus;
		else if (YAMLIdentifier == YAMLScene::Body_Neptune)
			return &Neptune;

		else
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot get celesital body: Invalid identifier " + enquote(YAMLIdentifier) + ".");
	}
}