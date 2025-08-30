/* PhysicsSystem.hpp - Manages the physics.
*/

#pragma once

#include <Core/Engine/ECS.hpp>
#include <Core/Application/LoggingManager.hpp>
#include <Core/Engine/ServiceLocator.hpp>
#include <Core/Application/EventDispatcher.hpp>

#include <Core/Data/Physics.hpp>

#include <Simulation/Systems/Time.hpp>

#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/RenderComponents.hpp>

#include <Simulation/ODEs.hpp>
#include <Simulation/Systems/CoordinateSystem.hpp>
#include <Simulation/Integrators/RK4.hpp>
#include <Simulation/Integrators/SymplecticEuler.hpp>
#include <Simulation/Propagators/SGP4/TLE.hpp>

#include <Utils/SpaceUtils.hpp>
#include <Utils/SPICEUtils.hpp>
#include <Utils/FilePathUtils.hpp>


class PhysicsSystem {
public:
	PhysicsSystem();
	~PhysicsSystem() = default;

	void configureCoordSys(CoordSys::FrameType frameType, CoordSys::Frame frame, const std::vector<std::string> &kernelPaths, CoordSys::Epoch epoch, const std::string &epochFormat);


	/* Performs a physics update.
		@param dt: Delta-time.
	*/
	void update(const double dt);


	/* Updates all entities in the scene that have SPICE ephemeris data.
		@param currentET: The current epoch in Ephemeris Time.
	*/
	void updateSPICEBodies(const double currentET);


	/* Updates entities, possibly custom-defined, that neither have SPICE ephemeris data nor use propagators.
		@param dt: Delta-time.
		@param currentET: The current epoch in Ephemeris Time.
	*/
	void updateGeneralBodies(const double dt, const double currentET);

private:
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<CoordinateSystem> m_coordSystem;

	double m_simulationTime = 0.0;		// Simulation time (a.k.a. seconds elapsed since epoch)

	void bindEvents();


	/* "Homogenizes" coordinate systems by converting the state vectors of all bodies in different coordinate systems to corresponding state vectors in the primary coordinate system (specified in the `SimulationConfigs` YAML section). This should be done only once, at the start of every simulation. */
	void homogenizeCoordinateSystems();
};