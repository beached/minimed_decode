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

#include <boost/optional.hpp>
#include <daw/daw_range.h>

namespace daw {
	namespace history {

		struct timestamp_t {
			struct {
				uint16_t year;
				uint8_t month;
				uint8_t day;
			} date;

			struct time_t {
				uint8_t hour;
				uint8_t minute;
				uint8_t second;
			};
			boost::optional<time_t> time;
			
			bool has_time( ) const {
				return static_cast<bool>(time);
			}
			
			static timestamp parse_timestamp( daw::range::Range arry ) {
				assert( arry.size( ) >= 5 );
				timestamp result;
				result.time = time_t{ };
				result.time.second = arry[0] & 0b00111111;
				result.time.minute = arry[1] & 0b00111111;
				result.time.hour = arry[2] & 0b00011111;
				result.date.day = arry[3] & 0b00011111;
				result.date.month = ((arry[0] >> 4) & 0b00001100) + (arry[1] >> 6);
				result.date.year = 2000 + (arry[4] & 0b01111111);
				return result;
			}

			static timestamp parse_date( daw::range::Range arry ) {
				assert( arry.size( ) >= 2 ); 
				timestamp result;
				result.date.day = arry[0] & 0b00011111;
				result.date.month = ((arry[0] & 0b11100000) >> 4) + ((arry[1] & 0b10000000) >> 7);
				result.date.year = 2000 + (arry[1] & 0b01111111);
				return result;	
			}
		};	// timestamp_t

		template<uint8_t child_opcode>
		class history_entry_obj {
			static uint8_t const m_opcode = child_opcode;
			daw::range::Range m_data;
			size_t m_size;
		public:
			histor_entry_obj( daw::range::Range & data, size_t data_size ): 
					m_data{ data.shrink( data_size ) } { }

			virtual ~history_entry_obj( ) = default;
			virtual size_t timestamp_offset( ) const = 0;
			virtual size-t timestamp_size( ) const = 0;

			std::tuple<uint8_t, size_t, size_t> register( ) const override {
				return std::make_tuple<uint8_t, size_t, size_t>( this->opcode( ), this->size( ), this->timestamp_offset( ) );
			}

			virtual boost::optional<timestamp_t> timestamp( ) const {
				switch( timestamp_size( ) ) {
					case 2:
						return timestamp_t::parse_date( m_data.slice( child->m_timestamp_offset ) );
					case 5:
						return timestamp_t::parse_timestamp( m_data.slice( child->m_timestamp_offset ) );
					default:
						return boost::optional<timestamp_t>{ };
					}
				}
			}

			uint8_t opcode( ) const {
				return m_opcode;	
			};
			
			daw::range::Range & data( ) {
				return m_data;
			}	

			daw::range::Range const & data( ) const {
				return m_data;
			}

			virtual daw::range::Range & size( ) {
				return m_size;
			}	

			virtual daw::range::Range const & size( ) const {
				return m_size;
			}
		};	// history_entry_obj

		template<typename Derived, uint8_t child_opcode>
		class histroy_entry: public history_entry_obj<child_opcode> {
			Derived * child;
		public:
			virtual ~history_entry( ) = default;
			history_entry( Derived * derived, daw::range::Range data, size_t data_size ): 
					history_entry_obj{ std::move( data ), data_size }, 
					child( derived ) {

				assert( nullptr != child );
			}

			size_t timestamp_offset( ) const override {
				return child->m_timestamp_offset;
			}
		
			size_t timestamp_size( ) const override {
				return child->m_timestamp_size;
			}

		};	// history_entry

		template<uint_t child_opcode, size_t child_size = 7, size_t child_timestamp_offset = 2, size_t child_timestamp_size = 5>
		class history_entry_static: public history_entry<history_entry_static<child_opcode, child_size, child_timestamp_offset>, child_opcode> {
			static size_t const m_timestamp_offset = child_timestamp_offset;
			static size_t const m_timestamp_size = child_timestamp_size;
			
			history_entry_static( daw::range::Range data ):
					history_entry<history_entry_static<child_opcode, child_size, child_timestamp_offset>>{ *this, std::move( data ), child_size },

			history_entry_static( daw::range::Range data, pump_model_t ):
					history_entry<history_entry_static<child_opcode, child_size, child_timestamp_offset>>{ *this, std::move( data ), child_size },


			virtual ~history_entry_static( ) = default;
		};	// history_entry_static	

		// Known History Entries in order of opcode
		struct hist_bolus_normal: public history_entry<hist_bolus_normal, 0x01> {
			size_t m_timestamp_offset;

			hist_bolus_normal( daw::range::Range data, pump_model_t pump_model ):
					history_entry<hist_bolus_normal>{ this, std::move( data ), pump_model.larger ? 13 : 9 },
					m_timestamp_offset{ pump_model.larger ? 8 : 4 } { }
			
			virtual ~hist_bolus_normal( ) = default;
		};	// hist_bolus_normal

		using hist_prime = history_entry_static<0x03, 10, 5>;
		using hist_alarm_pump = history_entry_static<0x06, 9, 4>;

		struct hist_result_daily_total: public history_entry<hist_result_daily_total, 0x07> {
			static size_t const m_timestamp_size = 2;
			size_t m_timestamp_offset;

			hist_result_daily_total( daw::range::Range data, pump_model_t pump_model ):
					history_entry<hist_result_daily_total>{ this, std::move( data ),  pump_model.larger ? 10 : 7 },
					m_timestamp_offset{ pump_model.larger ? 8 : 4 } { }
		};	// hist_result_daily_total

		using hist_change_basal_profile_pattern = history_entry_static<0x08, 152>;
		using hist_change_basal_profile = history_entry_static<0x09, 152>;
		using hist_cal_bg_for_ph = history_entry_static<0x0A>;
		using hist_alarm_sensor = history_entry_static<0x0B, 8>;
		using hist_clear_alarm = history_entry_static<0x0C>;
		using hist_select_basal_profile = history_entry_static<0x14>;
		using hist_temp_basal_duration = history_entry_static<0x16>;
		using hist_change_time = history_entry_static<0x17, 14, 9>;
		using hist_journal_entry_pump_low_battery = history_entry_static<0X19>;
		using hist_battery = history_entry_static<0X1A>;
		using hist_suspend = history_entry_static<0X1E>;
		using hist_resume = history_entry_static<0X1F>;
		using hist_rewind = history_entry_static<0X21>;
		using hist_change_child_block_enable = history_entry_static<0X23>;
		using hist_change_max_bolus = history_entry_static<0X24>;
		using hist_enable_disable_remote = history_entry_static<0X26, 21>;
		using hist_change_max_basal = history_entry_static<0X2C>;
		using hist_change_bg_reminder_offset = history_entry_static<0X31>;
		using hist_change_alarm_clock_time = history_entry_static<0X32, 14>;
		using hist_temp_basal = history_entry_static<0X33, 8>;
		using hist_pump_low_reservoir = history_entry_static<0X34>;
		using hist_alarm_clock_reminder = history_entry_static<0X35>;
		using hist_questionable_3b = history_entry_static<0X3B>;
		using hist_change_paradigm_linkid = history_entry_static<0X3C, 21>;
		using hist_bg_received = history_entry_static<0X3F, 10>;
		using hist_exercise_marker = history_entry_static<0X41, 8>;

		using hist_change_sensor_rate_of_change_alert_setup = history_entry_static<0X56, 12>;
		using hist_change_bolus_scroll_step_size = history_entry_static<0X57>;

		struct hist_change_bolus_wizard_setup: public history_entry<hist_result_daily_total, 0x5A> {
			hist_change_bolus_wizard_setup( daw::range::Range data, pump_model_t pump_model ):
					history_entry<hist_result_daily_total>{ this, std::move( data ),  pump_model.larger ? 144 : 124 } { } 
		};	// hist_change_bolus_wizard_setup

		struct hist_change_bolus_wizard_setup: public history_entry<hist_result_daily_total, 0x5B> {
			hist_change_bolus_wizard_setup( daw::range::Range data, pump_model_t pump_model ):
					history_entry<hist_result_daily_total>{ this, std::move( data ),  pump_model.larger ? 22 : 20 } { } 
		};	// hist_change_bolus_wizard_setup

		struct hist_unabsorbed_insulin: public history_entry<hist_result_daily_total, 0x5C> {
			hist_unabsorbed_insulin( daw::range::Range data, pump_model_t pump_model ):
					history_entry<hist_result_daily_total>{ this, std::move( data ),  std::max( data[1], 2 ) } { } 
		};	// hist_unabsorbed_insulin

		using hist_change_variable_bolus = history_entry_static<0X5E>;
		using hist_change_audio_bolus = history_entry_static<0X5F>;
		using hist_change_bg_reminder_enable = history_entry_static<0X60>;
		using hist_change_alarm_clock_enable = history_entry_static<0X61>;
		using hist_change_temp_basal_type = history_entry_static<0X62>;
		using hist_change_alarm_notify_mode = history_entry_static<0X63>;
		using hist_change_time_format = history_entry_static<0X64>;
		using hist_change_reservoir_warning_time = history_entry_static<0X65>;
		using hist_change_bolus_reminder_enable = history_entry_static<0X66>;
		using hist_change_bolus_reminder_time = history_entry_static<0X67>;
		using hist_delete_bolus_reminder_time = history_entry_static<0X68, 9>;
		using hist_delete_alarm_clock_time = history_entry_static<0X6A, 14>;
		using hist_model522resulttotals = history_entry_static<0X6D, 44, 1, 2>;
		using hist_sara_6e = history_entry_static<0X6E, 52, 1, 2>;
		using hist_change_carb_units = history_entry_static<0X6F>;
		using hist_basal_profile_start = history_entry_static<0X7B, 10>;
		using hist_change_watch_dog_enable = history_entry_static<0X7C>;
		using hist_change_other_device_id = history_entry_static<0X7D, 37>;
		using hist_change_watch_dog_marriage_profile = history_entry_static<0X81, 12>;
		using hist_delete_other_device_id = history_entry_static<0X82, 12>;
		using hist_change_capture_event_enable = history_entry_static<0X83>;

	}	// namespace history
}	// namespace daw

