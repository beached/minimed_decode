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

#include <cstdint>
#include <cstddef>
#include <vector>
#include <thread>
#include "queues.h"
#include "ring_buffer.h"

namespace daw {
	uint8_t encode_symbol( uint8_t value );

	uint8_t decode_symbol( uint8_t symbol );

	uint8_t crc8( uint8_t const *msg, size_t msg_size );

	uint16_t crc16( uint8_t const *msg, size_t msg_size );

	template<size_t max_size>
	class medtronic_decoder_t final {
		nibble_queue m_nibble_queue;
		bit_queue m_bit_queue;
		ring_buffer_t<uint8_t, max_size> m_processed_data;

	public:
		~medtronic_decoder_t( ) = default;

		medtronic_decoder_t( ) = default;

		medtronic_decoder_t( medtronic_decoder_t const & ) = default;

		medtronic_decoder_t( medtronic_decoder_t && ) = default;

		medtronic_decoder_t &operator=( medtronic_decoder_t const & ) = default;

		medtronic_decoder_t &operator=( medtronic_decoder_t && ) = default;

		void operator( )( uint8_t value ) {
			if( 0 == value ) {
				return;
			}
			m_bit_queue.push_back( value );
			while( m_bit_queue.can_pop( 6 )) {
				m_nibble_queue.push_back( decode_symbol( m_bit_queue.pop_front( 6 )));
			}
			while( m_nibble_queue.can_pop( 2 )) {
				m_processed_data.push_back( m_nibble_queue.pop_front( 2 ) );
			}
		}

		uint8_t operator( )( ) {
			while( m_processed_data.empty( ) ) {
				// spin spin sugar
				std::this_thread::yield( );
			}
			return m_processed_data.pop_front( );
		}

		explicit operator bool( ) {
			return !empty( );
		}

		bool empty( ) const {
			return m_processed_data.empty( ) && m_nibble_queue.empty( ) && m_bit_queue.empty( );
		}
	};    // medtronic_decoder_t

}    // namespace daw