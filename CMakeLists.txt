cmake_minimum_required(VERSION 3.0)
project(Mx5MetricsService C)

set(CMAKE_C_STANDARD 17)

add_executable(Mx5MetricsService main.c
        server.c
        server.h
        metrics.h
        metrics.c
        commands.c
        commands.h
        serial_port.c
        serial_port.h
        stnobd.c
        stnobd.h)
