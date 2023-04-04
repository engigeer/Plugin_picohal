/*

  picohal.c

  Part of grblHAL
  grblHAL is
  Copyright (c) 2022-2023 Terje Io

  picoHAL design and plugin code are copyright (c) 2023 Expatria Technologies Inc.

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <math.h>
#include <string.h>

#include "picohal.h"

static on_state_change_ptr on_state_change;
static on_report_options_ptr on_report_options;
static on_program_completed_ptr on_program_completed;
static on_execute_realtime_ptr on_execute_realtime; // For real time loop insertion
static user_mcode_ptrs_t user_mcode;
static driver_reset_ptr driver_reset;
static user_mcode_ptrs_t user_mcode;

static uint16_t retry_counter = 0;

static void picohal_rx_packet (modbus_message_t *msg);
static void picohal_rx_exception (uint8_t code, void *context);

static const modbus_callbacks_t callbacks = {
    .on_rx_packet = picohal_rx_packet,
    .on_rx_exception = picohal_rx_exception
};

#define PICOHAL_ADDRESS 10

static void picohal_set_state ()
{
    uint16_t data;

        switch (state_get()){
        case STATE_ALARM:
            data = 1;
            break;
        case STATE_ESTOP:
            data = 1;
            break;            
        case STATE_CYCLE:
            data = 2;
            break;
        case STATE_HOLD:
            data = 3;
            break;
        case STATE_TOOL_CHANGE:
            data = 4;
            break;
        case STATE_IDLE:
            data = 5;
            break;
        case STATE_HOMING:
            data = 6;
            break;   
        case STATE_JOG:
            data = 7;
            break;                                    
        default :
            data = 254;
            break;                                                        
    }

    modbus_message_t rpm_cmd = {
        .context = (void *)picohal_SetState,
        .crc_check = false,
        .adu[0] = PICOHAL_ADDRESS,
        .adu[1] = ModBus_WriteRegister,
        .adu[2] = 0x00,
        .adu[3] = 0x01,
        .adu[4] = data >> 8,
        .adu[5] = data & 0xFF,
        .tx_length = 8,
        .rx_length = 8
    };

    modbus_send(&rpm_cmd, &callbacks, true);
}

static void picohal_write_event (uint16_t current_event)
{
    modbus_message_t rpm_cmd = {
        .context = (void *)picohal_SetEvent,
        .crc_check = false,
        .adu[0] = PICOHAL_ADDRESS,
        .adu[1] = ModBus_WriteRegister,
        .adu[2] = 0x00,
        .adu[3] = 0x05,
        .adu[4] = current_event >> 8,
        .adu[5] = current_event & 0xFF,
        .tx_length = 8,
        .rx_length = 8
    };

    modbus_send(&rpm_cmd, &callbacks, true);
}

static void picohal_rx_packet (modbus_message_t *msg)
{
    if(!(msg->adu[0] & 0x80)) {

        switch((picohal_response_t)msg->context) {

            case picohal_SetState:
            case picohal_SetEvent:
                retry_counter = 0;
                break;

            default:
                retry_counter = 0;
                break;
        }
    }
}

static void picohal_rx_exception (uint8_t code, void *context)
{
    //if RX exceptions during one of the VFD messages, need to retry.
    if ((picohal_response_t)context > 0 ) {
        retry_counter++;
        if (retry_counter >=PICOHAL_RETRIES) {
            report_message("PicoHAL Communications Failure!", Message_Warning);
            retry_counter = 0;
            return;
        }

        switch((picohal_response_t)context) {

            case picohal_SetState:
//                modbus_reset();
                break;

            default:
                break;
        }//close switch statement
    } else {
        retry_counter = 0;
        report_message("PicoHAL comms failure 2!", Message_Warning);
    }
}

static void onReportOptions (bool newopt)
{
    on_report_options(newopt);

    if(!newopt){
        hal.stream.write("[PLUGIN:PICOHAL v0.1]"  ASCII_EOL);
    }
}

static void onStateChanged (sys_state_t state)
{
    picohal_set_state();
    if (on_state_change)         // Call previous function in the chain.
        on_state_change(state);    
}

// ON (Gcode) PROGRAM COMPLETION
static void onProgramCompleted (program_flow_t program_flow, bool check_mode)
{
    //write the program flow value to the event register.
    picohal_write_event((uint16_t) program_flow);
    
    if(on_program_completed)
        on_program_completed(program_flow, check_mode);
}

void picohal_init (void)
{
    on_report_options = grbl.on_report_options;
    grbl.on_report_options = onReportOptions;

    on_state_change = grbl.on_state_change;             // Subscribe to the state changed event by saving away the original
    grbl.on_state_change = onStateChanged;              // function pointer and adding ours to the chain.

    on_program_completed = grbl.on_program_completed;   // Subscribe to on program completed events (lightshow on complete?)
    grbl.on_program_completed = onProgramCompleted;     // Checkered Flag for successful end of program lives here

    //subscribe to following events:
    //coolant
    //spindle
    //

}
