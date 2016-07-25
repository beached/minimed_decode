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

#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <vector>
#include "serial_port.h"
#include "subg_rfspy.h"

namespace daw {
	SubGRFSpy::SubGRFSpy( boost::string_ref device_port ): 
			m_serial_port{ device_port, 19200 } {

	}

	SubGRFSpy::~SubGRFSpy( ) { }

	void SubGRFSpy::send_command( uint8_t cmd, SubGRFSpy::response_t on_response ) { 
		send_command( cmd, std::vector<uint8_t>{ }, on_response );		
	}
	
	void SubGRFSpy::send_command( uint8_t cmd, std::vector<uint8_t> params, SubGRFSpy::response_t on_response ) {

	}
}
