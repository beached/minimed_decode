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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include "queues.h"
#include <daw/daw_range.h>
#include "medtronic.h"

namespace {

	template <unsigned TDimension, typename TArr>
	constexpr size_t sizeof_array(TArr (&))
	{
		static_assert(std::is_array<TArr>::value,		  "sizeof_array must be called on a compile-time array");
		static_assert(TDimension < std::rank<TArr>::value, "sizeof_array<TDimension>'s dimension is larger than the array");
		return std::extent<TArr, TDimension>::value;
	}
	template <typename TArr>
	constexpr size_t sizeof_array(TArr (&arr)) { return sizeof_array<0, TArr>(arr); }

	template<typename T>
	bool in_range( T v, T lower, T upper ) {
		return lower <= v && v <= upper;
	}

	bool is_hex_char( char c ) {
		return in_range( c, '0', '9' ) || in_range( c, 'a', 'f' ) || in_range( c, 'A', 'F' );
	}

	template<typename Iterator>
	bool skip_non_hex( Iterator pos, Iterator last ) {
		while( pos != last && !is_hex_char( *pos ) ) {
			++pos;
		}
		return pos != last;
	}

	auto read_file( std::string file_name ) {
		std::ifstream ifs( file_name.c_str( ) );
		std::vector<char> data;

		std::copy_if( std::istreambuf_iterator<char>{ ifs }, std::istreambuf_iterator<char>{ }, std::back_inserter( data ), is_hex_char ); 
		if( data.size( ) % 2 != 0 ) {
			data.pop_back( );
		}
		std::vector<uint8_t> result; 
		
		for( size_t n=0; n<data.size( )-1; n+=2 ) {
			char tmp[3] = { data[n], data[n+1], 0 };
			result.push_back( static_cast<uint8_t>(strtol( tmp, nullptr, 16 )) );
		}
	
		return result;
	}

	template<typename IteratorIn, typename IteratorOut>
	void decode_4b6b( IteratorIn message_in, size_t message_in_sz, IteratorOut message_out, size_t & message_out_sz ) {
		for( size_t n=0; n<message_out_sz; ++n ) {
			message_out[n] = 0;
		}
		message_in_sz = std::min( message_in_sz, message_out_sz );
		message_out_sz = 0;
		daw::nibble_queue nq;
		daw::bit_queue bq;

		for( size_t n=0; n<=message_in_sz; ++n ) {
			auto const & val = message_in[n];	
			if( 0 == val ) { return; }
			bq.push_back( val );
			while( bq.can_pop( 6 ) ) {
				uint8_t symbol = bq.pop_front( 6 );
				nq.push_back( daw::decode_symbol( symbol ) );
				while( nq.can_pop( 2 ) ) {
					message_out[message_out_sz++] = nq.pop_front( 2 );
				}
			}
		} 
		if( !bq.empty( ) ) {
			nq.push_back( bq.pop_all( ) );
		}
		if( !nq.empty( ) ) {
			message_out[message_out_sz++] = nq.pop_all( );
		}
	}	



	int32_t get_packet_size( uint8_t val ) {
		switch( val ) {
		case 0xA5:	// Glucose Meter
			return 7;
		case 0xAA:	// Sensor
			return 32;
		case 0xA2:	// MySentry
		case 0xA6:	// Paradigm Remote
		case 0xA7:	// Pump
		case 0xA8:	// Sensor Test
		case 0xAB:	// Sensor 2
			return -1;
		default:
			return -2;
		}
	}

	bool is_valid_packet( uint8_t const * ptr, size_t sz ) {
		/*
			# 0xa2 (162) = mysentry
			# 0xa5 (165) = glucose meter (bayer contour)
			# 0xa6 (166) = paradigm remote (MMT-503NA)
			# 0xa7 (167) = pump
			# 0xa8 (168) = sensor test
			# 0xaa (170) = sensor
			# 0xab (171) = sensor2
		*/
		if( sz < 5 ) {
			// minimum packet AABBBBBBCC - AA - packet type, BB - device id, CC - 8bit crc
			return false;
		}
		switch( *ptr ) {
			case 0xA2:	// MySentry
				return ptr[sz-1] == daw::crc8( ptr, sz - 1 );
			case 0xA5:	// Glucose Meter
				if( sz == 7 ) {
					return ptr[6] == daw::crc8( ptr, 6 );	
				}
				return false;
			case 0xA6:	// Paradigm Remote
				return ptr[sz-1] == daw::crc8( ptr, sz - 1 );
			case 0xA7:	// Pump
				if( sz == 7 ) {
					return ptr[sz-1] == daw::crc8( ptr, sz - 1 );
				}
				return false;
			case 0xA8:	// Sensor Test
				return ptr[sz-1] == daw::crc16( ptr, sz - 2 );
			case 0xAA:	// Sensor
				return ptr[sz-2] == daw::crc16( ptr, sz - 2 );
			case 0xAB:	// Sensor 2
				return ptr[sz-1] == daw::crc16( ptr, sz - 2 );
			default:
				return false;
		}
	}

	template<typename T>
	T reverse_bits( T const val ) {
		static constexpr T const bit_count = sizeof( T ) * 8;
		T bit = 0;
		T result = 0;
		while( bit < bit_count ) {
			result |= ((val >> bit) & 1) << (bit_count - bit - 1);
			++bit;
		}
		return result;
	}

}	// namespace anonymous



template<typename T, typename U>
void show_packets( T const & message_out, U message_out_sz ) {
	auto pk_sz = get_packet_size( message_out[0] );
	if( pk_sz > -2 && (pk_sz == -1 || static_cast<size_t>(pk_sz) <= message_out_sz) ) {
		if( pk_sz > 0 ) {	// Fixed packet size
			if( is_valid_packet( message_out.data( ), static_cast<size_t>(pk_sz) ) ) {
				std::cout << daw::range::make_range( message_out.data( ), message_out.data( ) + pk_sz ).to_hex_string( ) << "\n\n";
			}
		} else {	// Variable packet size, compute crc
			for( size_t m = 5; m <= message_out_sz; ++m ) {
				if( is_valid_packet( message_out.data( ), m ) ) {
					std::cout << daw::range::make_range( message_out.data( ), message_out.data( ) + m ).to_hex_string( ) << "\n\n";
					return;
				}
			}
		}
	}

};



int main( int argc, char** argv ) {
	assert( argc == 2 );
	auto const & input_file_str = argv[1];
	
	auto data = read_file( input_file_str );
	std::array<uint8_t, 25000> message_out;

	std::cout << "Forward\n";
	for( size_t n=0; n<data.size( ); ++n ) {
		size_t message_out_sz = message_out.size( );
	
		decode_4b6b( data.data( ) + n, data.size( ) - n, message_out.data( ), message_out_sz );
		show_packets( message_out, message_out_sz );
	}

	std::cout << "\nReverse Bits\n";
	for( auto & v: data ) {
		v = reverse_bits( v ); 
	}
	for( size_t n=0; n<data.size( ); ++n ) {
		size_t message_out_sz = message_out.size( );
	
		decode_4b6b( data.data( ) + n, data.size( ) - n, message_out.data( ), message_out_sz );
		show_packets( message_out, message_out_sz );
	}

	return EXIT_SUCCESS;
}

