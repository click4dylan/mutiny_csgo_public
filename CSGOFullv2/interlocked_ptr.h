#pragma once

#ifndef INTERLOCKED_PTR_T
#define INTERLOCKED_PTR_T
template <typename T>
class CInterlockedPtr
{
public:
	CInterlockedPtr() : m_value(0) { COMPILE_TIME_ASSERT(sizeof(T *) == sizeof(int32_t)); /* Will need to rework operator+= for 64 bit */ }
	CInterlockedPtr(T *value) : m_value(value) {}

	operator T *() const { return m_value; }

	bool operator!() const { return (m_value == 0); }
	bool operator==(T *rhs) const { return (m_value == rhs); }
	bool operator!=(T *rhs) const { return (m_value != rhs); }

	T *operator++() { return ((T *)ThreadInterlockedExchangeAdd((int32_t *)&m_value, sizeof(T))) + 1; }
	T *operator++(int) { return (T *)ThreadInterlockedExchangeAdd((int32_t *)&m_value, sizeof(T)); }

	T *operator--() { return ((T *)ThreadInterlockedExchangeAdd((int32_t *)&m_value, -sizeof(T))) - 1; }
	T *operator--(int) { return (T *)ThreadInterlockedExchangeAdd((int32_t *)&m_value, -sizeof(T)); }

	bool AssignIf(T *conditionValue, T *newValue) { return ThreadInterlockedAssignPointerToConstIf((void const **)&m_value, (void const *)newValue, (void const *)conditionValue); }

	T *operator=(T *newValue) { ThreadInterlockedExchangePointerToConst((void const **)&m_value, (void const *)newValue); return newValue; }

	void operator+=(int add) { ThreadInterlockedExchangeAdd((int32_t *)&m_value, add * sizeof(T)); }
	void operator-=(int subtract) { operator+=(-subtract); }

	// Atomic add is like += except it returns the previous value as its return value
	T *AtomicAdd(int add) { return (T *)ThreadInterlockedExchangeAdd((int32_t *)&m_value, add * sizeof(T)); }

	T *operator+(int rhs) const { return m_value + rhs; }
	T *operator-(int rhs) const { return m_value - rhs; }
	T *operator+(unsigned rhs) const { return m_value + rhs; }
	T *operator-(unsigned rhs) const { return m_value - rhs; }
	size_t operator-(T *p) const { return m_value - p; }
	size_t operator-(const CInterlockedPtr<T> &p) const { return m_value - p.m_value; }

private:
	T * volatile m_value;
};

template <typename T>
class CInterlockedIntT
{
public:
	CInterlockedIntT() : m_value(0) { /*COMPILE_TIME_ASSERT(sizeof(T) == sizeof(int32));*/ }
	CInterlockedIntT(T value) : m_value(value) {}

	T operator()(void) const { return m_value; }
	operator T() const { return m_value; }

	bool operator!() const { return (m_value == 0); }
	bool operator==(T rhs) const { return (m_value == rhs); }
	bool operator!=(T rhs) const { return (m_value != rhs); }

	T operator++() { return (T)ThreadInterlockedIncrement((int32_t *)&m_value); }
	T operator++(int) { return operator++() - 1; }

	T operator--() { return (T)ThreadInterlockedDecrement((int32_t *)&m_value); }
	T operator--(int) { return operator--() + 1; }

	bool AssignIf(T conditionValue, T newValue) { return ThreadInterlockedAssignIf((int32_t *)&m_value, (int32_t)newValue, (int32_t)conditionValue); }

	T operator=(T newValue) { ThreadInterlockedExchange((int32_t *)&m_value, newValue); return m_value; }

	// Atomic add is like += except it returns the previous value as its return value
	T AtomicAdd(T add) { return (T)ThreadInterlockedExchangeAdd((int32_t *)&m_value, (int32_t)add); }

	void operator+=(T add) { ThreadInterlockedExchangeAdd((int32_t *)&m_value, (int32_t)add); }
	void operator-=(T subtract) { operator+=(-subtract); }
	void operator*=(T multiplier) {
		T original, result;
		do
		{
			original = m_value;
			result = original * multiplier;
		} while (!AssignIf(original, result));
	}
	void operator/=(T divisor) {
		T original, result;
		do
		{
			original = m_value;
			result = original / divisor;
		} while (!AssignIf(original, result));
	}

	T operator+(T rhs) const { return m_value + rhs; }
	T operator-(T rhs) const { return m_value - rhs; }

private:
	volatile T m_value;
};

typedef CInterlockedIntT<int> CInterlockedInt;
typedef CInterlockedIntT<unsigned> CInterlockedUInt;
#endif