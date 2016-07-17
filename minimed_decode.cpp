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
#include <daw/daw_memory_mapped_file.h>
#include <iostream>

template<typename Data>
void display( Data const & data ) {
	for( auto it = data.begin( ); it != data.end( ); ++it ) {
		std::cout << std::hex << *it << "  ";
	}
	std::cout << std::endl;
}

int main( int argc, char** argv ) {
	assert( argc > 2 );
	daw::history::pump_model_t pump_model( argv[1] );
	daw::filesystem::MemoryMappedFile<uint8_t> data( argv[2] );
	std::vector<uint8_t> v( data.size( ), (uint8_t)0 );
	std::copy( data.begin( ), data.end( ), v.begin( ) );
	auto range = daw::range::make_range( v );

	std::vector<std::unique_ptr<daw::history::history_entry_obj>> entries;

	while( !range.at_end( ) ) {
		auto item = daw::history::create_history_entry( range, pump_model );
		if( item ) {
			entries.push_back( std::move( item ) );
		} else {
			std::cout << "Error found\n";
			display( range );
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

