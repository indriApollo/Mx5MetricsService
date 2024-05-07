//
// Created by rleroux on 5/1/24.
//

#ifndef MX5METRICSSERVICE_COMMANDS_H
#define MX5METRICSSERVICE_COMMANDS_H

#include <stdint.h>
#include <stddef.h>
#include "metrics.h"

#define CMD_ID_SIZE      1
#define CMD_RSP_MAX_SIZE 32

enum command {
    ERROR = 0,
    GET_RPM = 1,
    GET_SPEED_KMH = 2,
    GET_ACCELERATOR_PEDAL_POSITION_PCT = 3,
    GET_CALCULATED_ENGINE_LOAD_PCT = 4,
    GET_ENGINE_COOLANT_TEMP_C = 5,
    GET_THROTTLE_VALVE_POSITION_PCT = 6,
    GET_INTAKE_AIR_TEMP_C = 7,
    GET_FUEL_LEVEL_PCT = 8,
    GET_BRAKES_PCT = 9,
    GET_FL_SPEED_KMH = 10,
    GET_FR_SPEED_KMH = 11,
    GET_RL_SPEED_KMH = 12,
    GET_RR_SPEED_KMH = 13
};

size_t handle_command(uint8_t cmd_id, const struct metrics *metrics, uint8_t *buf);

const char* command_str(enum command cmd);

#endif //MX5METRICSSERVICE_COMMANDS_H
