#pragma once

#ifndef UTLLINKEDLIST_H
#define UTLLINKEDLIST_H

#ifdef _WIN32
#pragma once
#endif

#include "utlmemory.h"
#include "utlfixedmemory.h"
//#include "Preinclude.hpp"

template <class T, class I>
struct UtlLinkedListElem_t
{
	T  m_Element;
	I  m_Previous;
	I  m_Next;
};

template <class T, class S = unsigned short, bool ML = false, class I = S, class M = CUtlMemory< UtlLinkedListElem_t<T, S>, I > >
class CUtlLinkedList
{
public:
	typedef T ElemType_t;
	typedef S IndexType_t; // should really be called IndexStorageType_t, but that would be a huge change
	typedef I IndexLocalType_t;
	typedef M MemoryAllocator_t;
	static const bool IsUtlLinkedList = true; // Used to match this at compiletime 

	I  Head() const
	{
		return m_Head;
	}

	typedef UtlLinkedListElem_t<T, S>  ListElem_t;

	ListElem_t& InternalElement(I i) { return m_Memory[i]; }
	ListElem_t const& InternalElement(I i) const { return m_Memory[i]; }

	bool IsValidIndex(I i) const
	{
		if (!m_Memory.IsIdxValid(i))
			return false;

		if (m_Memory.IsIdxAfter(i, m_LastAlloc))
			return false; // don't read values that have been allocated, but not constructed

		return (m_Memory[i].m_Previous != i) || (m_Memory[i].m_Next == i);
	}

	I Next(I i) const
	{
		if (!IsValidIndex(i))
			return 0;
		return InternalElement(i).m_Next;
	}

	I Previous(I i) const
	{
		if (!IsValidIndex(i))
			return 0;
		return InternalElement(i).m_Previous;
	}

	T& operator[](I i)
	{
		return m_Memory[i].m_Element;
	}

	T const& operator[](I i) const
	{
		return m_Memory[i].m_Element;
	}

	static S InvalidIndex()
	{
		return (S)M::InvalidIndex();
	}

	FORCEINLINE M const &Memory(void) const
	{
		return m_Memory;
	}

	bool IsInList(I i) const
	{
		if (!m_Memory.IsIdxValid(i) || m_Memory.IsIdxAfter(i, m_LastAlloc))
			return false; // don't read values that have been allocated, but not constructed

		return Previous(i) != i;
	}

	void Unlink(I elem)
	{
		if (IsInList(elem))
		{
			ListElem_t * __restrict pOldElem = &m_Memory[elem];

			// If we're the first guy, reset the head
			// otherwise, make our previous node's next pointer = our next
			if (pOldElem->m_Previous != InvalidIndex())
			{
				m_Memory[pOldElem->m_Previous].m_Next = pOldElem->m_Next;
			}
			else
			{
				m_Head = pOldElem->m_Next;
			}

			// If we're the last guy, reset the tail
			// otherwise, make our next node's prev pointer = our prev
			if (pOldElem->m_Next != InvalidIndex())
			{
				m_Memory[pOldElem->m_Next].m_Previous = pOldElem->m_Previous;
			}
			else
			{
				m_Tail = pOldElem->m_Previous;
			}

			// This marks this node as not in the list, 
			// but not in the free list either
			pOldElem->m_Previous = pOldElem->m_Next = elem;

			// One less puppy
			--m_ElementCount;
		}
	}

	void Free(I elem)
	{
		Unlink(elem);

		ListElem_t &internalElem = InternalElement(elem);
		Destruct(&internalElem.m_Element);
		internalElem.m_Next = m_FirstFree;
		m_FirstFree = elem;
	}

	void Remove(I elem)
	{
		Free(elem);
	}

private:

	struct Node_t
	{
		Node_t() {}
		Node_t(const T &_elem) : elem(_elem) {}

		T elem;
		Node_t *pPrev, *pNext;
	};

protected:

	// What the linked list element looks like
	typedef UtlLinkedListElem_t<T, S>  ListElem_t;


	M	m_Memory;
	I	m_Head;
	I	m_Tail;
	I	m_FirstFree;
	I	m_ElementCount;		// The number actually in the list
	I	m_NumAlloced;		// The number of allocated elements
	typename M::Iterator_t	m_LastAlloc; // the last index allocated

										 // For debugging purposes; 
										 // it's in release builds so this can be used in libraries correctly
	ListElem_t  *m_pElements;
};

template < class T >
class CUtlFixedLinkedList : public CUtlLinkedList< T, int, true, int, CUtlFixedMemory< UtlLinkedListElem_t< T, int > > >
{
public:
	CUtlFixedLinkedList(int growSize = 0, int initSize = 0)
		: CUtlLinkedList< T, int, true, int, CUtlFixedMemory< UtlLinkedListElem_t< T, int > > >(growSize, initSize) {}

	typedef CUtlLinkedList< T, int, true, int, CUtlFixedMemory< UtlLinkedListElem_t< T, int > > > BaseClass;

	bool IsValidIndex(int i) const
	{
		if (!BaseClass::Memory().IsIdxValid(i))
			return false;

		return (BaseClass::Memory()[i].m_Previous != i) || (BaseClass::Memory()[i].m_Next == i);
	}
};

#endif