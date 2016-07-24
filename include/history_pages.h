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
#include "history_pages_base.h"
#include <boost/endian/conversion.hpp>

namespace daw {
	namespace history {
		// Known History Entries in order of op_code

		using hist_skip = history_entry_static<0x00, true, 1, 0, 0>;

		enum class bolus_type_t: uint8_t {
			normal,
			square,
			dual_wave
		};

		struct hist_bolus_normal: public history_entry<0x01> {
			double m_amount;
			double m_programmed;
			double m_unabsorbed_insulin_total;
			uint16_t m_duration;
			bolus_type_t bolus_type;	 

			hist_bolus_normal( data_source_t data, pump_model_t pump_model );
			virtual ~hist_bolus_normal( );
			hist_bolus_normal( hist_bolus_normal const & ) = default;
			hist_bolus_normal( hist_bolus_normal && ) = default;
			hist_bolus_normal & operator=( hist_bolus_normal const & ) = default;
			hist_bolus_normal & operator=( hist_bolus_normal && ) = default;

		};	// hist_bolus_normal

		struct hist_prime: public history_entry_static<0x03, true, 10, 5> {
			double m_amount;
			std::string m_prime_type;
			double m_programmed_amount;

			hist_prime( data_source_t data, pump_model_t pump_model );

			virtual ~hist_prime( );
			hist_prime( hist_prime const & ) = default;
			hist_prime( hist_prime && ) = default;
			hist_prime & operator=( hist_prime const & ) = default;
			hist_prime & operator=( hist_prime && ) = default;
		};	// hist_prime

		struct hist_alarm_pump: public history_entry_static<0x06, false, 9, 4> {
			uint8_t m_raw_type;

			hist_alarm_pump( data_source_t data, pump_model_t pump_model );

			virtual ~hist_alarm_pump( );
			hist_alarm_pump( hist_alarm_pump const & ) = default;
			hist_alarm_pump( hist_alarm_pump && ) = default;
			hist_alarm_pump & operator=( hist_alarm_pump const & ) = default;
			hist_alarm_pump & operator=( hist_alarm_pump && ) = default;
		};	// hist_alarm_pump


		struct hist_result_daily_total: public history_entry<0x07> {
			hist_result_daily_total( data_source_t data, pump_model_t pump_model );

			virtual ~hist_result_daily_total( );
			hist_result_daily_total( hist_result_daily_total const & ) = default;
			hist_result_daily_total( hist_result_daily_total && ) = default;
			hist_result_daily_total & operator=( hist_result_daily_total const & ) = default;
			hist_result_daily_total & operator=( hist_result_daily_total && ) = default;
		};	// hist_result_daily_total

		using hist_change_basal_profile_pattern = history_entry_static<0x08, false, 152>;
		using hist_change_basal_profile = history_entry_static<0x09, false, 152>;

		struct hist_cal_bg_for_ph: public history_entry_static<0x0A, true> {
			uint16_t m_amount;
			hist_cal_bg_for_ph( data_source_t data, pump_model_t pump_model );

			virtual ~hist_cal_bg_for_ph( );
			hist_cal_bg_for_ph( hist_cal_bg_for_ph const & ) = default;
			hist_cal_bg_for_ph( hist_cal_bg_for_ph && ) = default;
			hist_cal_bg_for_ph & operator=( hist_cal_bg_for_ph const & ) = default;
			hist_cal_bg_for_ph & operator=( hist_cal_bg_for_ph && ) = default;
		};	// hist_cal_bg_for_ph

		using hist_alarm_sensor = history_entry_static<0x0B, false, 8, 3>;
		using hist_clear_alarm = history_entry_static<0x0C>;

		// Test this, but I see what looks like the idx changes I use
		struct hist_select_basal_profile: public history_entry_static<0x14, false> {
			uint8_t m_basal_profile_index;

			hist_select_basal_profile( data_source_t data, pump_model_t pump_model );

			virtual ~hist_select_basal_profile( );
			hist_select_basal_profile( hist_select_basal_profile const & ) = default;
			hist_select_basal_profile( hist_select_basal_profile && ) = default;
			hist_select_basal_profile & operator=( hist_select_basal_profile const & ) = default;
			hist_select_basal_profile & operator=( hist_select_basal_profile && ) = default;
		};	// hist_select_basal_profile

		struct hist_temp_basal_duration: public history_entry_static<0x16, true> {
			uint16_t m_duration_minutes;

			hist_temp_basal_duration( data_source_t data, pump_model_t pump_model );

			virtual ~hist_temp_basal_duration( );
			hist_temp_basal_duration( hist_temp_basal_duration const & ) = default;
			hist_temp_basal_duration( hist_temp_basal_duration && ) = default;
			hist_temp_basal_duration & operator=( hist_temp_basal_duration const & ) = default;
			hist_temp_basal_duration & operator=( hist_temp_basal_duration && ) = default;
		};	// hist_temp_basal_duration

		struct hist_change_time: public history_entry_static<0x17, false, 14, 9> {
			boost::posix_time::ptime m_old_timestamp;

			hist_change_time( data_source_t data, pump_model_t pump_model );

			virtual ~hist_change_time( );
			hist_change_time( hist_change_time const & ) = default;
			hist_change_time( hist_change_time && ) = default;
			hist_change_time & operator=( hist_change_time const & ) = default;
			hist_change_time & operator=( hist_change_time && ) = default;
		};	// hist_change_time


		using hist_pump_low_battery = history_entry_static<0x19>;
		using hist_battery = history_entry_static<0x1A>;
		using hist_suspend = history_entry_static<0x1E>;
		using hist_resume = history_entry_static<0x1F>;
		using hist_rewind = history_entry_static<0x21>;
		using hist_change_child_block_enable = history_entry_static<0x23>;
		using hist_change_max_bolus = history_entry_static<0x24>;
		using hist_enable_disable_remote = history_entry_static<0x26, false, 21>;
		using hist_change_max_basal = history_entry_static<0x2C>;
		using hist_change_bg_reminder_offset = history_entry_static<0x31>;
		using hist_change_alarm_clock_time = history_entry_static<0x32, false, 14>;

		struct hist_temp_basal: public history_entry_static<0x33, true, 8> {
			std::string m_rate_type;
			double m_rate;
			
			hist_temp_basal( data_source_t data, pump_model_t pump_model );

			virtual ~hist_temp_basal( );
			hist_temp_basal( hist_temp_basal const & ) = default;
			hist_temp_basal( hist_temp_basal && ) = default;
			hist_temp_basal & operator=( hist_temp_basal const & ) = default;
			hist_temp_basal & operator=( hist_temp_basal && ) = default;
		};	// hist_temp_basal
	
		using hist_pump_low_reservoir = history_entry_static<0x34>;
		using hist_alarm_clock_reminder = history_entry_static<0x35>;
		using hist_questionable_3b = history_entry_static<0x3B>;
		using hist_change_paradigm_linkid = history_entry_static<0x3C, false, 21>;

		struct hist_bg_received: public history_entry_static<0x3F, true, 10> {
			uint16_t m_amount;
			std::string m_meter;
			hist_bg_received( data_source_t data, pump_model_t pump_model );

			virtual ~hist_bg_received( );
			hist_bg_received( hist_bg_received const & ) = default;
			hist_bg_received( hist_bg_received && ) = default;
			hist_bg_received & operator=( hist_bg_received const & ) = default;
			hist_bg_received & operator=( hist_bg_received && ) = default;
		};	// hist_bg_received

		struct hist_meal_marker: public history_entry_static<0x40, true, 9> {
			double m_carbohydrates;
			std::string m_carb_units;

			hist_meal_marker( data_source_t data, pump_model_t pump_model );

			virtual ~hist_meal_marker( );
			hist_meal_marker( hist_meal_marker const & ) = default;
			hist_meal_marker( hist_meal_marker && ) = default;
			hist_meal_marker & operator=( hist_meal_marker const & ) = default;
			hist_meal_marker & operator=( hist_meal_marker && ) = default;
		};	// hist_meal_marker



		using hist_exercise_marker = history_entry_static<0x41, false, 8>;
		using hist_manual_insulin_marker = history_entry_static<0x42, false, 8>;
		using hist_other_marker = history_entry_static<0x43, false, 7>;
		using hist_change_sensor_rate_of_change_alert_setup = history_entry_static<0x56, false, 12>;
		using hist_change_bolus_scroll_step_size = history_entry_static<0x57>;

		struct hist_change_sensor_setup: public history_entry<0x50> {
			hist_change_sensor_setup( data_source_t data, pump_model_t pump_model );
			virtual ~hist_change_sensor_setup( );
		};	// hist_change_sensor_setup

		struct hist_change_bolus_wizard_setup: public history_entry<0x5A> {
			hist_change_bolus_wizard_setup( data_source_t data, pump_model_t pump_model );
			virtual ~hist_change_bolus_wizard_setup( );
		};	// hist_change_bolus_wizard_setup

		struct hist_bolus_wizard_estimate: public history_entry<0x5B> {
			uint16_t m_carbohydrates;
			uint16_t m_blood_glucose;
			double m_insulin_food_estimate;
			double m_insulin_correction_estimate;
			double m_insulin_bolus_estimate;
			double m_unabsorbed_insulin_total;
			uint8_t m_bg_target_low;
			uint8_t m_bg_target_high;
			uint8_t m_insulin_sensitivity;
			double m_carbohydrate_ratio;

			hist_bolus_wizard_estimate( data_source_t data, pump_model_t pump_model );
			virtual ~hist_bolus_wizard_estimate( );
		};	// hist_bolus_wizard_estimate

		struct hist_unabsorbed_insulin: public history_entry<0x5C> {
			struct unabsorbed_insulin_record_t: public daw::json::JsonLink<unabsorbed_insulin_record_t>  {
				double m_amount;
				uint32_t m_age;
				virtual ~unabsorbed_insulin_record_t( );
				unabsorbed_insulin_record_t( double amount = 0.0, uint32_t age = 0 );
				unabsorbed_insulin_record_t( unabsorbed_insulin_record_t const & ) = default;
				unabsorbed_insulin_record_t( unabsorbed_insulin_record_t && ) = default;
				unabsorbed_insulin_record_t & operator=( unabsorbed_insulin_record_t const & ) = default;
				unabsorbed_insulin_record_t & operator=( unabsorbed_insulin_record_t && ) = default;
			};
			std::vector<unabsorbed_insulin_record_t> m_records;
			hist_unabsorbed_insulin( data_source_t data, pump_model_t pump_model );
			virtual ~hist_unabsorbed_insulin( );
			hist_unabsorbed_insulin( hist_unabsorbed_insulin const & ) = default;
			hist_unabsorbed_insulin( hist_unabsorbed_insulin && ) = default;
			hist_unabsorbed_insulin & operator=( hist_unabsorbed_insulin const & ) = default;
			hist_unabsorbed_insulin & operator=( hist_unabsorbed_insulin && ) = default;
		};	// hist_unabsorbed_insulin

		using hist_change_variable_bolus = history_entry_static<0x5E>;
		using hist_change_audio_bolus = history_entry_static<0x5F>;
		using hist_change_bg_reminder_enable = history_entry_static<0x60>;
		using hist_change_alarm_clock_enable = history_entry_static<0x61>;

		struct hist_change_temp_basal_type: public history_entry_static<0x62, true> {
			std::string m_basal_type;

			hist_change_temp_basal_type( data_source_t data, pump_model_t pump_model );

			virtual ~hist_change_temp_basal_type( );
			hist_change_temp_basal_type( hist_change_temp_basal_type const & ) = default;
			hist_change_temp_basal_type( hist_change_temp_basal_type && ) = default;
			hist_change_temp_basal_type & operator=( hist_change_temp_basal_type const & ) = default;
			hist_change_temp_basal_type & operator=( hist_change_temp_basal_type && ) = default;
		};	// hist_change_temp_basal_type


		using hist_change_alarm_notify_mode = history_entry_static<0x63>;

		struct hist_change_time_format: public history_entry_static<0x64, true> {
			std::string m_time_format;

			hist_change_time_format( data_source_t data, pump_model_t pump_model );

			virtual ~hist_change_time_format( );
			hist_change_time_format( hist_change_time_format const & ) = default;
			hist_change_time_format( hist_change_time_format && ) = default;
			hist_change_time_format & operator=( hist_change_time_format const & ) = default;
			hist_change_time_format & operator=( hist_change_time_format && ) = default;
		};	// hist_change_time_format


		using hist_change_reservoir_warning_time = history_entry_static<0x65>;
		using hist_change_bolus_reminder_enable = history_entry_static<0x66>;
		using hist_change_bolus_reminder_time = history_entry_static<0x67>;
		using hist_delete_bolus_reminder_time = history_entry_static<0x68, false, 9>;
		using hist_delete_alarm_clock_time = history_entry_static<0x6A, false, 14>;
		using hist_model_522_result_totals = history_entry_static<0x6D, false, 44, 1, 2>;
		using hist_sara_6e = history_entry_static<0x6E, false, 52, 1, 2>;
		using hist_change_carb_units = history_entry_static<0x6F>;

		struct hist_basal_profile_start: public history_entry_static<0x7B, true, 10> {
			double m_rate;
			uint32_t m_offset;
			uint8_t m_profile_index;

			hist_basal_profile_start( data_source_t data, pump_model_t pump_model );

			virtual ~hist_basal_profile_start( );
			hist_basal_profile_start( hist_basal_profile_start const & ) = default;
			hist_basal_profile_start( hist_basal_profile_start && ) = default;
			hist_basal_profile_start & operator=( hist_basal_profile_start const & ) = default;
			hist_basal_profile_start & operator=( hist_basal_profile_start && ) = default;
		};	// hist_basal_profile_start


		using hist_change_watch_dog_enable = history_entry_static<0x7C>;
		using hist_change_other_device_id = history_entry_static<0x7D, false, 37>;

		struct hist_change_watch_dog_marriage_profile: public history_entry_static<0x81, false, 12> {
			std::string m_other_device_id;

			hist_change_watch_dog_marriage_profile( data_source_t data, pump_model_t pump_model );

			virtual ~hist_change_watch_dog_marriage_profile( );
			hist_change_watch_dog_marriage_profile( hist_change_watch_dog_marriage_profile const & ) = default;
			hist_change_watch_dog_marriage_profile( hist_change_watch_dog_marriage_profile && ) = default;
			hist_change_watch_dog_marriage_profile & operator=( hist_change_watch_dog_marriage_profile const & ) = default;
			hist_change_watch_dog_marriage_profile & operator=( hist_change_watch_dog_marriage_profile && ) = default;
		};	// hist_change_watch_dog_marriage_profile

		using hist_delete_other_device_id = history_entry_static<0x82, false, 12>;
		using hist_change_capture_event_enable = history_entry_static<0x83>;
	}	// namespace history
}	// namespace daw

