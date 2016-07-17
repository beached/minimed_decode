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

template<typename Data>
void display( Data const & data ) {
	for( auto it = data.begin( ); it != data.end( ); ++it ) {
		std::cout << std::hex << std::setw( 2 ) << std::setfill( '0' ) << (int)*it << "  ";
	}
	std::cout << std::endl;
}

std::string read_file( std::string file_name ) {
	std::ifstream ifs( file_name.c_str( ) );
	std::string result;

	ifs.seekg( 0, std::ios::end );
	result.reserve( ifs.tellg( ) );
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

	while( !range.at_end( ) ) {
		auto item = daw::history::create_history_entry( range, pump_model, pos );
		if( item ) {
			entries.push_back( std::move( item ) );
		} else {
			std::cout << "Error found at position " << pos << "/" << v.size( ) << "\n";
			display( range );
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

