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

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/optional.hpp>
#include <daw/daw_range.h>
#include <sstream>
#include <tuple>
#include <daw/json/daw_json.h>
#include <daw/json/daw_json_link.h>
#include "history_pages.h"

namespace daw {
	namespace history {
			namespace {
			
			uint32_t seconds_from_gmt( ) {
#ifdef WIN32
#pragma message ("Warning: GMT Offset set to 0 seconds")
				return 0;
#else
				static auto const result = []( ) { 
					time_t t = time( nullptr );
					struct tm lt = { };
					localtime_r( &t, &lt );
					
					return lt.tm_gmtoff;
				}( );
				return static_cast<uint32_t>(result);
#endif
			}

		}	// namespace anonymous
		std::string op_string( uint8_t op_code ) {
			switch( op_code ) {
				case 0x00: return "skip";
				case 0x01: return "BolusNormal";
				case 0x03: return "Prime";
				case 0x06: return "AlarmPump";
				case 0x07: return "ResultDailyTotal";
				case 0x08: return "ChangeBasalProfilePattern";
				case 0x09: return "ChangeBasalProfile";
				case 0x0A: return "CalBGForPH";
				case 0x0B: return "AlarmSensor"; 
				case 0x0C: return "ClearAlarm";
				case 0x14: return "SelectBasalProfile";
				case 0x16: return "TempBasal";	// hist_temp_basal_duration
				case 0x17: return "ChangeTime";
				case 0x19: return "JournalEntryPumpLowBattery";
				case 0x1A: return "Battery";
				case 0x1E: return "Suspend";
				case 0x1F: return "Resume";
				case 0x21: return "Rewind";
				case 0x23: return "ChangeChildBlockEnable";
				case 0x24: return "ChangeMaxBolus";
				case 0x26: return "EnableDisableRemote";
				case 0x2C: return "ChangeMaxBasal";
				case 0x31: return "ChangeBGReminderOffset";
				case 0x32: return "ChangeAlarmClockTime";
				case 0x33: return "TempBasal";
				case 0x34: return "JournalEntryPumpLowReservoir";
				case 0x35: return "AlarmClockReminder";
				case 0x3B: return "Questionable3b";
				case 0x3C: return "ChangeParadigmLinkID";
				case 0x3F: return "BGReceivedPumpEvent";
				case 0x40: return "JournalEntryMealMarker";
				case 0x41: return "JournalEntryExerciseMarker";
				case 0x42: return "manual_insulin_marker";
				case 0x43: return "other_marker";
				case 0x50: return "ChangeSensorSetup2";
				case 0x56: return "ChangeSensorRateOfChangeAlertSetup";
				case 0x57: return "ChangeBolusScrollStepSize";
				case 0x5A: return "ChangeBolusWizardSetup";
				case 0x5B: return "BolusWizardBolusEstimate";
				case 0x5C: return "UnabsorbedInsulin";
				case 0x5E: return "ChangeVariableBolus";
				case 0x5F: return "ChangeAudioBolus";
				case 0x60: return "ChangeBGReminderEnable";
				case 0x61: return "ChangeAlarmClockEnable";
				case 0x62: return "TempBasal";	// hist_change_temp_basal_type
				case 0x63: return "ChangeAlarmNotifyMode";
				case 0x64: return "ChangeTimeFormat";
				case 0x65: return "ChangeReservoirWarningTime";
				case 0x66: return "ChangeBolusReminderEnable";
				case 0x67: return "ChangeBolusReminderTime";
				case 0x68: return "DeleteBolusReminderTime";
				case 0x6A: return "DeleteAlarmClockTime";
				case 0x6D: return "Model522ResultTotals";
				case 0x6E: return "Sara6E";
				case 0x6F: return "ChangeCarbUnits";
				case 0x7B: return "BasalProfileStart";
				case 0x7C: return "ChangeWatchdogEnable";
				case 0x7D: return "ChangeOtherDeviceID";
				case 0x81: return "ChangeWatchdogMarriageProfile";
				case 0x82: return "DeleteOtherDeviceID";
				case 0x83: return "ChangeCaptureEventEnable";
				default: return "unknown";
			}
		}

		namespace {
			template<typename Container>
			boost::optional<boost::posix_time::ptime> parse_timestamp( Container const & arry ) noexcept {
				if( arry.size( ) < 5 ) {
					return boost::optional<boost::posix_time::ptime>{ };
				}
				uint8_t  second = arry[0] & 0b00111111;
				uint8_t  minute = arry[1] & 0b00111111;
				uint8_t  hour = arry[2] & 0b00011111;
				uint8_t  day = arry[3] & 0b00011111;
				uint8_t  month = ((arry[0] >> 4) & 0b00001100) + (arry[1] >> 6);
				uint16_t  year = 2000 + (arry[4] & 0b01111111);
				if( day < 1 || day > 31 || month < 1 || month > 12 || hour > 24 || minute > 59 || second > 60 ) {
					std::cerr << "WARNING: Could not parse timestamp year=" << static_cast<int>(year) << " month=" << static_cast<int>(month)
							<< " day=" << static_cast<int>(day) << " hour=" << static_cast<int>(hour) << " minute=" << static_cast<int>(minute)
							<< " second=" << static_cast<int>(second) << "\n";
					return boost::optional<boost::posix_time::ptime>{ };
				}
				try {
					using namespace boost::posix_time;
					using namespace boost::gregorian;
					ptime result { date { year, month, day }, time_duration { hour, minute, second } };
					result = result - seconds( seconds_from_gmt( ) ); 
					return result;
				} catch( ... ) {
					std::cerr << "WARNING: Could not parse timestamp year=" << static_cast<int>(year) << " month=" << static_cast<int>(month)
							<< " day=" << static_cast<int>(day) << " hour=" << static_cast<int>(hour) << " minute=" << static_cast<int>(minute)
							<< " second=" << static_cast<int>(second) << "\n";
					return boost::optional<boost::posix_time::ptime>{ };
				}
			}

			template<typename Container>
			boost::optional<boost::posix_time::ptime> parse_date( Container const & arry ) noexcept {
				if( arry.size( ) < 2 ) {
					return boost::optional<boost::posix_time::ptime>{ };
				}
				auto c1 = arry[0];
				auto c2 = arry[1];
				uint8_t day = c1 & 0b00011111;
				uint8_t month = ((c1 & 0b11100000) >> 4) + ((c2 & 0b10000000) >> 7); 
				uint16_t year = 2000 + (c2 & 0b01111111); 
				using namespace boost::posix_time;
				using namespace boost::gregorian;
				if( day < 1 || day > 31 || month < 1 || month > 12 ) {
					std::cerr << "WARNING: Could not parse date year=" << static_cast<int>(year) << " month=" << static_cast<int>(month)
							<< " day=" << static_cast<int>(day) << "\n";
					return boost::optional<boost::posix_time::ptime>{ };
				}
				try {
					using namespace boost::posix_time;
					using namespace boost::gregorian;
					ptime result { date { year, month, day } };
					return result;
				} catch( ... ) {
					std::cerr << "WARNING: Could not parse date year=" << static_cast<int>(year) << " month=" << static_cast<int>(month)
							<< " day=" << static_cast<int>(day) << "\n";
					return boost::optional<boost::posix_time::ptime>{ };
				}
			}

			template<typename Container>
			boost::optional<boost::posix_time::ptime> parse_timestamp_in_array( Container const & data, size_t ts_offset, size_t ts_size ) noexcept {
				boost::optional<boost::posix_time::ptime> result{ };	
				switch( ts_size ) {
				case 2:
					result = parse_date( data.slice( ts_offset ) );
					break;
				case 5:
					result = parse_timestamp( data.slice( ts_offset ) );
					break;
				}
				return result;
			}

			template<typename To, typename From>
			To convert_to( From const & value ) {
				std::stringstream ss;
				ss << value;
				To result;
				ss >> result;
				return result;
			}

			template<typename T, typename U, typename R=T>
			R max( T lhs, U rhs ) {
				return lhs > rhs ? static_cast<R>(lhs) : static_cast<R>(rhs);
			}

		}	// namespace anonymous

		pump_model_t::pump_model_t( std::string const & model ):
			generation( convert_to<uint16_t>( model ) % 100u ),
			larger { generation >= 23 },
			has_low_suspend { generation >= 51 },
			strokes_per_unit( generation >= 23 ? 40 : 10 ) { }

		pump_model_t::~pump_model_t( ) { }

		history_entry_obj::history_entry_obj( data_source_t data, bool is_decoded, size_t data_size, pump_model_t, size_t timestamp_offset, size_t timestamp_size ):
			JsonLink<history_entry_obj>( op_string( data[0] ) ),
			m_op_code { data[0] },
			m_size { data_size }, 
			m_timestamp_offset { timestamp_offset },
			m_timestamp_size { timestamp_size },
			m_data { data.shrink( data_size ).as_vector( ) },
			m_timestamp{ parse_timestamp_in_array( data, m_timestamp_offset, m_timestamp_size ) } {
				
				link_integral( "op_code", m_op_code );
				if( !is_decoded ) {
					link_integral( "size", m_size );
					link_integral( "timestamp_offset", m_timestamp_offset );
					link_integral( "timestamp_size", m_timestamp_size );
					link_array( "rawData", m_data );
				}
				link_timestamp( "timestamp", m_timestamp );

			}

		history_entry_obj::~history_entry_obj( ) { };

		std::tuple<uint8_t, size_t, size_t, size_t> history_entry_obj::register_event_type( ) const {
			return std::make_tuple( this->op_code( ), this->size( ), this->timestamp_offset( ), this->timestamp_size( ) );
		}

		boost::optional<boost::posix_time::ptime> history_entry_obj::timestamp( ) const {
			return m_timestamp;
		}

		uint8_t history_entry_obj::op_code( ) const {
			return this->m_op_code;
		}

		std::vector<uint8_t> const & history_entry_obj::data( ) const {
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

		hist_result_daily_total::~hist_result_daily_total( ) { }
		hist_change_sensor_setup::~hist_change_sensor_setup( ) { }
		hist_change_bolus_wizard_setup::~hist_change_bolus_wizard_setup( ) { }
		hist_bolus_wizard_estimate::~hist_bolus_wizard_estimate( ) { }
		
		namespace {
			template<typename T, typename Container>
			T bigendian_to_native_from_bytes( Container const & c, size_t sz = sizeof( T ) ) {
				assert( sz <= sizeof( T ) );
				assert( c.size( ) >= sz );
				T result = 0;
				unsigned char * p = reinterpret_cast<unsigned char *>(&result);
				std::copy( c.begin( ), c.begin( ) + sz, p );
				boost::endian::big_to_native_inplace( result );
				return result;
			}

			template<typename Container>
			double decode_insulin_from_bytes( Container const & c, pump_model_t const & pm ) {
				return static_cast<double>(bigendian_to_native_from_bytes<uint16_t>( c, pm.larger ? 2 : 1 ))/static_cast<double>(pm.strokes_per_unit);
			}
		}

		hist_bolus_normal::hist_bolus_normal( data_source_t data, pump_model_t pump_model ):
				history_entry<0x01>( std::move( data ), false, pump_model.larger ? 13 : 9, std::move( pump_model ), pump_model.larger ? 8 : 4 ),
				m_amount{ decode_insulin_from_bytes( data.slice( 3 ), pump_model ) }, 
				m_programmed{ decode_insulin_from_bytes( data.slice( 1 ), pump_model ) },
				m_unabsorbed_insulin_total{ pump_model.larger ? decode_insulin_from_bytes( data.slice( 5 ), pump_model ) : 0 },
				m_duration( static_cast<uint16_t>(data[pump_model.larger ? 7 : 3])*30 ) {

			link_real( "amount", m_amount );
			link_real( "programmed", m_programmed );
			link_real( "unabsorbed", m_unabsorbed_insulin_total );
			link_integral( "duration", m_duration );
		}

		hist_bolus_normal::~hist_bolus_normal( ) { }

		hist_prime::hist_prime( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x03, true, 10, 5>{ std::move( data ), std::move( pump_model ) },
				m_amount{ static_cast<double>(static_cast<uint16_t>(data[4]) << 2)/40.0 },
				m_prime_type{ (static_cast<uint16_t>(data[2]) << 2) == 0 ? "manual": "fixed" },
				m_programmed_amount{ static_cast<double>(static_cast<uint16_t>(data[2]) << 2)/40.0 } {

			link_real( "amount", m_amount );
			link_string( "primeType", m_prime_type );
			link_real( "programmedAmount", m_programmed_amount );
		}
		
		hist_prime::~hist_prime( ) { }

		hist_alarm_pump::hist_alarm_pump( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x06, false, 9, 4>{ std::move( data ), std::move( pump_model ) },
				m_raw_type{ data[1] } {

			link_integral( "rawType", m_raw_type );
		}

		hist_alarm_pump::~hist_alarm_pump( ) { }

		hist_result_daily_total::hist_result_daily_total( data_source_t data, pump_model_t pump_model ):
				history_entry<0x07>( std::move( data ), false, pump_model.larger ? 10 : 7, std::move( pump_model ), 5, 2 ) {

		}

		hist_bg_received::hist_bg_received( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x3F, true, 10>{ std::move( data ), std::move( pump_model ) },
				m_amount{ static_cast<uint8_t>((data[1] << 3) | (data[4] >> 5)) },
				m_meter{ data.slice( 7, 10 ).to_hex_string( ) } {

			link_integral( "amount", m_amount );
			link_string( "meter", m_meter );
		}

		hist_bg_received::~hist_bg_received( ) { }

		namespace {
			bool use_carb_exchange( uint8_t d8 ) {
				return impl::read_bit( d8, 2 );
			}

			double calc_meal_marker_carb( uint8_t d7, uint8_t d8 ) {

				if( use_carb_exchange( d8 ) ) {
					return static_cast<double>( d7 );
				} else {
					return static_cast<double>(static_cast<uint16_t>(impl::read_bit( d8, 1 ) << 8) | static_cast<uint8_t>( d7 ));
				}

			}

		}	// namespace anonymous
		
		hist_meal_marker::hist_meal_marker( data_source_t data, pump_model_t pump_model ): 
				history_entry_static<0x40, true, 9>{ std::move( data ), std::move( pump_model ) },
				m_carbohydrates{ calc_meal_marker_carb( data[7], data[8] ) },
				m_carb_units{ use_carb_exchange( data[8] ) ? "Exchanges" : "Grams" } {

			link_real( "carbohydrates", m_carbohydrates );
			link_string( "carbUnits", m_carb_units );
		}

		hist_meal_marker::~hist_meal_marker( ) { }



		hist_cal_bg_for_ph::hist_cal_bg_for_ph( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x0A, true>{ std::move( data ), std::move( pump_model ) },
				m_amount{ static_cast<uint16_t>(((data[6] & 0b10000000) << 1) | data[1]) } {

				link_integral( "amount", m_amount );
		}

		hist_cal_bg_for_ph::~hist_cal_bg_for_ph( ) { }
		
		hist_select_basal_profile::hist_select_basal_profile( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x14, false>{ std::move( data ), std::move( pump_model ) },
				m_basal_profile_index{ data[1] } {

			link_integral( "BasalProfileIndex", m_basal_profile_index );
		}

		hist_select_basal_profile::~hist_select_basal_profile( ) { }

		hist_temp_basal_duration::hist_temp_basal_duration( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x16, true>{ std::move( data ), std::move( pump_model ) },
				m_duration_minutes{ static_cast<uint16_t>(static_cast<uint16_t>(data[1]) * 30) } {
			
			link_integral( "duration", m_duration_minutes );
		}

		hist_temp_basal_duration::~hist_temp_basal_duration( ) { }

		hist_change_time::hist_change_time( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x17, false, 14, 9>{ std::move( data ), std::move( pump_model ) },
				m_old_timestamp{ *parse_timestamp( data.slice( 2 ) ) } {

			link_timestamp( "oldTimeStamp", m_old_timestamp );
		}

		hist_change_time::~hist_change_time( ) { }



		hist_temp_basal::hist_temp_basal( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x33, true, 8>{ std::move( data ), std::move( pump_model ) },
				m_rate_type{ (data[7] >> 3) == 0 ? "absolute" : "percent" },
				m_rate{ (data[7] >> 3) == 0 ? static_cast<double>(data[1])/40.0 : static_cast<double>(data[1]) } {
				
			link_string( "rateType", m_rate_type );
			link_real( "rate", m_rate );
		}

		hist_temp_basal::~hist_temp_basal( ) { }

		hist_basal_profile_start::hist_basal_profile_start( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x7B, true, 10>{ std::move( data ), std::move( pump_model ) },
				m_rate{ static_cast<double>(data[8])/40.0 },
				m_offset{ static_cast<uint32_t>(data[7]) * 30 * 1000 * 60 },
				m_profile_index{ data[1] } {

			link_real( "rate", m_rate );
			link_integral( "offset", m_offset );
			link_integral( "profileIndex", m_profile_index );
		}

		hist_basal_profile_start::~hist_basal_profile_start( ) { }

		hist_change_watch_dog_marriage_profile::hist_change_watch_dog_marriage_profile( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x81, false, 12>{ std::move( data ), std::move( pump_model ) },
				m_other_device_id( data.slice( 8, 12 ).to_hex_string( ) ) {

			link_string( "otherDeviceID", m_other_device_id );
		}

		hist_change_watch_dog_marriage_profile::~hist_change_watch_dog_marriage_profile( ) { }


		hist_change_sensor_setup::hist_change_sensor_setup( data_source_t data, pump_model_t pump_model ):
			history_entry<0x50>( std::move( data ), false, pump_model.has_low_suspend ? 41 : 37, std::move( pump_model ) ) { }

		hist_change_bolus_wizard_setup::hist_change_bolus_wizard_setup( data_source_t data, pump_model_t pump_model ):
			history_entry<0x5A>( std::move( data ), false, pump_model.larger ? 144 : 124, std::move( pump_model ) ) { }

		namespace {
			auto bolus_wizard_insulin_decoder( uint8_t a, uint8_t b ) {
				return static_cast<double>((static_cast<uint16_t>(a) << static_cast<uint16_t>(8)) | static_cast<uint16_t>(b)) / 40.0;
			}

			auto bolus_wizard_correction_decoder_lrg( uint8_t a, uint8_t b ) {
				return static_cast<double>((static_cast<uint16_t>(a & 0b00111000) << static_cast<uint16_t>(5)) | static_cast<uint16_t>(b)) / 40.0;
			}

			auto bolus_wizard_correction_decoder( uint8_t a, uint8_t b ) {
				return static_cast<double>((static_cast<uint16_t>(a) << static_cast<uint16_t>(8)) | static_cast<uint16_t>(b)) / 10.0;
			}

			auto bolus_wizard_insulin_decoder( uint8_t a ) {
				return static_cast<double>(a)/10.0;	
			}

			auto bolus_wizard_carb_ratio_decoder( uint8_t a, uint8_t b ) {
				return static_cast<double>((static_cast<uint16_t>(a & 0b00000111) << static_cast<uint16_t>(8)) | static_cast<uint16_t>(b)) / 10.0;
			}

		}
	
		hist_bolus_wizard_estimate::hist_bolus_wizard_estimate( data_source_t data, pump_model_t pump_model ):
				history_entry<0x5B> { std::move( data ), true, static_cast<size_t>(pump_model.larger ? 22 : 20), pump_model },
				m_carbohydrates{ pump_model.larger ? static_cast<uint16_t>((static_cast<uint16_t>(data[8] & 0b00001100) << static_cast<uint16_t>(6)) | static_cast<uint16_t>(data[7])) : static_cast<uint16_t>(data[7]) },
				m_blood_glucose{ static_cast<uint16_t>(static_cast<uint16_t>(static_cast<uint16_t>(data[8] & 0b00000011) << 8) | static_cast<uint16_t>(data[1])) },
				m_insulin_food_estimate{ pump_model.larger ? bolus_wizard_insulin_decoder( data[14], data[15] ) : bolus_wizard_insulin_decoder( data[13] ) },
				m_insulin_correction_estimate{ pump_model.larger ? bolus_wizard_correction_decoder_lrg( data[16], data[13] ) : bolus_wizard_correction_decoder( data[14], data[12] ) } ,
				m_insulin_bolus_estimate{ pump_model.larger ? bolus_wizard_insulin_decoder( data[19], data[20] ) : bolus_wizard_insulin_decoder( data[18] ) },
				m_unabsorbed_insulin_total{ pump_model.larger ? bolus_wizard_insulin_decoder( data[17], data[18] ) : bolus_wizard_insulin_decoder( data[16] ) },
				m_bg_target_low{ pump_model.larger ? data[12] : data[11] },
				m_bg_target_high{ pump_model.larger ? data[21] : data[19] },
				m_insulin_sensitivity{ pump_model.larger ? data[11] : data[10] },
				m_carbohydrate_ratio{ pump_model.larger ? bolus_wizard_carb_ratio_decoder( data[9], data[10] ) : static_cast<double>(data[9]) } {

			link_integral( "carbInput", m_carbohydrates );
			link_integral( "bg", m_blood_glucose );
			link_real( "foodEstimate", m_insulin_food_estimate );
			link_real( "correctionEstimate", m_insulin_correction_estimate );
			link_real( "bolusEstimate", m_insulin_bolus_estimate );
			link_real( "unabsorbedInsulinTotal", m_unabsorbed_insulin_total );
			link_integral( "bgTargetLow", m_bg_target_low );
			link_integral( "bgTargetHigh", m_bg_target_high );
			link_real( "carbRatio", m_carbohydrate_ratio );
		}

		hist_unabsorbed_insulin::unabsorbed_insulin_record_t::~unabsorbed_insulin_record_t( ) { }
		hist_unabsorbed_insulin::~hist_unabsorbed_insulin( ) { }

		hist_unabsorbed_insulin::unabsorbed_insulin_record_t::unabsorbed_insulin_record_t( double amount, uint32_t age ):
			daw::json::JsonLink<hist_unabsorbed_insulin::unabsorbed_insulin_record_t>{ },
			m_amount{ amount },
			m_age{ age } {

			link_real( "amount", m_amount );
			link_integral( "age", m_age );
		}	

		hist_unabsorbed_insulin::hist_unabsorbed_insulin( data_source_t data, pump_model_t pump_model ):
			history_entry<0x5C>{ std::move( data ), true, max( data[1], 2 ), std::move( pump_model ), 1, 0 },
			m_records{ } {
				
				{
					uint8_t num_records = data[1]; 
					if( num_records >= 5 ) {
						num_records = (num_records - 2)/2;
						for( uint8_t n = 0; n<num_records; ++n ) {
							m_records.emplace_back( static_cast<double>(data[2+(n*3)])/40.0, data[3+(n*3)] + ((data[4+(n*3)] & 0b110000) << 4) );
						}
					}
				}
				link_array( "records", m_records );	
		}

		hist_change_temp_basal_type::~hist_change_temp_basal_type( ) { }

		hist_change_temp_basal_type::hist_change_temp_basal_type( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x62, true>{ std::move( data ), std::move( pump_model ) },
				m_basal_type{ data[1] == 1 ? "percent" : "absolute" } {
	
			link_string( "basalType", m_basal_type );
		}

		hist_change_time_format::~hist_change_time_format( ) { }
		hist_change_time_format::hist_change_time_format( data_source_t data, pump_model_t pump_model ):
				history_entry_static<0x64, true>{ std::move( data ), std::move( pump_model ) },
				m_time_format{ data[1] == 1 ? "24hr" : "am_pm" } {

			link_string( "timeFormat", m_time_format );
		}
			
		namespace {
			template<typename... Args>
			std::unique_ptr<history_entry_obj> create_history_entry_impl( uint8_t op_code, Args&&... arg ) {
				return  std::unique_ptr<history_entry_obj>( [op_code]( Args&&... args ) -> history_entry_obj* {
					switch( op_code ) {
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
							case 0x5B: return new hist_bolus_wizard_estimate( std::forward<Args>( args )... );
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
			data.advance( static_cast<data_source_t::difference_type>(result->size( )) );
			return result;
		}
	}	// namespace history
}	// namespace daw

