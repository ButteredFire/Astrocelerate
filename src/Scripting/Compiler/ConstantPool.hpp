#pragma once

#include <string>
#include <vector>
#include <limits>
#include <iterator>
#include <exception>
#include <unordered_map>
#include <unordered_set>

#include <Scripting/AsTLTypes.hpp>
#include <Scripting/GraphTypes.hpp>


namespace Compiler {
	class ConstantPool {
	public:
		ConstantPool() = default;
		~ConstantPool() = default;

		// The Constant Pool is a contiguous array of AstroAssembly literals, indexed by array index
		using ConstantPoolT = std::vector<AsTL::StackValue>;

		// The Constant Pool Lookup Map links the literals to the literals' indices in the Constant Pool.
		using ConstantPoolLookupT = std::unordered_map<AsTL::StackValue, AsTL::IDX>;

		/* Gets an index of an existing literal into the constant pool, or creates a new index for a new literal into the constant pool.
			If the literal does not exist, it will be inserted into the constant pool.

			@param val: The literal.
			@return The index pointing to the literal value in the constant pool.
		*/
		AsTL::IDX getOrCreateIndex(const AsTL::StackValue &val) {
			auto it = m_lookup.find(val);
			if (it == m_lookup.end()) {
				if (m_pool.size() > std::numeric_limits<AsTL::IDX>::max())
					throw std::exception("Encountered constant pool overflow");

				m_lookup[val] = static_cast<AsTL::IDX>(m_pool.size());
				m_pool.push_back(val);
			}

			return m_lookup.at(val);
		}

		const auto &getPool() const { return m_pool; }
		const auto &getPoolLookup() const { return m_lookup; }

		bool contains(const AsTL::StackValue &val) const { return m_lookup.contains(val); }

		const AsTL::StackValue &getValue(AsTL::IDX idx) const { return m_pool[idx]; }

	private:
		ConstantPoolT m_pool;
		ConstantPoolLookupT m_lookup;
	};
}
