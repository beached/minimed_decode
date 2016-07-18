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

#include "history_pages.h"
#include <iostream>
#include <streambuf>
#include <fstream>
#include <cstdlib>
#include <chrono>

template<typename Data>
void display( Data const & data ) {
	for( auto it = data.begin( ); it != data.end( ); ++it ) {
		if( it != data.begin( ) ) {
			std::cout << " ";
		}
		std::cout << std::hex << std::setw( 2 ) << std::setfill( '0' ) << static_cast<int>(*it); 
	}
}

auto current_year( ) {
	auto result = boost::posix_time::second_clock::local_time( ).date( ).year( );
	return result;
}

std::string read_file( std::string file_name ) {
	std::ifstream ifs( file_name.c_str( ) );
	std::string result;

	ifs.seekg( 0, std::ios::end );
	result.reserve( static_cast<size_t>(ifs.tellg( )) );
	ifs.seekg( 0, std::ios::beg );

	result.assign( (std::istreambuf_iterator<char>( ifs )), std::istreambuf_iterator<char>( ) );

	return result;
}

int main( int argc, char** argv ) {
	assert( argc > 2 );
	daw::history::pump_model_t pump_model( argv[1] );
	
	auto data = read_file( argv[2] );

	std::vector<uint8_t> v;
	for( size_t n = 0; n < data.size( ); n += 2 ) {
		while( std::isspace( data[n] ) ) {
			++n;
		}
		char tmp[3] = { data[n], data[n + 1], 0 };
		v.push_back( static_cast<uint8_t>(strtol( tmp, nullptr, 16 )) );
	}
	if( v.back( ) == 0 ) {
		v.pop_back( ); // null terminator
	}
	v.pop_back( ); // crc
	v.pop_back( ); // crc
	auto range = daw::range::make_range( v.data( ), v.data( ) + v.size( ) );

	std::vector<std::unique_ptr<daw::history::history_entry_obj>> entries;
	size_t pos = 0;

	auto good_item = []( auto const & i ) {
		if( !i ) {
			return false;
		}
		if( !i->timestamp( ) ) {
			return false;
		}
		uint16_t const item_year = i->timestamp( )->date( ).year( );
		static uint16_t const now_year = current_year( );
		return item_year == now_year;
	};

	while( !range.at_end( ) ) {
		std::cout << std::dec << std::dec << pos+1 << "/" << v.size( ) << ": ";
		auto item = daw::history::create_history_entry( range, pump_model, pos );
		if( item ) {
			std::cout << *item;
			entries.push_back( std::move( item ) );
		} else {
			std::cout << "ERROR: data( ";
			auto err_start = pos;
			safe_advance( range, 1 );
			while( !range.at_end( ) && (range[0] == 0 || !(item = daw::history::create_history_entry( range, pump_model, pos )) || !good_item( item ) )) {
				safe_advance( range, 1 );
				++pos;
			}
			auto offset = item ? item->size( ) - 1 : 0;
			std::cout << (pos-(err_start+offset)) << " ) { ";
			display( daw::range::make_range( v.data( ) + err_start, v.data( ) + pos - offset ) );
			std::cout << " }\n";
			if( !range.at_end( ) ) {
				std::cout << std::dec << std::dec << (pos-offset)+1 << "/" << v.size( ) << ": " << *item;
				entries.push_back( std::move( item ) );
			}
		}
		std::cout << "\n";
	}
	return EXIT_SUCCESS;
}

