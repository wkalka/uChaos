cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(uchaos)

# set(fogml_sources 
#     ../src/fogml_sdk/anomaly/fogml_anomaly.c
#     ../src/fogml_sdk/anomaly_rt/fogml_lof.c
#     ../src/fogml_sdk/anomaly_rt/fogml_reservoir.c
#     ../src/fogml_sdk/ports/fogml_ports_zephyr.c
#     ../src/fogml_sdk/dsp/fogml_dsp.c
#     ../src/fogml_sdk/scaler/fogml_scaler.c
# )

# set (fogml_generated
#     ../src/fogml_generated/kmeans_anomaly_model.c
#     ../src/fogml_generated/min_max_scaler_model.c
#     ../src/fogml_generated/random_forrest_model.c
# )

set (uchaos
    src/uchaos/uchaos_battery/uchaos_battery.c
    src/uchaos/uchaos_console/uchaos_console.c
    src/uchaos/uchaos_sensor/uchaos_sensor.c
)

FILE(GLOB app_sources 
    src/*.c
    src/inc/*.h
)

target_sources(app PRIVATE ${app_sources} ${uchaos})
