//
// Created by rleroux on 4/28/24.
//

#define BRAKE_PRESSURE_MASK             0xffff // 0-1

#define RPM_MASK                        0xffff // 0-1
#define SPEED_MASK              0xffff00000000 // 4-5
#define ACCEL_MASK            0xff000000000000 // 6

#define ENGINE_LOAD_MASK                  0xff // 0
#define COOLANT_MASK                    0xff00 // 1
#define THROTTLE_MASK               0xff000000 // 3
#define INTAKE_MASK               0xff00000000 // 4

#define FUEL_LEVEL_MASK                   0xff // 0

#define FL_SPEED_MASK                   0xffff // 0-1
#define FR_SPEED_MASK               0xffff0000 // 2-3
#define RL_SPEED_MASK           0xffff00000000 // 4-5
#define RR_SPEED_MASK       0xffff000000000000 // 6-7

#define SPEED_BIT_SHIFT    (4 * 8)
#define ACCEL_BIT_SHIFT    (6 * 8)

#define COOLANT_BIT_SHIFT  (1 * 8)
#define THROTTLE_BIT_SHIFT (3 * 8)
#define INTAKE_BIT_SHIFT   (4 * 8)

#define FR_SPEED_BIT_SHIFT (2 * 8)
#define RL_SPEED_BIT_SHIFT (4 * 8)
#define RR_SPEED_BIT_SHIFT (6 * 8)

#define BRAKE_PRESSURE_OFFSET 102
#define BRAKE_PRESSURE_COEF   0.2f
#define RPM_DIV               4
#define SPEED_DIV             100.0f
#define SPEED_OFFSET          100
#define ACCEL_COEF            2
#define PCT_DIV               2.55f
#define TEMP_OFFSET           40

#include "metrics.h"
#include <stdio.h>

static uint16_t raw_speed_to_kmh(uint16_t raw_speed) {
    int16_t speed = (int16_t)(((float)raw_speed / SPEED_DIV) - SPEED_OFFSET);
    return speed < 0 ? 0 : speed;
}

static uint8_t raw_to_pct(uint8_t raw) {
    return (uint8_t)((float)raw / PCT_DIV);
}

static int16_t raw_to_temp(int16_t raw) {
    return (int16_t)(raw - TEMP_OFFSET);
}

static int handle_brakes(uint64_t can_data, struct metrics *metrics) {
    int16_t brake_pressure = (int16_t)(can_data & BRAKE_PRESSURE_MASK);
    brake_pressure -= BRAKE_PRESSURE_OFFSET;

    // Pressure can be momentarily negative (vacuum ?)
    // Limit to min 0
    metrics->brakes_pct = brake_pressure < 0 ? 0 : (uint8_t)((float)brake_pressure * BRAKE_PRESSURE_COEF);

    //printf("brakes %d %%\n", metrics->brakes_pct);

    return 0;
}

static int handle_rpm_speed_accel(uint64_t can_data, struct metrics *metrics) {
    uint16_t rpm = (uint16_t)(can_data & RPM_MASK);
    metrics->rpm = rpm / RPM_DIV;

    uint16_t speed = (uint16_t)((can_data & SPEED_MASK) >> SPEED_BIT_SHIFT);
    metrics->speed_kmh = raw_speed_to_kmh(speed);

    uint8_t accel = (uint8_t)((can_data & ACCEL_MASK) >> ACCEL_BIT_SHIFT);
    metrics->accelerator_pedal_position_pct = accel * ACCEL_COEF;

    //printf("rpm %d, speed %d kmh, accel %d %%\n",
      //     metrics->rpm, metrics->speed_kmh, metrics->accelerator_pedal_position_pct);

    return 0;
}

static int handle_load_coolant_throttle_intake(uint64_t can_data, struct metrics *metrics) {
    uint8_t engine_load = (uint8_t)(can_data & ENGINE_LOAD_MASK);
    metrics->calculated_engine_load_pct = raw_to_pct(engine_load);

    int16_t coolant_temp = (int16_t)((can_data & COOLANT_MASK) >> COOLANT_BIT_SHIFT);
    metrics->engine_coolant_temp_c = raw_to_temp(coolant_temp);

    uint8_t throttle_valve = (uint8_t)((can_data & THROTTLE_MASK) >> THROTTLE_BIT_SHIFT);
    metrics->throttle_valve_position_pct = raw_to_pct(throttle_valve);

    int16_t intake_temp = (int16_t)((can_data & INTAKE_MASK) >> INTAKE_BIT_SHIFT);
    metrics->intake_air_temp_c = raw_to_temp(intake_temp);

    //printf("engine %d %%, coolant %d °C, throttle %d %%, intake %d °C\n",
      //     metrics->calculated_engine_load_pct, metrics->engine_coolant_temp_c,
        //   metrics->throttle_valve_position_pct, metrics->intake_air_temp_c);

    return 0;
}

static int handle_fuel_level(uint64_t can_data, struct metrics *metrics) {
    uint8_t fuel_level = (uint8_t)(can_data & FUEL_LEVEL_MASK);
    metrics->fuel_level_pct = raw_to_pct(fuel_level);

    //printf("fuel %d %%\n", metrics->fuel_level_pct);

    return 0;
}

static int handle_wheel_speeds(uint64_t can_data, struct metrics *metrics) {
    uint16_t fl = (uint16_t)(can_data & FL_SPEED_MASK);
    metrics->fl_speed_kmh = raw_speed_to_kmh(fl);

    uint16_t fr = (uint16_t)((can_data & FR_SPEED_MASK) >> FR_SPEED_BIT_SHIFT);
    metrics->fr_speed_kmh = raw_speed_to_kmh(fr);

    uint16_t rl = (uint16_t)((can_data & RL_SPEED_MASK) >> RL_SPEED_BIT_SHIFT);
    metrics->rl_speed_kmh = raw_speed_to_kmh(rl);

    uint16_t rr = (uint16_t)((can_data & RR_SPEED_MASK) >> RR_SPEED_BIT_SHIFT);
    metrics->rr_speed_kmh = raw_speed_to_kmh(rr);

    //printf("fl %d fr %d rl %d rr %d kmh\n", fr, fr, rl, rr);

    return 0;
}

int handle_can_msg(uint16_t can_id, uint64_t can_data, struct metrics *metrics) {
    switch(can_id) {
        case CAN_ID_BRAKES:
            return handle_brakes(can_data, metrics);
        case CAN_ID_RPM_SPEED_ACCEL:
            return handle_rpm_speed_accel(can_data, metrics);
        case CAN_ID_COOLANT_THROTTLE_INTAKE:
            return handle_load_coolant_throttle_intake(can_data, metrics);
        case CAN_ID_FUEL_LEVEL:
            return handle_fuel_level(can_data, metrics);
        case CAN_ID_WHEEL_SPEEDS:
            return handle_wheel_speeds(can_data, metrics);
        default:
            printf("unhandled can id 0x%x\n", can_id);
            return -1;
    }
}
