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

#include <boost/asio.hpp>
#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <mutex.hpp>
#include <vector>

namespace daw { 
	class SerialPort {
		boost::asio::io_service m_io;
		boost::asio::serial_port m_serial_port;
		static mutable std::mutex s_mutex;
	public:
		virtual ~SerialPort( );
		SerialPort( SerialPort && ) = default;
		SerialPort & operator=( SerialPort && ) = default;
		SerialPort( ) = delete;
		SerialPort( SerialPort const & ) = delete;
		SerialPort & operator=( SerialPort const & ) = delete;

		SerialPort( boost::string_ref device, uint32_t bps = 57600 );
		std::vector<uint8_t> receive( size_t count );
		
		void send( uint8_t value );
		void send( std::vector<uint8_t> const & values, size_t count = 0 );

		void send( std::vector<uint8_t> const & values, 

		bool is_open( ) const {
			return mSerialPort.is_open( );
		}

		void close( ) {
			mSerialPort.close( );
		}
	};
}
