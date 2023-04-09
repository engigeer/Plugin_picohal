/*

  shared.h - shared spindle plugin symbols

  Part of grblHAL

  Copyright (c) 2022-2023 Terje Io

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
#include "../../grbl/hal.h"
#include "../../grbl/protocol.h"
#include "../../grbl/state_machine.h"
#include "../../grbl/report.h"
#else
#include "grbl/hal.h"
#include "grbl/protocol.h"
#include "grbl/state_machine.h"
#include "grbl/report.h"
#endif

//#include <modbus.h>
#include "spindle/modbus.h"

#define RETRY_DELAY         250
#define POLLING_INTERVAL    250
#define PICOHAL_RETRIES     5

typedef enum {
    picohal_Idle = 0,
    picohal_SetState,
    picohal_SetCoolant,
    picohal_SetEvent
} picohal_response_t;

typedef enum {
    TOOLCHANGE_ACK = 0,
    PROBE_START = 1,
    PROBE_COMPLETED = 2,
    PROBE_FIXTURE = 3,
    PROGRAM_COMPLETED = 30,
    INVALID_EVENT = 255,
} picohal_events;

/**/
