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

#include <boost/optional.hpp>
#include <daw/daw_range.h>
#include <sstream>
#include <tuple>
#include "history_pages.h"

namespace daw {
	namespace history {
		namespace {
			template<typename To, typename From>
			To convert_to( From const & value ) {
				std::stringstream ss;
				ss << value;
				To result;
				ss >> result;
				return result;
			}
		}	// namespace anonymous

		pump_model_t::pump_model_t( std::string const & model ):
			generation( convert_to<uint16_t>( model ) % 100u ),
			larger { generation >= 23 },
			has_low_suspend { generation >= 51 },
			strokes_per_unit( generation >= 23 ? 40 : 10 ) { }

		pump_model_t::~pump_model_t( ) { }

		history_entry_obj::history_entry_obj( data_source_t data, size_t data_size, pump_model_t, size_t timestamp_offset, size_t timestamp_size ):
			m_opcode { data[0] },
			m_data { data.shrink( data_size ) },
			m_size { data_size }, 
			m_timestamp_offset { timestamp_offset },
			m_timestamp_size { timestamp_size } { }

		history_entry_obj::~history_entry_obj( ) { };

		std::tuple<uint8_t, size_t, size_t> history_entry_obj::register_event_type( ) const {
			return std::make_tuple( this->opcode( ), this->size( ), this->timestamp_offset( ) );
		}

		boost::optional<timestamp_t> history_entry_obj::timestamp( ) const {
			switch( this->timestamp_size( ) ) {
			case 2:
				return timestamp_t::parse_date( m_data.slice( this->timestamp_offset( ) ) );
			case 5:
				return timestamp_t::parse_timestamp( m_data.slice( this->timestamp_offset( ) ) );
			default:
				return boost::optional<timestamp_t>{ };
			}
		}

		uint8_t history_entry_obj::opcode( ) const {
			return this->m_opcode;
		}

		data_source_t & history_entry_obj::data( ) {
			return this->m_data;
		}

		data_source_t const & history_entry_obj::data( ) const {
			return this->m_data;
		}

		size_t history_entry_obj::size( ) const {
			return this->m_size;
		}

		size_t history_entry_obj::timestamp_offset( ) const {
			return this->m_timestamp_offset;
		}

		size_t history_entry_obj::timestamp_size( ) const {
			return this->m_timestamp_size;
		}

		hist_bolus_normal::~hist_bolus_normal( ) { }
		hist_result_daily_total::~hist_result_daily_total( ) { }
		hist_change_sensor_setup::~hist_change_sensor_setup( ) { }
		hist_change_bolus_wizard_setup::~hist_change_bolus_wizard_setup( ) { }
		hist_change_bolus_wizard_estimate::~hist_change_bolus_wizard_estimate( ) { }
		hist_unabsorbed_insulin::~hist_unabsorbed_insulin( ) { }

		bool timestamp_t::has_time( ) const {
			return static_cast<bool>(time);
		}

		timestamp_t timestamp_t::parse_timestamp( data_source_t arry ) {
			assert( arry.size( ) >= 5 );
			timestamp_t result;
			result.time = time_t { };
			result.time->second = arry[0] & 0b00111111;
			result.time->minute = arry[1] & 0b00111111;
			result.time->hour = arry[2] & 0b00011111;
			result.date.day = arry[3] & 0b00011111;
			result.date.month = ((arry[0] >> 4) & 0b00001100) + (arry[1] >> 6);
			result.date.year = 2000 + (arry[4] & 0b01111111);
			return result;
		}

		timestamp_t timestamp_t::parse_date( data_source_t arry ) {
			assert( arry.size( ) >= 2 );
			timestamp_t result;
			result.date.day = arry[0] & 0b00011111;
			result.date.month = ((arry[0] & 0b11100000) >> 4) + ((arry[1] & 0b10000000) >> 7);
			result.date.year = 2000 + (arry[1] & 0b01111111);
			return result;
		}

		hist_bolus_normal::hist_bolus_normal( data_source_t data, pump_model_t pump_model ):
			history_entry<0x01>( std::move( data ), pump_model.larger ? 13 : 9, std::move( pump_model ) ),
			m_timestamp_offset( pump_model.larger ? 8 : 4 ) { }

		hist_result_daily_total::hist_result_daily_total( data_source_t data, pump_model_t pump_model ):
			history_entry<0x07>( std::move( data ), pump_model.larger ? 10 : 7, std::move( pump_model ) ),
			m_timestamp_offset( pump_model.larger ? 8 : 4 ) { }


		hist_change_sensor_setup::hist_change_sensor_setup( data_source_t data, pump_model_t pump_model ):
			history_entry<0x50>( std::move( data ), pump_model.has_low_suspend ? 41 : 37, std::move( pump_model ) ) { }

		hist_change_bolus_wizard_setup::hist_change_bolus_wizard_setup( data_source_t data, pump_model_t pump_model ):
			history_entry<0x5A>( std::move( data ), pump_model.larger ? 144 : 124, std::move( pump_model ) ) { }

		hist_change_bolus_wizard_estimate::hist_change_bolus_wizard_estimate( data_source_t data, pump_model_t pump_model ):
			history_entry<0x5B> { std::move( data ), static_cast<size_t>(pump_model.larger ? 22 : 20), pump_model } { }

		hist_unabsorbed_insulin::hist_unabsorbed_insulin( data_source_t data, pump_model_t pump_model ):
			history_entry<0x5C>( std::move( data ), (data[1] > 2 ? data[1] : 2), std::move( pump_model ) ) { }

		namespace {
			template<typename... Args>
			std::unique_ptr<history_entry_obj> create_history_entry_impl( uint8_t opcode, Args&&... arg ) {
				return std::unique_ptr<history_entry_obj>( [opcode]( Args&&... args ) -> history_entry_obj* {
					switch( opcode ) {
							case 0x00: return new hist_skip( std::forward<Args>( args )... );
							case 0x01: return new hist_bolus_normal( std::forward<Args>( args )... );
							case 0x03: return new hist_prime( std::forward<Args>( args )... );
							case 0x06: return new hist_alarm_pump( std::forward<Args>( args )... );
							case 0x07: return new hist_result_daily_total( std::forward<Args>( args )... );
							case 0x08: return new hist_change_basal_profile_pattern( std::forward<Args>( args )... );
							case 0x09: return new hist_change_basal_profile( std::forward<Args>( args )... );
							case 0x0A: return new hist_cal_bg_for_ph( std::forward<Args>( args )... );
							case 0x0B: return new hist_alarm_sensor( std::forward<Args>( args )... );
							case 0x0C: return new hist_clear_alarm( std::forward<Args>( args )... );
							case 0x14: return new hist_select_basal_profile( std::forward<Args>( args )... );
							case 0x16: return new hist_temp_basal_duration( std::forward<Args>( args )... );
							case 0x17: return new hist_change_time( std::forward<Args>( args )... );
							case 0x19: return new hist_pump_low_battery( std::forward<Args>( args )... );
							case 0x1A: return new hist_battery( std::forward<Args>( args )... );
							case 0x1E: return new hist_suspend( std::forward<Args>( args )... );
							case 0x1F: return new hist_resume( std::forward<Args>( args )... );
							case 0x21: return new hist_rewind( std::forward<Args>( args )... );
							case 0x23: return new hist_change_child_block_enable( std::forward<Args>( args )... );
							case 0x24: return new hist_change_max_bolus( std::forward<Args>( args )... );
							case 0x26: return new hist_enable_disable_remote( std::forward<Args>( args )... );
							case 0x2C: return new hist_change_max_basal( std::forward<Args>( args )... );
							case 0x31: return new hist_change_bg_reminder_offset( std::forward<Args>( args )... );
							case 0x32: return new hist_change_alarm_clock_time( std::forward<Args>( args )... );
							case 0x33: return new hist_temp_basal( std::forward<Args>( args )... );
							case 0x34: return new hist_pump_low_reservoir( std::forward<Args>( args )... );
							case 0x35: return new hist_alarm_clock_reminder( std::forward<Args>( args )... );
							case 0x3B: return new hist_questionable_3b( std::forward<Args>( args )... );
							case 0x3C: return new hist_change_paradigm_linkid( std::forward<Args>( args )... );
							case 0x3F: return new hist_bg_received( std::forward<Args>( args )... );
							case 0x40: return new hist_meal_marker( std::forward<Args>( args )... );
							case 0x41: return new hist_exercise_marker( std::forward<Args>( args )... );
							case 0x42: return new hist_manual_insulin_marker( std::forward<Args>( args )... );
							case 0x43: return new hist_other_marker( std::forward<Args>( args )... );
							case 0x50: return new hist_change_sensor_setup( std::forward<Args>( args )... );
							case 0x56: return new hist_change_sensor_rate_of_change_alert_setup( std::forward<Args>( args )... );
							case 0x57: return new hist_change_bolus_scroll_step_size( std::forward<Args>( args )... );
							case 0x5A: return new hist_change_bolus_wizard_setup( std::forward<Args>( args )... );
							case 0x5B: return new hist_change_bolus_wizard_estimate( std::forward<Args>( args )... );
							case 0x5C: return new hist_unabsorbed_insulin( std::forward<Args>( args )... );
							case 0x5E: return new hist_change_variable_bolus( std::forward<Args>( args )... );
							case 0x5F: return new hist_change_audio_bolus( std::forward<Args>( args )... );
							case 0x60: return new hist_change_bg_reminder_enable( std::forward<Args>( args )... );
							case 0x61: return new hist_change_alarm_clock_enable( std::forward<Args>( args )... );
							case 0x62: return new hist_change_temp_basal_type( std::forward<Args>( args )... );
							case 0x63: return new hist_change_alarm_notify_mode( std::forward<Args>( args )... );
							case 0x64: return new hist_change_time_format( std::forward<Args>( args )... );
							case 0x65: return new hist_change_reservoir_warning_time( std::forward<Args>( args )... );
							case 0x66: return new hist_change_bolus_reminder_enable( std::forward<Args>( args )... );
							case 0x67: return new hist_change_bolus_reminder_time( std::forward<Args>( args )... );
							case 0x68: return new hist_delete_bolus_reminder_time( std::forward<Args>( args )... );
							case 0x6A: return new hist_delete_alarm_clock_time( std::forward<Args>( args )... );
							case 0x6D: return new hist_model_522_result_totals( std::forward<Args>( args )... );
							case 0x6E: return new hist_sara_6e( std::forward<Args>( args )... );
							case 0x6F: return new hist_change_carb_units( std::forward<Args>( args )... );
							case 0x7B: return new hist_basal_profile_start( std::forward<Args>( args )... );
							case 0x7C: return new hist_change_watch_dog_enable( std::forward<Args>( args )... );
							case 0x7D: return new hist_change_other_device_id( std::forward<Args>( args )... );
							case 0x81: return new hist_change_watch_dog_marriage_profile( std::forward<Args>( args )... );
							case 0x82: return new hist_delete_other_device_id( std::forward<Args>( args )... );
							case 0x83: return new hist_change_capture_event_enable( std::forward<Args>( args )... );
							default: return nullptr;
					}
				}( std::forward<Args>(arg)... ) );
			}
		}	// namespace anonymous

		std::unique_ptr<history_entry_obj> create_history_entry( data_source_t & data, pump_model_t pump_model, size_t & position ) {
			auto result = create_history_entry_impl( data[0], data, std::move( pump_model ) );
			if( !result || data.size( ) < result->size( ) ) {
				return nullptr;
			}
			position += result->size( );
			data.advance( result->size( ) );
			return result;
		}
	}	// namespace history
}	// namespace daw

