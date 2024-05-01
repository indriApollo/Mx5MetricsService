//
// Created by rleroux on 5/1/24.
//

#include "commands.h"
#include <assert.h>
#include <string.h>

static const char unknown_cmd_id_msg[] = "unknown cmd";

static int get_command_response(uint8_t cmd_id, const void *val, int val_len, uint8_t *buf)
{
    assert(val_len <= CMD_RSP_MAX_SIZE - CMD_ID_SIZE);
    buf[0] = cmd_id;
    memcpy(buf + 1, val, val_len);
    return val_len + 1;
}

size_t handle_command(uint8_t cmd_id, const struct metrics *metrics, uint8_t *buf)
{
    // Response bytes :
    // 0      | 1 up to RSP_BUFFER_SIZE
    // cmd id | msg
    // On error : msg is ascii

    switch (cmd_id) {
        case GET_RPM:
            return get_command_response(
                    GET_RPM,
                    &metrics->rpm, sizeof(metrics->rpm), buf);

        case GET_SPEED_KMH:
            return get_command_response(
                    GET_SPEED_KMH,
                    &metrics->speed_kmh, sizeof(metrics->speed_kmh), buf);

        case GET_ACCELERATOR_PEDAL_POSITION_PCT:
            return get_command_response(
                    GET_ACCELERATOR_PEDAL_POSITION_PCT,
                    &metrics->accelerator_pedal_position_pct, sizeof(metrics->accelerator_pedal_position_pct), buf);

        case GET_CALCULATED_ENGINE_LOAD_PCT:
            return get_command_response(
                    GET_CALCULATED_ENGINE_LOAD_PCT,
                    &metrics->calculated_engine_load_pct, sizeof(metrics->calculated_engine_load_pct), buf);

        case GET_ENGINE_COOLANT_TEMP_C:
            return get_command_response(
                    GET_ENGINE_COOLANT_TEMP_C,
                    &metrics->engine_coolant_temp_c, sizeof(metrics->engine_coolant_temp_c), buf);

        case GET_THROTTLE_VALVE_POSITION_PCT:
            return get_command_response(
                    GET_THROTTLE_VALVE_POSITION_PCT,
                    &metrics->throttle_valve_position_pct, sizeof(metrics->throttle_valve_position_pct), buf);

        case GET_INTAKE_AIR_TEMP_C:
            return get_command_response(
                    GET_INTAKE_AIR_TEMP_C,
                    &metrics->intake_air_temp_c, sizeof(metrics->intake_air_temp_c), buf);

        case GET_FUEL_LEVEL_PCT:
            return get_command_response(
                    GET_FUEL_LEVEL_PCT,
                    &metrics->fuel_level_pct, sizeof(metrics->fuel_level_pct), buf);

        case GET_BRAKES_PCT:
            return get_command_response(
                    GET_BRAKES_PCT,
                    &metrics->brakes_pct, sizeof(metrics->brakes_pct), buf);

        case GET_FL_SPEED_KMH:
            return get_command_response(
                    GET_FL_SPEED_KMH,
                    &metrics->fl_speed_kmh, sizeof(metrics->fl_speed_kmh), buf);

        case GET_FR_SPEED_KMH:
            return get_command_response(
                    GET_FR_SPEED_KMH,
                    &metrics->fr_speed_kmh, sizeof(metrics->fr_speed_kmh), buf);

        case GET_RL_SPEED_KMH:
            return get_command_response(
                    GET_RL_SPEED_KMH,
                    &metrics->rl_speed_kmh, sizeof(metrics->rl_speed_kmh), buf);

        case GET_RR_SPEED_KMH:
            return get_command_response(
                    GET_RR_SPEED_KMH,
                    &metrics->rr_speed_kmh, sizeof(metrics->rr_speed_kmh), buf);

        default:
            return get_command_response(
                    ERROR,
                    unknown_cmd_id_msg, strlen(unknown_cmd_id_msg), buf);
    }
}

const char* command_str(enum command cmd) {
    switch (cmd) {
        case ERROR:
            return "ERROR";
        case GET_RPM:
            return "GET_RPM";
        case GET_SPEED_KMH:
            return "GET_SPEED_KMH";
        case GET_ACCELERATOR_PEDAL_POSITION_PCT:
            return "GET_ACCELERATOR_PEDAL_POSITION_PCT";
        case GET_CALCULATED_ENGINE_LOAD_PCT:
            return "GET_CALCULATED_ENGINE_LOAD_PCT";
        case GET_ENGINE_COOLANT_TEMP_C:
            return "GET_ENGINE_COOLANT_TEMP_C";
        case GET_THROTTLE_VALVE_POSITION_PCT:
            return "GET_THROTTLE_VALVE_POSITION_PCT";
        case GET_INTAKE_AIR_TEMP_C:
            return "GET_INTAKE_AIR_TEMP_C";
        case GET_FUEL_LEVEL_PCT:
            return "GET_FUEL_LEVEL_PCT";
        case GET_BRAKES_PCT:
            return "GET_BRAKES_PCT";
        case GET_FL_SPEED_KMH:
            return "GET_FL_SPEED_KMH";
        case GET_FR_SPEED_KMH:
            return "GET_FR_SPEED_KMH";
        case GET_RL_SPEED_KMH:
            return "GET_RL_SPEED_KMH";
        case GET_RR_SPEED_KMH:
            return "GET_RR_SPEED_KMH";
        default:
            return "UNKNOWN_CMD";
    }
}
