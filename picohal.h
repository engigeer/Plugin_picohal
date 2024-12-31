/*

  picohal.h

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

#ifdef ARDUINO
#include "../grbl/hal.h"
#include "../grbl/protocol.h"
#include "../grbl/state_machine.h"
#include "../grbl/report.h"
#include "../grbl/modbus.h"
#else
#include "grbl/hal.h"
#include "grbl/protocol.h"
#include "grbl/state_machine.h"
#include "grbl/report.h"
#include "grbl/modbus.h"
#include "driver.h"
#endif

#define PICOHAL_ADDRESS 10
#define QUEUE_SIZE 8

#define RETRY_DELAY         250
#define POLLING_INTERVAL    100
#define PICOHAL_RETRIES     5

const user_mcode_t LaserReady_On   = (user_mcode_t)510;
const user_mcode_t LaserReady_Off  = (user_mcode_t)511;
const user_mcode_t LaserMains_On   = (user_mcode_t)512;
const user_mcode_t LaserMains_Off  = (user_mcode_t)513;
const user_mcode_t LaserGuide_On   = (user_mcode_t)514;
const user_mcode_t LaserGuide_Off  = (user_mcode_t)515;
const user_mcode_t LaserEnable_On  = (user_mcode_t)516;
const user_mcode_t LaserEnable_Off = (user_mcode_t)517;

const user_mcode_t Argon_On    = (user_mcode_t)520;
const user_mcode_t Argon_Off   = (user_mcode_t)521;
const user_mcode_t Powder1_On  = (user_mcode_t)522;
const user_mcode_t Powder1_Off = (user_mcode_t)523;
const user_mcode_t Powder2_On  = (user_mcode_t)524;
const user_mcode_t Powder2_Off = (user_mcode_t)525;

typedef enum {
    TOOLCHANGE_ACK = 0,
    PROBE_START = 1,
    PROBE_COMPLETED = 2,
    PROBE_FIXTURE = 3,
    PROGRAM_COMPLETED = 30,
    HOMING_COMPLETED = 31,
    INVALID_EVENT = 255,
} picohal_events;

// typedef enum {
//     SPINDLE_Idle = 0,
//     SPINDLE_SetSpeed,
//     SPINDLE_GetSpeed,
//     SPINDLE_GetStatus,
//     SPINDLE_SetStatus,
// } picohal_response_t;

typedef union {
    uint8_t bits;                  //!< Bitmask bits
    uint8_t mask;                  //!< Bitmask
    uint8_t value;                 //!< Bitmask value
    struct {
        uint8_t ready          :1, //!< 
                mains          :1, //!< 
                guide          :1, //!< 
                enable         :1, //!< 
                unused         :4;
    };
} IPG_state_t;

typedef union {
    uint8_t bits;                  //!< Bitmask bits
    uint8_t mask;                  //!< Bitmask
    uint8_t value;                 //!< Bitmask value
    struct {
        uint8_t argon          :1, //!< 
                powder1        :1, //!< 
                powder2        :1, //!< 
                unused         :5;
    };
} BLC_state_t;

/**/
