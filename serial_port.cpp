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


#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <vector>

#include "serial_port.h"

namespace daw { 
	std::mutex SerialPort::s_mutex;

	SerialPort::~SerialPort( ) { }

	SerialPort::SerialPort( boost::string_ref device, uint32_t bps ): 
			m_io{ }, 
			m_serial_port{ m_io, device.c_str( ) } {

		m_serial_port.set_option( boost::asio::serial_port_base::baud_rate( bps ) );
		m_serial_port.set_option( boost::asio::serial_port_base::character_size( 8 ) );
		m_serial_port.set_option( boost::asio::serial_port_base::parity( boost::asio::serial_port_base::parity::none ) );
		m_serial_port.set_option( boost::asio::serial_port_base::stop_bits( boost::asio::serial_port_base::stop_bits::one ) );	// default one
		m_serial_port.set_option( boost::asio::serial_port_base::flow_control( boost::asio::serial_port_base::flow_control::none ) );
	}

	std::vector<uint8_t> SerialPort::receive( size_t count ) {
		std::vector<uint8_t> ret{ count, static_cast<uint8_t>( 0 ) };
		{
			std::lock_guard<std::mutex> lock( s_mutex );
			boost::asio::read( m_serial_port, boost::asio::buffer( &ret[0], count ) );
		}
		return ret;
	}


	void SerialPort::send( uint8_t value ) {
		std::lock_guard<std::mutex> lock( s_mutex );
		boost::asio::write( m_serial_port, boost::asio::buffer( &value, 1 ) );
	}

	void SerialPort::send( std::vector<uint8_t> const & values, size_t count ) {
		if( 0 == count ) {
			count = values.size( );
		}
		std::lock_guard<std::mutex> lock( s_mutex );
		boost::asio::write( m_serial_port, boost::asio::buffer( values.data( ), count ) );
	}

}

