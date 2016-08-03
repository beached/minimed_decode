// The MIT License (MIT)
//
// Copyright (c) 2014-2015 Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cstddef>
#include <algorithm>

namespace daw {
	template<typename T, size_t rb_capacity>
	struct ring_buffer_t final {
		using value_type = std::decay_t<T>;
		using reference = T &;
		using const_reference = T const &;
		using size_type = size_t;
	private:
		value_type m_data[rb_capacity];
		volatile size_type m_front;
		volatile size_type m_back;

	public:
		ring_buffer_t( ) noexcept:
				m_data{ 0 },
				m_front{ 0 },
				m_back{ 0 } { }

		~ring_buffer_t( ) = default;
		ring_buffer_t( ring_buffer_t && ) = default;
		ring_buffer_t & operator=( ring_buffer_t && ) = default;

		ring_buffer_t( ring_buffer_t const & other ):
				m_data{ },
				m_front{ other.m_front },
				m_back{ other.m_back } {

			std::copy_n( m_data, rb_capacity, other.m_data );
		}

		friend void swap( ring_buffer_t & lhs, ring_buffer_t & rhs ) noexcept {
			using std::swap;
			swap( lhs.m_data, rhs.m_data );
			swap( lhs.m_front, rhs.m_front );
			swap( lhs.m_back, rhs.m_back );
		}

		ring_buffer_t & operator=( ring_buffer_t const & rhs ) noexcept {
			if( this != & rhs ) {
				ring_buffer_t tmp{ rhs };
				using std::swap;
				swap( *this, tmp );
			}
			return *this;
		}

		bool empty( ) const noexcept {
			return m_front == m_back;
		}

		bool full( ) const noexcept {
			if( m_back >= m_front ) {
				return m_back - m_front == 1;
			}
			return m_front - m_back == 1;
		}

		size_type capacity( ) const noexcept {
			return rb_capacity;
		}

		explicit operator bool( ) const noexcept {
			return !empty( );
		}

		bool can_pop( ) const noexcept {
			return !empty( );
		}

		size_type size( ) const noexcept {
			if( m_front <= m_back ) {
				return m_back - m_front;
			}
			return (rb_capacity-m_back) + m_front;
		}

		void push_back( value_type value ) {
			assert( !full( ) );
			if( rb_capacity == m_back ) {
				m_back = 0;
			}
			m_data[m_back++] = std::move( value );
		}

		value_type pop_front( ) {
			assert( !empty( ) );
			if( rb_capacity == m_front ) {
				m_front = 0;
			}
			return std::move( m_data[m_front++] );
		}
	};	// ring_buffer_t

}
