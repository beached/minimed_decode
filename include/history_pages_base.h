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

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>
#include <daw/daw_range.h>
#include <cstdint>
#include <vector>

namespace daw {
	namespace history {
		std::string op_string( uint8_t op_code );

		using data_source_t = daw::range::Range<uint8_t *>;
		struct pump_model_t {
			uint16_t generation;
			bool larger;
			bool has_low_suspend;
			uint8_t strokes_per_unit;

			pump_model_t( ) = delete;
			pump_model_t( std::string const & model );

			virtual ~pump_model_t( );
			pump_model_t( pump_model_t const & ) = default;
			pump_model_t( pump_model_t && ) = default;
			pump_model_t & operator=( pump_model_t const & ) = default;
			pump_model_t & operator=( pump_model_t && ) = default;
		};	// pump_model_t

		boost::optional<boost::posix_time::ptime> parse_date( data_source_t const & arry );
		boost::optional<boost::posix_time::ptime> parse_timestamp( data_source_t const & arry );

		class history_entry_obj {
			uint8_t m_op_code; 
			data_source_t m_data;
			size_t m_size;
			size_t m_timestamp_offset;
			size_t m_timestamp_size;
		protected:
			history_entry_obj( data_source_t data, size_t data_size, pump_model_t, size_t timestamp_offset = 2, size_t timestamp_size = 5 );
		public:
			virtual ~history_entry_obj( );
			virtual std::string to_string( ) const;
			uint8_t op_code( ) const;
			data_source_t & data( );
			data_source_t const & data( ) const;
			size_t size( ) const;
			size_t timestamp_offset( ) const; 
			size_t timestamp_size( ) const; 

			boost::optional<boost::posix_time::ptime> timestamp( ) const;
			std::tuple<uint8_t, size_t, size_t, size_t> register_event_type( ) const;

			history_entry_obj( ) = delete;
			history_entry_obj( history_entry_obj const & ) = default;
			history_entry_obj( history_entry_obj && ) = default;
			history_entry_obj & operator=( history_entry_obj const & ) = default;
			history_entry_obj & operator=( history_entry_obj && ) = default;
		};	// history_entry_obj

		template<uint8_t child_op_code>
		class history_entry: public history_entry_obj {
		protected:
			history_entry( data_source_t data, size_t data_size, pump_model_t pump_model, size_t timestamp_offset = 2, size_t timestamp_size = 5 ): 
				history_entry_obj{ std::move( data ), data_size, std::move( pump_model ), timestamp_offset, timestamp_size } { }

		public:
			virtual ~history_entry( ) = default;
			history_entry( history_entry const & ) = default;
			history_entry( history_entry && ) = default;
			history_entry & operator=( history_entry const & ) = default;
			history_entry & operator=( history_entry && ) = default;			
		};	// history_entry

		template<uint8_t child_op_code, size_t child_size = 7, size_t child_timestamp_offset = 2, size_t child_timestamp_size = 5>
		struct history_entry_static: public history_entry<child_op_code> {
			history_entry_static( data_source_t data, pump_model_t pump_model ):
				history_entry<child_op_code>{ std::move( data ), child_size, std::move( pump_model ), child_timestamp_offset, child_timestamp_size } { }

			virtual ~history_entry_static( ) = default;
			history_entry_static( history_entry_static const & ) = default;
			history_entry_static( history_entry_static && ) = default;
			history_entry_static & operator=( history_entry_static const & ) = default;
			history_entry_static & operator=( history_entry_static && ) = default;
		};	// history_entry_static	

		std::ostream & operator<<( std::ostream & os, history_entry_obj const & entry );

		std::unique_ptr<history_entry_obj> create_history_entry( data_source_t & data, pump_model_t pump_model, size_t & position );
	}	// namespace history
}	// namespace daw

