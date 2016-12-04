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

namespace {
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
		return boost::posix_time::second_clock::local_time( ).date( ).year( );
	}

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
	
		if( result.back( ) == 0 ) {
			result.pop_back( ); // null terminator
		}
		result.pop_back( ); // crc
		result.pop_back( ); // crc

		return result;
	}


}	// namespace anonymous

int main( int argc, char** argv ) {

	assert( argc > 2 );
	auto const & pump_model_str = argv[1];
	auto const & input_file_str = argv[2];
	daw::history::pump_model_t pump_model( pump_model_str );
	
	auto data = read_file( input_file_str );

	auto range = daw::range::make_range( data.data( ), data.data( ) + data.size( ) );

	std::vector<std::unique_ptr<daw::history::history_entry_obj>> entries;
	size_t pos = 0;

	auto good_item = []( auto const & i ) {
		if( !i ) {
			return false;
		}
		if( !i->timestamp( ) ) {
			return false;
		}
		return i->timestamp( )->date( ).year( ) == current_year( );
	};

	auto reasonible_year = []( auto const & i ) {
		if( i->timestamp( ) ) {
			auto item_year = i->timestamp( )->date( ).year( );
			auto this_year = current_year( );
			if( item_year < this_year - 2 ) {
				return false;
			}
			if( item_year > this_year + 2 ) {
				return false;
			}
		}
		return true;	// Not all items have timestamps
	};

	std::cout << "{ \"values\": [\n";
	bool first = true;
	auto buff_backup = std::cerr.rdbuf( );	
	while( !range.at_end( ) ) {
		std::stringstream ss_err;
		std::cerr.rdbuf( ss_err.rdbuf( ) );
		auto item = daw::history::create_history_entry( range, pump_model, pos );

		if( item ) {
			if( item->op_code( ) == 0x0 ) {
				continue;	// These where the null entries
			}
			if( !reasonible_year( item ) ) {
				std::cerr << "WARNING: The year does not look correct, outside of plus or minus 2 years from current system year\n";
			}
			if( first ) {
				first = false;
			} else {
				std::cout << ",";
			}
			std::cout << "{ \"loc\": \"" << std::dec << std::dec << pos+1 << "/" << data.size( ) << "\", ";
			std::cout << item->to_string( ) << "}";
			entries.push_back( std::move( item ) );
		} else {
			std::cerr << std::dec << std::dec << pos+1 << "/" << data.size( ) << ": ";
			std::cerr << "ERROR: data( ";
			auto err_start = pos;
			safe_advance( range, 1 );
			while( !range.at_end( ) && (range[0] == 0 || !(item = daw::history::create_history_entry( range, pump_model, pos )) || !good_item( item ) )) {
				safe_advance( range, 1 );
				++pos;
			}
			auto offset = item ? item->size( ) - 1 : 0;
			std::cerr << (pos-(err_start+offset)) << " ) { ";
			std::cerr << daw::range::make_range( data.data( ) + err_start, data.data( ) + pos - offset ).to_hex_string( ) << " }\n";
			if( !range.at_end( ) ) {
				if( first ) {
					first = false;
				} else {
					std::cerr << ",";
				}
				std::cerr << "{ \"loc\": \"" << std::dec << std::dec << (pos-offset)+1 << "/" << data.size( ) << "\", " << item->to_string( ) << "}";
				entries.push_back( std::move( item ) );
			}
		}
		if( ss_err.tellp( ) != 0 ) {
			std::cout << ss_err.str( );
		}
		std::cout << "\n";
	}
	std::cerr.rdbuf( buff_backup );
	std::cout << "]}\n\n";
	return EXIT_SUCCESS;
}

