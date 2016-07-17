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
#include <cstdint>

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
			
			bool has_time( ) const;
			
			static timestamp_t parse_timestamp( data_source_t arry );
			static timestamp_t parse_date( data_source_t arry );
		};	// timestamp_t


		class history_entry_obj {
			uint8_t m_opcode; 
			data_source_t m_data;
			size_t m_size;
			size_t m_timestamp_offset;
			size_t m_timestamp_size;
		protected:
			history_entry_obj( data_source_t data, size_t data_size, pump_model_t, size_t timestamp_offset = 2, size_t timestamp_size = 5 );
		public:
			virtual ~history_entry_obj( );
			virtual std::string to_string( ) const;
			uint8_t opcode( ) const;
			data_source_t & data( );
			data_source_t const & data( ) const;
			size_t size( ) const;
			size_t timestamp_offset( ) const; 
			size_t timestamp_size( ) const; 

			boost::optional<timestamp_t> timestamp( ) const;
			std::tuple<uint8_t, size_t, size_t> register_event_type( ) const;

			history_entry_obj( ) = delete;
			history_entry_obj( history_entry_obj const & ) = default;
			history_entry_obj( history_entry_obj && ) = default;
			history_entry_obj & operator=( history_entry_obj const & ) = default;
			history_entry_obj & operator=( history_entry_obj && ) = default;
		};	// history_entry_obj

		template<uint8_t child_opcode>
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

		template<uint8_t child_opcode, size_t child_size = 7, size_t child_timestamp_offset = 2, size_t child_timestamp_size = 5>
		struct history_entry_static: public history_entry<child_opcode> {
			history_entry_static( data_source_t data, pump_model_t pump_model ):
				history_entry<child_opcode>{ std::move( data ), child_size, std::move( pump_model ), child_timestamp_offset, child_timestamp_size } { }

			virtual ~history_entry_static( ) = default;
			history_entry_static( history_entry_static const & ) = default;
			history_entry_static( history_entry_static && ) = default;
			history_entry_static & operator=( history_entry_static const & ) = default;
			history_entry_static & operator=( history_entry_static && ) = default;
		};	// history_entry_static	

		// Known History Entries in order of opcode
		using hist_skip = history_entry_static<0x00, 1, 0, 0>;

		struct hist_bolus_normal: public history_entry<0x01> {
			hist_bolus_normal( data_source_t data, pump_model_t pump_model );
			virtual ~hist_bolus_normal( );
			hist_bolus_normal( hist_bolus_normal const & ) = default;
			hist_bolus_normal( hist_bolus_normal && ) = default;
			hist_bolus_normal & operator=( hist_bolus_normal const & ) = default;
			hist_bolus_normal & operator=( hist_bolus_normal && ) = default;

		};	// hist_bolus_normal

		using hist_prime = history_entry_static<0x03, 10, 5>;
		using hist_alarm_pump = history_entry_static<0x06, 9, 4>;

		struct hist_result_daily_total: public history_entry<0x07> {
			hist_result_daily_total( data_source_t data, pump_model_t pump_model );

			virtual ~hist_result_daily_total( );
			hist_result_daily_total( hist_result_daily_total const & ) = default;
			hist_result_daily_total( hist_result_daily_total && ) = default;
			hist_result_daily_total & operator=( hist_result_daily_total const & ) = default;
			hist_result_daily_total & operator=( hist_result_daily_total && ) = default;
		};	// hist_result_daily_total

		using hist_change_basal_profile_pattern = history_entry_static<0x08, 152>;
		using hist_change_basal_profile = history_entry_static<0x09, 152>;
		using hist_cal_bg_for_ph = history_entry_static<0x0A>;
		using hist_alarm_sensor = history_entry_static<0x0B, 8, 3>;
		using hist_clear_alarm = history_entry_static<0x0C>;
		using hist_select_basal_profile = history_entry_static<0x14>;
		using hist_temp_basal_duration = history_entry_static<0x16>;
		using hist_change_time = history_entry_static<0x17, 14, 9>;
		using hist_pump_low_battery = history_entry_static<0x19>;
		using hist_battery = history_entry_static<0x1A>;
		using hist_suspend = history_entry_static<0x1E>;
		using hist_resume = history_entry_static<0x1F>;
		using hist_rewind = history_entry_static<0x21>;
		using hist_change_child_block_enable = history_entry_static<0x23>;
		using hist_change_max_bolus = history_entry_static<0x24>;
		using hist_enable_disable_remote = history_entry_static<0x26, 21>;
		using hist_change_max_basal = history_entry_static<0x2C>;
		using hist_change_bg_reminder_offset = history_entry_static<0x31>;
		using hist_change_alarm_clock_time = history_entry_static<0x32, 14>;
		using hist_temp_basal = history_entry_static<0x33, 8>;
		using hist_pump_low_reservoir = history_entry_static<0x34>;
		using hist_alarm_clock_reminder = history_entry_static<0x35>;
		using hist_questionable_3b = history_entry_static<0x3B>;
		using hist_change_paradigm_linkid = history_entry_static<0x3C, 21>;
		using hist_bg_received = history_entry_static<0x3F, 10>;
		using hist_meal_marker = history_entry_static<0x40, 9>;
		using hist_exercise_marker = history_entry_static<0x41, 8>;
		using hist_manual_insulin_marker = history_entry_static<0x42, 8>;
		using hist_other_marker = history_entry_static<0x43, 7>;
		using hist_change_sensor_rate_of_change_alert_setup = history_entry_static<0x56, 12>;
		using hist_change_bolus_scroll_step_size = history_entry_static<0x57>;

		struct hist_change_sensor_setup: public history_entry<0x50> {
			hist_change_sensor_setup( data_source_t data, pump_model_t pump_model );
			virtual ~hist_change_sensor_setup( );
		};	// hist_change_sensor_setup

		struct hist_change_bolus_wizard_setup: public history_entry<0x5A> {
			hist_change_bolus_wizard_setup( data_source_t data, pump_model_t pump_model );
			virtual ~hist_change_bolus_wizard_setup( );
		};	// hist_change_bolus_wizard_setup

		struct hist_change_bolus_wizard_estimate: public history_entry<0x5B> {
			hist_change_bolus_wizard_estimate( data_source_t data, pump_model_t pump_model );
			virtual ~hist_change_bolus_wizard_estimate( );
		};	// hist_change_bolus_wizard_estimate

		struct hist_unabsorbed_insulin: public history_entry<0x5C> {
			hist_unabsorbed_insulin( data_source_t data, pump_model_t pump_model );
			virtual ~hist_unabsorbed_insulin( );
		};	// hist_unabsorbed_insulin

		using hist_change_variable_bolus = history_entry_static<0x5E>;
		using hist_change_audio_bolus = history_entry_static<0x5F>;
		using hist_change_bg_reminder_enable = history_entry_static<0x60>;
		using hist_change_alarm_clock_enable = history_entry_static<0x61>;
		using hist_change_temp_basal_type = history_entry_static<0x62>;
		using hist_change_alarm_notify_mode = history_entry_static<0x63>;
		using hist_change_time_format = history_entry_static<0x64>;
		using hist_change_reservoir_warning_time = history_entry_static<0x65>;
		using hist_change_bolus_reminder_enable = history_entry_static<0x66>;
		using hist_change_bolus_reminder_time = history_entry_static<0x67>;
		using hist_delete_bolus_reminder_time = history_entry_static<0x68, 9>;
		using hist_delete_alarm_clock_time = history_entry_static<0x6A, 14>;
		using hist_model_522_result_totals = history_entry_static<0x6D, 44, 1, 2>;
		using hist_sara_6e = history_entry_static<0x6E, 52, 1, 2>;
		using hist_change_carb_units = history_entry_static<0x6F>;
		using hist_basal_profile_start = history_entry_static<0x7B, 10>;
		using hist_change_watch_dog_enable = history_entry_static<0x7C>;
		using hist_change_other_device_id = history_entry_static<0x7D, 37>;
		using hist_change_watch_dog_marriage_profile = history_entry_static<0x81, 12>;
		using hist_delete_other_device_id = history_entry_static<0x82, 12>;
		using hist_change_capture_event_enable = history_entry_static<0x83>;


		std::unique_ptr<history_entry_obj> create_history_entry( data_source_t & data, pump_model_t pump_model, size_t & position );
		std::ostream & operator<<( std::ostream & os, timestamp_t::time_t const & t );
		std::ostream & operator<<( std::ostream & os, timestamp_t const & ts );
		std::ostream & operator<<( std::ostream & os, history_entry_obj const & entry );
	}	// namespace history
}	// namespace daw

