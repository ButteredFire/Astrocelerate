#pragma once

#include <format>
#include <string>
#include <exception>


/* Virtual Machine Configuration Exception */
class VMConfigException : public std::exception {
public:
	template<typename... Args>
	VMConfigException(const std::string_view messageFmt, Args&&... args) :
		m_excMsg(std::vformat(messageFmt, std::make_format_args(args...))) {}

	~VMConfigException() = default;

	inline const char *what() const noexcept override {
		return m_excMsg.c_str();
	}

private:
	std::string m_excMsg;
};


/* Virtual Machine Runtime Exception */
class VMRuntimeException : public std::exception {
public:
	template<typename... Args>
	VMRuntimeException(const std::string_view messageFmt, Args&&... args) :
		m_excMsg(std::vformat(messageFmt, std::make_format_args(args...))) {}

	~VMRuntimeException() = default;

	inline const char *what() const noexcept override {
		return m_excMsg.c_str();
	}

private:
	std::string m_excMsg;
};


/* Virtual Machine Breakpoint Trap */
class VMBreakpointTrap : public std::exception {
public:
	VMBreakpointTrap(size_t progCounter) :
		m_pc(progCounter),
		m_msg(
			std::format("Breakpoint triggered at instruction address 0x{:0>{}X}",
				progCounter - 1, 4
			)
		)
	{}
	~VMBreakpointTrap() = default;

	inline const char* what() const noexcept override {
		return m_msg.c_str();
	}
	
private:
	std::string m_msg;
	size_t m_pc;
};


/* Virtual Machine Trap upon Execution Finish at Simulation Tick */
class VMFinishedExecTick : public std::exception {
public:
	VMFinishedExecTick() {}
	~VMFinishedExecTick() = default;

	inline const char* what() const noexcept override {
		return "";
	}
};
