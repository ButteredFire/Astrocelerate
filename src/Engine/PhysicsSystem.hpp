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
#include <Engine/Threading/WorkerThread.hpp>

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


	/* Advances the simulation time by one tick and returns the accumulated simulation time.
		NOTE: Since this task could take considerable amounts of time to finish and therefore freeze the main thread, it has been designed to run in a worker thread.

		@param worker: The worker thread in which this function is run.

		@return The accumulated simulation time.
	*/
	double tick(std::shared_ptr<WorkerThread> worker);


	/* Performs a physics update.
		@param dt: Delta-time.
	*/
	void update(const double dt);


	/* Updates all entities in the scene that have SPICE ephemeris data.
		@param et: The current epoch in Ephemeris Time.
	*/
	void updateSPICEBodies(const double et);


	/* Propagates all entities in the scene that have propagators attached to them.
		@param et: The epoch in Ephemeris Time.
	*/
	void propagateBodies(const double et);


	/* Updates entities, possibly custom-defined, that neither have SPICE ephemeris data nor use propagators.
		@param dt: Delta-time.
		@param et: The current epoch in Ephemeris Time.
	*/
	void updateGeneralBodies(const double dt, const double et);

private:
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<CoordinateSystem> m_coordSystem;

	double m_accumulator = 0.0;
	double m_simulationTime = 0.0;		// Simulation time (a.k.a. RELATIVE seconds elapsed since epoch; ABSOLUTE seconds is `epoch + m_simulationTime`)

	void bindEvents();


	/* "Homogenizes" coordinate systems by converting the state vectors of all bodies in different coordinate systems to corresponding state vectors in the primary coordinate system (specified in the `SimulationConfigs` YAML section). This should be done only once, at the start of every simulation. */
	void homogenizeCoordinateSystems();
};