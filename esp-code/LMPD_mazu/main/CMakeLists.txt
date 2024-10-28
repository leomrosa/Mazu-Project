idf_component_register(SRCS   "main.c" "LMPD_blt/BLT_spp.c" "LMPD_ch/MSG_handler.c" "LMPD_sen/SEN_ds.c" "LMPD_rom/ROM_msd.c" "LMPD_rtc/RTC_ds.c" "LMPD_biomech/Biomech_calc.c" "LMPD_gps/GPS_air.c" "LMPD_imu/IMU_mpu.c" "LMPD_init/lmpd_init.c"
                       INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
