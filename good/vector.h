//----------------------------------------------------------------------------------------------------------------
// Vector implementation, based on dynamically allocated/reallocated/deallocated arrays.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_VECTOR_H__
#define __GOOD_VECTOR_H__


#include "good/utility.h"


#define DEFAULT_VECTOR_BUFFER_ALLOC  16      ///< Size of vector buffer allocated by default.
//#define DEBUG_VECTOR_PRINT


namespace good
{

	
	//************************************************************************************************************
	/** Class that holds an array of objects of type T.
	 * Note that assignment/copy constructor will "steal" internal buffer so use carefully. 
	 * Use duplicate() to obtain new duplicated array.
	 * Buffer will double his size when needed. */
	//************************************************************************************************************
	template <
		typename T,
		typename Alloc = good::allocator<T>
	>
	class vector
	{
	public:

		//========================================================================================================
		/// Const iterator of vector.
		//========================================================================================================
		class const_iterator
		{
		public:
			friend class good::vector<T, Alloc>;
			
			typedef T* pointer;
			typedef T& reference;

			// Constructor by value.
			const_iterator( T* n = NULL ): m_pCurrent(n) {}
			// Copy constructor.
			const_iterator( const_iterator const& itOther ): m_pCurrent(itOther.m_pCurrent) {}

			/// Operator <.
			bool operator< ( const const_iterator& itOther ) const { return m_pCurrent < itOther.m_pCurrent; }
			/// Operator <.
			bool operator<= ( const const_iterator& itOther ) const { return m_pCurrent <= itOther.m_pCurrent; }
			/// Operator <.
			bool operator> ( const const_iterator& itOther ) const { return m_pCurrent > itOther.m_pCurrent; }
			/// Operator <.
			bool operator>= ( const const_iterator& itOther ) const { return m_pCurrent >= itOther.m_pCurrent; }

			/// Operator ==.
			bool operator== ( const const_iterator& itOther ) const { return m_pCurrent == itOther.m_pCurrent; }
			/// Operator !=.
			bool operator!= ( const const_iterator& itOther ) const { return m_pCurrent != itOther.m_pCurrent; }

			/// Operator -.
			int operator- ( const const_iterator& itOther ) const { DebugAssert(m_pCurrent); return m_pCurrent - itOther.m_pCurrent; }

			/// Operator +.
			const_iterator operator+ ( int iOffset ) const { DebugAssert(m_pCurrent); return const_iterator(m_pCurrent + iOffset); }
			/// Operator +=.
			const_iterator& operator+= ( int iOffset ) { DebugAssert(m_pCurrent); m_pCurrent += iOffset; return *this; }

			/// Operator -.
			const_iterator operator- ( int iOffset ) const { DebugAssert(m_pCurrent); return const_iterator(m_pCurrent - iOffset); }
			/// Operator -=.
			const_iterator& operator-= ( int iOffset ) { DebugAssert(m_pCurrent); m_pCurrent -= iOffset; return *this; }

			/// Pre-increment.
			const_iterator& operator++() {DebugAssert(m_pCurrent);  m_pCurrent++; return *this; }
			/// Pre-decrement.
			const_iterator& operator--() { DebugAssert(m_pCurrent); m_pCurrent--; return *this; }

			/// Post-increment.
			const_iterator operator++ (int) { DebugAssert(m_pCurrent); const_iterator tmp(*this); m_pCurrent++; return tmp; }
			/// Post-decrement.
			const_iterator operator-- (int) { DebugAssert(m_pCurrent); const_iterator tmp(*this); m_pCurrent--; return tmp; }

			/// Dereference.
			const T& operator*() const { DebugAssert(m_pCurrent); return *m_pCurrent; }
			/// Dereference.
			const T& operator[] (int iOffset) const { DebugAssert(m_pCurrent); return m_pCurrent[iOffset]; }
			/// Element selection through pointer.
			const pointer operator->() const { DebugAssert(m_pCurrent); return m_pCurrent; }
			
		protected:
			pointer m_pCurrent;
		};


		//========================================================================================================
		/// Iterator of vector.
		//========================================================================================================
		class iterator: public const_iterator
		{
		public:
			typedef const_iterator base_class;

			// Constructor by value.
			iterator( pointer n = NULL ): base_class(n) {}
			// Copy constructor.
			iterator( iterator const& itOther ): base_class(itOther) {}

			/// Operator +.
			iterator operator+ ( int iOffset ) const { DebugAssert(m_pCurrent); return iterator(m_pCurrent + iOffset); }
			/// Operator +=.
			iterator& operator+= ( int iOffset ) { DebugAssert(m_pCurrent); m_pCurrent += iOffset; return *this; }

			/// Operator -.
			int operator- ( const iterator& other ) const { DebugAssert(m_pCurrent); return (m_pCurrent - other.m_pCurrent); }

			/// Operator -.
			iterator operator- ( int iOffset ) const { DebugAssert(m_pCurrent); return iterator(m_pCurrent - iOffset); }
			/// Operator -=.
			iterator& operator-= ( int iOffset ) { DebugAssert(m_pCurrent); m_pCurrent -= iOffset; return *this; }

			/// Pre-increment.
			iterator& operator++() { DebugAssert(m_pCurrent); m_pCurrent++; return *this; }
			/// Pre-decrement.
			iterator& operator--() { DebugAssert(m_pCurrent); m_pCurrent--; return *this; }

			/// Post-increment.
			iterator operator++ (int) { DebugAssert(m_pCurrent); iterator tmp(*this); m_pCurrent++; return tmp; }
			/// Post-decrement.
			iterator operator-- (int) { DebugAssert(m_pCurrent); iterator tmp(*this); m_pCurrent--; return tmp; }

			/// Dereference.
			reference operator*() const { DebugAssert(m_pCurrent); return *m_pCurrent; }
			/// Dereference.
			reference operator[] (int iOffset) const { DebugAssert(m_pCurrent); return m_pCurrent[iOffset]; }
			/// Element selection through pointer.
			pointer operator->() const { DebugAssert(m_pCurrent); return m_pCurrent; }
		};


		typedef reverse_iterator<const_iterator> const_reverse_iterator; ///< Reverse const iterator of a vector.
		typedef reverse_iterator<iterator> reverse_iterator;             ///< Reverse iterator of a vector.


		//--------------------------------------------------------------------------------------------------------
		/// Default constructor.
		//--------------------------------------------------------------------------------------------------------
		vector(): m_iSize(0), m_iCapacity(0), m_pBuffer(NULL)
		{
#ifdef DEBUG_VECTOR_PRINT
			DebugPrint( "vector default constructor(): capacity 0\n" );
#endif
		}

		//--------------------------------------------------------------------------------------------------------
		/// Constructor with capacity parameter.
		//--------------------------------------------------------------------------------------------------------
		vector( int iCapacity ): m_iSize(0), m_iCapacity(iCapacity)
		{
#ifdef DEBUG_VECTOR_PRINT
			DebugPrint( "vector constructor(): capacity %d\n", iCapacity );
#endif
			if ( iCapacity > 0 )
				m_pBuffer = m_cAlloc.allocate(iCapacity);
			else
				m_pBuffer = NULL;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Copy constructor.
		//--------------------------------------------------------------------------------------------------------
		vector( const vector& aOther ): m_iSize(0), m_iCapacity(0), m_pBuffer(NULL)
		{
#ifdef DEBUG_VECTOR_PRINT
			DebugPrint( "vector copy constructor(): %d elements\n", aOther.size() );
#endif
			assign(aOther);
		}

		//--------------------------------------------------------------------------------------------------------
		/// Destructor.
		//--------------------------------------------------------------------------------------------------------
		~vector()
		{
#ifdef DEBUG_VECTOR_PRINT
			DebugPrint( "vector destructor(): %d elements, %d reserved\n", m_iSize, m_iCapacity );
#endif
			if ( m_pBuffer )
			{
				clear();
				m_cAlloc.deallocate( m_pBuffer, m_iCapacity );
			}
		}


		//--------------------------------------------------------------------------------------------------------
		/// Return underlaying const array.
		//--------------------------------------------------------------------------------------------------------
		const T* data() const { return m_pBuffer; }

		//--------------------------------------------------------------------------------------------------------
		/// Return underlaying array.
		//--------------------------------------------------------------------------------------------------------
		T* data() { return m_pBuffer; }

		//--------------------------------------------------------------------------------------------------------
		/// Return const iterator to the first element. Notice that this is a random access iterator.
		//--------------------------------------------------------------------------------------------------------
		const_iterator begin() const { return const_iterator(m_pBuffer); }

		//--------------------------------------------------------------------------------------------------------
		/// Return const iterator to the element past last one. Notice that this is a random access iterator.
		//--------------------------------------------------------------------------------------------------------
		const_iterator end() const { return const_iterator(m_pBuffer + m_iSize); }

		//--------------------------------------------------------------------------------------------------------
		/// Return reverse const iterator to the last element. Notice that this is a random access iterator.
		//--------------------------------------------------------------------------------------------------------
		const_reverse_iterator rbegin() const { return const_reverse_iterator( end() ); }

		//--------------------------------------------------------------------------------------------------------
		/// Return reverse const iterator to the first element. Notice that this is a random access iterator.
		//--------------------------------------------------------------------------------------------------------
		const_reverse_iterator rend() const { return const_reverse_iterator( begin() ); }

		
		//--------------------------------------------------------------------------------------------------------
		/// Return iterator to the first element. Notice that this is a random access iterator.
		//--------------------------------------------------------------------------------------------------------
		iterator begin() { return iterator(m_pBuffer); }

		//--------------------------------------------------------------------------------------------------------
		/// Return iterator to the element past last one. Notice that this is a random access iterator.
		//--------------------------------------------------------------------------------------------------------
		iterator end() { return iterator(m_pBuffer + m_iSize); }

		//--------------------------------------------------------------------------------------------------------
		/// Return iterator to the last element. Notice that this is a random access iterator.
		//--------------------------------------------------------------------------------------------------------
		reverse_iterator rbegin() { return reverse_iterator( end() ); }

		//--------------------------------------------------------------------------------------------------------
		/// Return iterator to the element before first one. Notice that this is a random access iterator.
		//--------------------------------------------------------------------------------------------------------
		reverse_iterator rend() { return reverse_iterator( begin() ); }


		//--------------------------------------------------------------------------------------------------------
		/// Return true ifarray has no elements.
		//--------------------------------------------------------------------------------------------------------
		bool empty() const { return m_iSize == 0; }

		//--------------------------------------------------------------------------------------------------------
		/// Get vector size, that is, count of elements.
		//--------------------------------------------------------------------------------------------------------
		int size() const { return m_iSize; }

		//--------------------------------------------------------------------------------------------------------
		/// Get vector capacity.
		//--------------------------------------------------------------------------------------------------------
		int capacity() const { return m_iCapacity; }

		//--------------------------------------------------------------------------------------------------------
		/// Assignment.
		//--------------------------------------------------------------------------------------------------------
		void assign( const vector& aOther, bool bCopy = false, bool bUseMemCpy = false )
		{
			if ( bCopy )
			{
				reserve(aOther.m_iCapacity);
				if ( bUseMemCpy )
				{
					m_iSize = aOther.m_iSize;
					memcpy(data(), aOther.data(), m_iSize * sizeof(T));
				}
				else
				{
					clear();
					m_iSize = aOther.m_iSize;
					for ( int i=0; i < m_iSize; ++i )
						m_pBuffer[i] = aOther.m_pBuffer[i];
				}
			}
			else
			{
				clear();
				good::swap(m_pBuffer, ((vector&)aOther).m_pBuffer);
				good::swap(m_iCapacity, ((vector&)aOther).m_iCapacity);
				good::swap(m_iSize, ((vector&)aOther).m_iSize);
			}
		}

		//--------------------------------------------------------------------------------------------------------
		/// Operator =. Note that this operator moves content, not copies it.
		//--------------------------------------------------------------------------------------------------------
		vector& operator= ( const vector& aOther )
		{
#ifdef DEBUG_VECTOR_PRINT
			DebugPrint( "vector operator=: %d elements\n", aOther.size() );
#endif
			assign(aOther);
			return *this;
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Array subscript.
		//--------------------------------------------------------------------------------------------------------
		T& operator[] ( int iIndex ) { DebugAssert(iIndex < m_iSize); return m_pBuffer[iIndex]; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Array subscript const.
		//--------------------------------------------------------------------------------------------------------
		T const& operator[] ( int iIndex ) const { DebugAssert(iIndex < m_iSize); return m_pBuffer[iIndex]; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Element at index.
		//--------------------------------------------------------------------------------------------------------
		T& at( int iIndex ) { DebugAssert(iIndex < m_iSize); return m_pBuffer[iIndex]; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Element at index.
		//--------------------------------------------------------------------------------------------------------
		T const& at( int iIndex ) const { DebugAssert(iIndex < m_iSize); return m_pBuffer[iIndex]; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Get first array element.
		//--------------------------------------------------------------------------------------------------------
		T& front() { DebugAssert(m_iSize > 0); return m_pBuffer[0]; }

		//--------------------------------------------------------------------------------------------------------
		/// Get last array element.
		//--------------------------------------------------------------------------------------------------------
		T& back() { DebugAssert(m_iSize > 0); return m_pBuffer[m_iSize-1]; }

		//--------------------------------------------------------------------------------------------------------
		/// Add element to end of buffer.
		//--------------------------------------------------------------------------------------------------------
		void push_back( const T& tElem ) { insert(m_iSize, tElem); }

		//--------------------------------------------------------------------------------------------------------
		/// Add element to end of buffer.
		//--------------------------------------------------------------------------------------------------------
		void push_front( const T& tElem ) { insert(0, tElem); }

		//--------------------------------------------------------------------------------------------------------
		/// Remove element from end of buffer.
		//--------------------------------------------------------------------------------------------------------
		void pop_back() { erase(m_iSize-1); }

		//--------------------------------------------------------------------------------------------------------
		/// Add element to end of buffer.
		//--------------------------------------------------------------------------------------------------------
		void pop_front() { erase(0); }

		//--------------------------------------------------------------------------------------------------------
		/// Insert element tElem at vector position iPos.
		//--------------------------------------------------------------------------------------------------------
		iterator insert( int iPos, const T& tElem )
		{
			DebugAssert( iPos <= m_iSize );
			increment(1);
			if ( iPos < m_iSize )
				memmove( &m_pBuffer[iPos + 1], &m_pBuffer[iPos],(m_iSize - iPos) * sizeof(T) );
			m_cAlloc.construct( &m_pBuffer[iPos], tElem );
			m_iSize++;
			return iterator(m_pBuffer + iPos);
		}

		//--------------------------------------------------------------------------------------------------------
		/// Insert element tElem before iterator position iPos. Return iterator pointing to next element.
		//--------------------------------------------------------------------------------------------------------
		iterator insert( iterator it, const T& tElem )
		{
			int iPos = it.m_pCurrent - m_pBuffer;
			insert( iPos, tElem );
			return iterator(m_pBuffer + iPos);
		}

		//--------------------------------------------------------------------------------------------------------
		/// Insert element tElem at vector position iPos. Return iterator pointing to next element.
		//--------------------------------------------------------------------------------------------------------
		iterator erase( int iPos )
		{
			DebugAssert( iPos < m_iSize );
			m_cAlloc.destroy(&m_pBuffer[iPos]);
			m_iSize--;
			if ( iPos < m_iSize )
				memmove( &m_pBuffer[iPos], &m_pBuffer[iPos+1], (m_iSize - iPos) * sizeof(T) );
			return iterator(m_pBuffer + iPos);
		}

		//--------------------------------------------------------------------------------------------------------
		/// Delete element at iterator. Return iterator pointing to next element.
		//--------------------------------------------------------------------------------------------------------
		iterator erase( iterator it )
		{
			int iPos = it.m_pCurrent - m_pBuffer;
			erase(iPos);
			return it;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Clear the list.
		//--------------------------------------------------------------------------------------------------------
		void clear()
		{
			for ( int i=0; i<m_iSize; ++i )
				m_cAlloc.destroy(&m_pBuffer[i]);
			m_iSize = 0;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Reserve buffer size.
		//--------------------------------------------------------------------------------------------------------
		void reserve( int iCapacity )
		{
			if ( m_iCapacity >= iCapacity ) return;
			m_pBuffer = m_cAlloc.reallocate( m_pBuffer, iCapacity, m_iCapacity );
			m_iCapacity = iCapacity;
#ifdef DEBUG_VECTOR_PRINT
			DebugPrint( "vector reserve(): %d\n", m_iCapacity );
#endif
		}

		//--------------------------------------------------------------------------------------------------------
		/// Resize array.
		//--------------------------------------------------------------------------------------------------------
		void resize( int iSize, T const& elem = T() )
		{
			if (iSize >= m_iSize)
			{
				reserve(iSize);
				for (int i=m_iSize; i<iSize; ++i)
					m_cAlloc.construct(&m_pBuffer[i], elem);					
			}
			else
			{
				for (int i=iSize; i<m_iSize; ++i)
					m_cAlloc.destroy(&m_pBuffer[i]);
			}
			m_iSize = iSize;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Increment buffer size.
		//--------------------------------------------------------------------------------------------------------
		void increment( int iBySize = 1 )
		{
			int desired = m_iSize + iBySize;
			if ( desired > m_iCapacity )
			{
				iBySize = m_iCapacity;
				if (iBySize == 0)
					iBySize = DEFAULT_VECTOR_BUFFER_ALLOC;
				do {
					iBySize = iBySize<<1; // Double buffer size.
				} while(iBySize < desired);
				reserve(iBySize);
			}
		}

	protected:
		typedef typename Alloc::template rebind<T>::other alloc_t; // Allocator object for T.

		alloc_t m_cAlloc;         // Allocator for T.

		T* m_pBuffer;             // Array of T.
		int m_iCapacity;          // Allocated size.
		int m_iSize;              // Used size.
	};

} // namespace good

#endif // __GOOD_VECTOR_H__
