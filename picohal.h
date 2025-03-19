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

typedef enum {
    LaserReady_On   = 510,
    LaserReady_Off  = 511,
    LaserMains_On   = 512,
    LaserMains_Off  = 513,
    LaserGuide_On   = 514,
    LaserGuide_Off  = 515,
    LaserShutter_On  = 516,
    LaserShutter_Off = 517,

    Argon_On    = 520,
    Argon_Off   = 521,
    Powder1_On  = 522,
    Powder1_Off = 523,
    Powder2_On  = 524,
    Powder2_Off = 525
} picohal_mcode_t;

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
                shutter         :1, //!< 
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

void picohal_init (void);

/**/
