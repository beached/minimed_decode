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

			boost::optional<timestamp_t> timestamp( ) const {
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

			daw::range::Range & size( ) {
				return m_size;
			}	

			daw::range::Range const & size( ) const {
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

    case ChangeTime = 0x17
    case JournalEntryPumpLowBattery = 0x19
    case Battery = 0x1a
    case Suspend = 0x1e
    case Resume = 0x1f
    case Rewind = 0x21
    case ChangeChildBlockEnable = 0x23
    case ChangeMaxBolus = 0x24
    case EnableDisableRemote = 0x26
    case ChangeMaxBasal = 0x2c
    case ChangeBGReminderOffset = 0x31
    case ChangeAlarmClockTime = 0x32
    case TempBasal = 0x33
    case JournalEntryPumpLowReservoir = 0x34
    case AlarmClockReminder = 0x35
    case Questionable3b = 0x3b
    case ChangeParadigmLinkID = 0x3c
    case BGReceived = 0x3f
    case JournalEntryExerciseMarker = 0x41
    case ChangeSensorSetup2 = 0x50
    case ChangeSensorRateOfChangeAlertSetup = 0x56
    case ChangeBolusScrollStepSize = 0x57
    case ChangeBolusWizardSetup = 0x5a
    case BolusWizardBolusEstimate = 0x5b
    case UnabsorbedInsulin = 0x5c
    case ChangeVariableBolus = 0x5e
    case ChangeAudioBolus = 0x5f
    case ChangeBGReminderEnable = 0x60
    case ChangeAlarmClockEnable = 0x61
    case ChangeTempBasalType = 0x62
    case ChangeAlarmNotifyMode = 0x63
    case ChangeTimeFormat = 0x64
    case ChangeReservoirWarningTime = 0x65
    case ChangeBolusReminderEnable = 0x66
    case ChangeBolusReminderTime = 0x67
    case DeleteBolusReminderTime = 0x68
    case DeleteAlarmClockTime = 0x6a
    case Model522ResultTotals = 0x6d
    case Sara6E = 0x6e
    case ChangeCarbUnits = 0x6f
    case BasalProfileStart = 0x7b
    case ChangeWatchdogEnable = 0x7c
    case ChangeOtherDeviceID = 0x7d
    case ChangeWatchdogMarriageProfile = 0x81
    case DeleteOtherDeviceID = 0x82
    case ChangeCaptureEventEnable = 0x83
	}	// namespace history
}	// namespace daw

