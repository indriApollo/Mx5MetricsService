//
// Created by rleroux on 4/28/24.
//

#ifndef MX5METRICSSERVICE_METRICS_H
#define MX5METRICSSERVICE_METRICS_H

#include <stdint.h>

struct metrics {
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

#endif //MX5METRICSSERVICE_METRICS_H
