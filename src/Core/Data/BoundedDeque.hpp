#pragma once

#include <mutex>
#include <deque>
#include <optional>
#include <condition_variable>


/* Thread-safe deque wrapper with a fixed capacity that ignores pushes when it is full. */
template<typename T, size_t Capacity>
class BoundedDeque {
public:
	BoundedDeque() = default;
	~BoundedDeque() = default;

	/* Pushes data to the back of the deque. 
		@note If the deque is full, this function will block until space is available. The data is moved-from and should not be used after this call.
		
		@param data: The data to be pushed.
	*/
	void push_back(T&& data) {
		push([&]() { m_deque.push_back(std::move(data)); });
	}


	/* Attempts to push data to the back of the deque. 
		@note If the deque is full, this function will not block and instead return false. Otherwise, the data is moved-from and should not be used after this call.
		
		@param data: The data to be pushed.
		
		@return True if the push was successful, false if the deque was full.
	*/
	bool try_push_back(T&& data) {
		return tryPush([&]() { m_deque.push_back(std::move(data)); });
	}


	/* Pushes data to the front of the deque. 
		@note If the deque is full, this function will block until space is available. The data is moved-from and should not be used after this call.
		@param data: The data to be pushed.
	*/
	void push_front(T&& data) {
		push([&]() { m_deque.push_front(std::move(data)); });
	}


	/* Attempts to push data to the front of the deque.
		@note If the deque is full, this function will not block and instead return false. Otherwise, the data is moved-from and should not be used after this call.

		@param data: The data to be pushed.

		@return True if the push was successful, false if the deque was full.
	*/
	bool try_push_front(T&& data) {
		return tryPush([&]() { m_deque.push_front(std::move(data)); });
	}


	/* Pops data from the back of the deque.
		@note If the deque is empty, this function will block until data is pushed to the deque.

		@return The popped data.
	*/
	T pop_back() {
		return pop(
			[&]() -> T& { return m_deque.back(); },
			[&]() { m_deque.pop_back(); }
		);
	}


	/* Attempts to pop data from the back of the deque.
		@return The popped data (if the operation was successful), otherwise std::nullopt (if the deque is empty).
	*/
	std::optional<T> try_pop_back() {
		return tryPop(
			[&]() -> T& { return m_deque.back(); },
			[&]() { m_deque.pop_back(); }
		);
	}


	/* Pops data from the front of the deque.
		@note If the deque is empty, this function will block until data is pushed to the deque.

		@return The popped data.
	*/
	T pop_front() {
		return pop(
			[&]() -> T& { return m_deque.front(); },
			[&]() { m_deque.pop_front(); }
		);
	}


	/* Attempts to pop data from the front of the deque.
		@return The popped data (if the operation was successful), otherwise std::nullopt (if the deque is empty).
	*/
	std::optional<T> try_pop_front() {
		return tryPop(
			[&]() -> T& { return m_deque.front(); },
			[&]() { m_deque.pop_front(); }
		);
	}


	size_t size() const {
		std::lock_guard<std::mutex> lock(m_dequeMtx);
		return m_deque.size();
	}


	bool empty() const {
		std::lock_guard<std::mutex> lock(m_dequeMtx);
		return m_deque.empty();
	}

private:
	std::deque<T> m_deque;

	mutable std::mutex m_dequeMtx;

	std::condition_variable m_dequeNotFull;
	std::condition_variable m_dequeNotEmpty;

	/* Pushes to deque, but blocks current thread if thread is full until it isn't. */
	template<typename Func>
	void push(Func&& pushFunc) {
		std::unique_lock<std::mutex> lock(m_dequeMtx);
		m_dequeNotFull.wait(lock, [this]() { return m_deque.size() < Capacity; });

		pushFunc();
		m_dequeNotEmpty.notify_one();
	}


	/* Attempts to push to deque; thread is not blocked and instead receives a bool indicating the push success status. */
	template<typename Func>
	bool tryPush(Func &&pushFunc) {
		std::lock_guard<std::mutex> lock(m_dequeMtx);
		if (m_deque.size() >= Capacity)
			return false;

		pushFunc();
		m_dequeNotEmpty.notify_one();
	
		return true;
	}


	/* Pops from deque, but blocks current thread if thread is empty until it isn't. */
	template<typename GetFunc, typename PopFunc>
	T pop(GetFunc&& getFunc, PopFunc&& popFunc) {
		std::unique_lock<std::mutex> lock(m_dequeMtx);
		m_dequeNotEmpty.wait(lock, [this]() { return !m_deque.empty(); });
	
		T data = std::move(getFunc());
		popFunc();

		m_dequeNotFull.notify_one();

		return data;
	}


	/* Attempts to pop from deque; thread is not blocked and instead receives either nothing (if deque empty) or the popped data (if deque has data). */
	template<typename GetFunc, typename PopFunc>
	std::optional<T> tryPop(GetFunc &&getFunc, PopFunc &&popFunc) {
		std::unique_lock<std::mutex> lock(m_dequeMtx);
		
		if (m_deque.empty())
			return std::nullopt;

		T data = std::move(getFunc());
		popFunc();

		m_dequeNotFull.notify_one();

		return data;
	}
};
