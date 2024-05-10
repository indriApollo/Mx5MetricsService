//
// Created by rleroux on 4/28/24.
//

#ifndef MX5METRICSSERVICE_METRICS_H
#define MX5METRICSSERVICE_METRICS_H

#define CAN_ID_BRAKES                          0x085 // 100hz
#define CAN_ID_HEX_STR_BRAKES                  "085"
#define CAN_ID_RPM_SPEED_ACCEL                 0x201 // 100hz
#define CAN_ID_HEX_STR_RPM_SPEED_ACCEL         "201"
#define CAN_ID_COOLANT_THROTTLE_INTAKE         0x240 // 10hz
#define CAN_ID_HEX_STR_COOLANT_THROTTLE_INTAKE "240"
#define CAN_ID_FUEL_LEVEL                      0x430 // 10 hz
#define CAN_ID_HEX_STR_FUEL_LEVEL              "430"
#define CAN_ID_WHEEL_SPEEDS                    0x4b0 // 100hz
#define CAN_ID_HEX_STR_WHEEL_SPEEDS            "4B0"

#include <stdint.h>

struct __attribute__((__packed__)) metrics {
     uint16_t rpm;
     uint16_t speed_kmh;
     uint8_t accelerator_pedal_position_pct;
     uint8_t calculated_engine_load_pct;
     int16_t engine_coolant_temp_c;
     uint8_t throttle_valve_position_pct;
     int16_t intake_air_temp_c;
     uint8_t fuel_level_pct;
     uint8_t brakes_pct;
     uint16_t fl_speed_kmh;
     uint16_t fr_speed_kmh;
     uint16_t rl_speed_kmh;
     uint16_t rr_speed_kmh;
};

int handle_can_msg(uint16_t can_id, uint64_t can_data, struct metrics *metrics);

#endif //MX5METRICSSERVICE_METRICS_H
