#pragma once

struct imu_data {
    float f32_acc_x;
    float f32_acc_y;
    float f32_acc_z;
    float f32_gyro_x;
    float f32_gyro_y;
    float f32_gyro_z;
};

struct dps_data {
    float f32_Alt;
    float f32_DSP_Pa;
    float f32_DSP_Temp;
    float f32_RTC_Temp;
};