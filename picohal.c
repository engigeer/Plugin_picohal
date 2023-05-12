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
#include <stdio.h>

#include "picohal.h"

static on_state_change_ptr on_state_change;
static on_report_options_ptr on_report_options;
static on_program_completed_ptr on_program_completed;
static coolant_set_state_ptr on_coolant_changed; // For real time loop insertion
static driver_reset_ptr driver_reset;
//static user_mcode_ptrs_t user_mcode;
static on_probe_completed_ptr on_probe_completed;
static on_probe_start_ptr on_probe_start;
static on_probe_fixture_ptr on_probe_fixture;
static on_execute_realtime_ptr on_execute_realtime, on_execute_delay;

static uint16_t retry_counter = 0;

static void picohal_rx_packet (modbus_message_t *msg);
static void picohal_rx_exception (uint8_t code, void *context);

static const modbus_callbacks_t callbacks = {
    .on_rx_packet = picohal_rx_packet,
    .on_rx_exception = picohal_rx_exception
};

//important variables for retries
static coolant_state_t current_cooolant_state;
static sys_state_t current_state; 

#define PICOHAL_ADDRESS 10
#define QUEUE_SIZE 8

typedef struct {
    uint16_t index;
    modbus_message_t picohal_packet;
} QueueItem;

QueueItem message_queue[QUEUE_SIZE];
int front = 0;
int rear = -1;
int item_count = 0;
modbus_message_t current_message;
modbus_message_t * current_msg_ptr = &current_message;
uint16_t current_index;

static bool enqueue_message(modbus_message_t data) {
    static uint16_t message_index;
    if (item_count == QUEUE_SIZE) {
        report_message("Warning: PicoHAL queue is full.", Message_Warning);
        return 0;
    }
    rear = (rear + 1) % QUEUE_SIZE;
    message_queue[rear].picohal_packet = data;
    message_queue[rear].index = message_index;
    message_queue[rear].picohal_packet.context = &message_queue[rear].index;
    message_index++;
    item_count++;
        return 1;
}

static bool dequeue_message() {
    if (item_count == 0) {
        //report_message("Error: queue is empty", Message_Info);
        return 0;
    }
    current_message = (message_queue[front].picohal_packet);
    front = (front + 1) % QUEUE_SIZE;
    item_count--;
    return 1;
}

static bool peek_message() {
    if (item_count == 0) {
        return 0;
    }
    current_message = (message_queue[front].picohal_packet);
    //sprintf(buf, "peek_context f: %p",*picohal_packet->context);
    //sprintf(buf, "peek_context: %d",19535); 

    return 1;
}

static void picohal_send (){

    uint32_t ms = hal.get_elapsed_ticks();

    //can only send if there is something in the queue.
    //if (ms<1000)
    //    return;

    if(peek_message()){
        modbus_send(current_msg_ptr, &callbacks, false);
    }
}

static void picohal_rx_packet (modbus_message_t *msg)
{
    //check the context/index and pop it off the queue if it matches.
    //sprintf(buf, "recv_context:%d current_context: %d",*((uint16_t*)msg->context), *((uint16_t*)current_msg_ptr->context));
    //report_message(buf, Message_Plain);
    if(*((uint16_t*)msg->context) == *((uint16_t*)current_msg_ptr->context)){
        dequeue_message();
    }
    //else it should stay on the queue to be re-transmitted.
    
}

static void picohal_set_state ()
{   
    uint16_t data;
    uint16_t alarm_code;

        switch (current_state){
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

    modbus_message_t cmd = {
        .context = NULL,
        .crc_check = false,
        .adu[0] = PICOHAL_ADDRESS,
        .adu[1] = ModBus_WriteRegister,
        .adu[2] = 0x00,
        .adu[3] = 0x01, //status register
        .adu[4] = data >> 8,
        .adu[5] = data & 0xFF,
        .tx_length = 8,
        .rx_length = 8
    };
    enqueue_message(cmd);

    //if in alarm state, write the alarm code to the alarm register.
    if (data == STATE_ALARM){

        alarm_code = (uint16_t) sys.alarm;

        modbus_message_t code_cmd = {
            .context = NULL,
            .crc_check = false,
            .adu[0] = PICOHAL_ADDRESS,
            .adu[1] = ModBus_WriteRegister,
            .adu[2] = 0x00,
            .adu[3] = 0x02, //alarm code register.
            .adu[4] = alarm_code >> 8,
            .adu[5] = alarm_code & 0xFF,
            .tx_length = 8,
            .rx_length = 8
        };
        enqueue_message(code_cmd); 
    }

}

static void picohal_set_coolant ()
{       
    //set coolant state in register 0x100
    modbus_message_t cmd = {
        .context = NULL,
        .crc_check = false,
        .adu[0] = PICOHAL_ADDRESS,
        .adu[1] = ModBus_WriteRegister,
        .adu[2] = 0x01,
        .adu[3] = 0x00,
        .adu[4] = 0x00,
        .adu[5] = current_cooolant_state.value & 0xFF,
        .tx_length = 8,
        .rx_length = 8
    };
    enqueue_message(cmd);
}

static void picohal_create_event (picohal_events event){

    modbus_message_t cmd = {
        .context = NULL,
        .crc_check = false,
        .adu[0] = PICOHAL_ADDRESS,
        .adu[1] = ModBus_WriteRegister,
        .adu[2] = 0x00,
        .adu[3] = 0x05,
        .adu[4] = event >> 8,
        .adu[5] = event & 0xFF,
        .tx_length = 8,
        .rx_length = 8
    };
    enqueue_message(cmd);
}

static void picohal_set_homing_status (){
    /*
    modbus_message_t cmd = {
        .context = NULL,
        .crc_check = false,
        .adu[0] = PICOHAL_ADDRESS,
        .adu[1] = ModBus_WriteRegister,
        .adu[2] = 0x00,
        .adu[3] = 0x05,
        .adu[4] = event >> 8,
        .adu[5] = event & 0xFF,
        .tx_length = 8,
        .rx_length = 8
    };
    enqueue_message(cmd);
    */
}

static void picohal_rx_exception (uint8_t code, void *context)
{
    
    uint8_t value = *((uint8_t*)context);
    char buf[16];
    
    report_message("picohal_rx_exception", Message_Warning);
    sprintf(buf, "CODE: %d", code);
    report_message(buf, Message_Plain);   
    sprintf(buf, "CONT: %d", value);
    report_message(buf, Message_Plain);             
    //if RX exceptions during one of the messages, need to retry?
}

static void picohal_poll (void)
{
    static uint32_t last_ms;
    uint32_t ms = hal.get_elapsed_ticks();

    //control the rate at which the queue is emptied to avoid filling the modbus queue
    if(ms < last_ms + POLLING_INTERVAL)
        return;    

    //if there is a message try to send it.
    if(item_count){
        picohal_send();
        last_ms = ms;
    }
}

static void picohal_poll_realtime (sys_state_t grbl_state)
{
    on_execute_realtime(grbl_state);
    picohal_poll();
}

static void picohal_poll_delay (sys_state_t grbl_state)
{
    on_execute_delay(grbl_state);
    picohal_poll();
}

static void onReportOptions (bool newopt)
{
    on_report_options(newopt);

    if(!newopt){
        hal.stream.write("[PLUGIN:PICOHAL v0.1]"  ASCII_EOL);
    }
}

static void onCoolantChanged (coolant_state_t state){

    current_cooolant_state = state;
    picohal_set_coolant();

    if (on_coolant_changed)         // Call previous function in the chain.
        on_coolant_changed(state);    
}

static void onStateChanged (sys_state_t state)
{
    current_state = state;
    picohal_set_state();
    if (on_state_change)         // Call previous function in the chain.
        on_state_change(state);    
}

static bool onProbeStart (axes_signals_t axes, float *target, plan_line_data_t *pl_data)
{
    //write the program flow value to the event register.
    //enqueue(PROBE_START);
    picohal_create_event(PROBE_START);
    
    if(on_probe_start)
        on_probe_start(axes, target, pl_data);

    return false;
}

static void onProbeCompleted ()
{
    //write the program flow value to the event register.
    //enqueue(PROBE_COMPLETED);
    picohal_create_event(PROBE_COMPLETED);
    
    if(on_probe_completed)
        on_probe_completed();
}

static bool onProbeFixture (tool_data_t *tool, bool at_g59_3, bool on)
{
    //write the program flow value to the event register.
    //enqueue(PROBE_FIXTURE);
    picohal_create_event(PROBE_FIXTURE);
    
    if(on_probe_fixture)
        on_probe_fixture(tool, at_g59_3, on);

    return false;
}

// ON (Gcode) PROGRAM COMPLETION
static void onProgramCompleted (program_flow_t program_flow, bool check_mode)
{
    //write the program flow value to the event register.
    //enqueue(PROGRAM_COMPLETED);
    picohal_create_event(PROGRAM_COMPLETED);
    
    if(on_program_completed)
        on_program_completed(program_flow, check_mode);
}

// DRIVER RESET
static void driverReset (void)
{
    picohal_set_state();
    picohal_set_homing_status();
    driver_reset();
}

void picohal_init (void)
{
    on_report_options = grbl.on_report_options;
    grbl.on_report_options = onReportOptions;

    on_state_change = grbl.on_state_change;             // Subscribe to the state changed event by saving away the original
    grbl.on_state_change = onStateChanged;              // function pointer and adding ours to the chain.

    on_coolant_changed = hal.coolant.set_state;         //subscribe to coolant events
    hal.coolant.set_state = onCoolantChanged;

    on_probe_start = grbl.on_probe_start;               //subscribe to probe start
    grbl.on_probe_start = onProbeStart;

    on_probe_completed = grbl.on_probe_completed;               //subscribe to probe start
    grbl.on_probe_completed = onProbeCompleted; 

    on_probe_fixture = grbl.on_probe_fixture;               //subscribe to probe start
    grbl.on_probe_fixture = onProbeFixture;

    on_execute_realtime = grbl.on_execute_realtime;
    grbl.on_execute_realtime = picohal_poll_realtime;

    on_execute_delay = grbl.on_execute_delay;
    grbl.on_execute_delay = picohal_poll_delay;         

    on_program_completed = grbl.on_program_completed;   // Subscribe to on program completed events (lightshow on complete?)
    grbl.on_program_completed = onProgramCompleted;     // Checkered Flag for successful end of program lives here

    driver_reset = hal.driver_reset;                    // Subscribe to driver reset event
    hal.driver_reset = driverReset;

}

//add homing completed event.  picohal_set_homing_status to update the homing state.
