/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 *
 * $Id: ml.c 6218 2011-10-19 04:13:47Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @defgroup ML
 *  @brief  Motion Library APIs.
 *          The Motion Library processes gyroscopes, accelerometers, and
 *          compasses to provide a physical model of the movement for the
 *          sensors.
 *          The results of this processing may be used to control objects
 *          within a user interface environment, detect gestures, track 3D
 *          movement for gaming applications, and analyze the blur created
 *          due to hand movement while taking a picture.
 *
 *  @{
 *      @file   ml.c
 *      @brief  The Motion Library APIs.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include <string.h>

#include "ml.h"
#include "mldl.h"
#include "mltypes.h"
#include "mlinclude.h"
#include "compass.h"
#include "dmpKey.h"
#include "dmpDefault.h"
#include "mlstates.h"
#include "mlFIFO.h"
#include "mlFIFOHW.h"
#include "mlMathFunc.h"
#include "mlsupervisor.h"
#include "mlmath.h"
#include "mlcontrol.h"
#include "mldl_cfg.h"
#include "mpu.h"
#include "accel.h"
#include "mlos.h"
#include "mlsl.h"
#include "mlos.h"
#include "mlBiasNoMotion.h"
#include "temp_comp.h"
#include "log.h"
#include "mlSetGyroBias.h"

#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-ml"

#define ML_MOT_TYPE_NONE 0
#define ML_MOT_TYPE_NO_MOTION 1
#define ML_MOT_TYPE_MOTION_DETECTED 2

#define ML_MOT_STATE_MOVING 0
#define ML_MOT_STATE_NO_MOTION 1
#define ML_MOT_STATE_BIAS_IN_PROG 2

#define SIGNSET(x) ((x) ? -1 : +1)
#define _mlDebug(x)             //{x}

/* Global Variables */
const unsigned char mlVer[] = { INV_VERSION };

struct inv_params_obj inv_params_obj = {
    INV_ORIENTATION_MASK_DEFAULT,   // orientation_mask
    INV_PROCESSED_DATA_CALLBACK_DEFAULT,    // fifo_processed_func
    INV_ORIENTATION_CALLBACK_DEFAULT,   // orientation_cb_func
    INV_MOTION_CALLBACK_DEFAULT,    // motion_cb_func
    INV_STATE_SERIAL_CLOSED     // starting state
};

void *g_mlsl_handle;
unsigned short inv_gyro_orient;
unsigned short inv_accel_orient;

typedef struct {
    // These describe callbacks happening everythime a DMP interrupt is processed
    int_fast8_t numInterruptProcesses;
    // Array of function pointers, function being void function taking void
    inv_obj_func processInterruptCb[MAX_INTERRUPT_PROCESSES];

} tMLXCallbackInterrupt;        // MLX_callback_t

tMLXCallbackInterrupt mlxCallbackInterrupt;

void inv_init_ml_cb (void)
{
    memset(&mlxCallbackInterrupt, 0, sizeof(tMLXCallbackInterrupt));
}


/* --------------- */
/* -  Functions. - */
/* --------------- */

inv_error_t inv_freescale_sensor_fusion_16bit();
inv_error_t inv_freescale_sensor_fusion_8bit();
void inv_mpu6050_accel(unsigned short orient, int mode, unsigned char *instr);
static inv_error_t inv_dead_zone_CB(unsigned char new_state);
static inv_error_t inv_bias_from_LPF_CB(unsigned char new_state);
static inv_error_t inv_bias_from_gravity_CB(unsigned char new_state);

/**
 *  @brief  Open serial connection with the MPU device.
 *          This is the entry point of the MPL and must be
 *          called prior to any other function call.
 *
 *  @param  port     System handle for 'port' MPU device is found on.
 *                   The significance of this parameter varies by
 *                   platform. It is passed as 'port' to MLSLSerialOpen.
 *
 *  @return INV_SUCCESS or error code.
 */
inv_error_t inv_serial_start(char const *port)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;

    if (inv_get_state() >= INV_STATE_SERIAL_OPENED)
        return INV_SUCCESS;

    result = inv_state_transition(INV_STATE_SERIAL_OPENED);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    result = inv_serial_open(port, &g_mlsl_handle);
    if (INV_SUCCESS != result) {
        (void)inv_state_transition(INV_STATE_SERIAL_CLOSED);
    }

    return result;
}

/**
 *  @brief  Close the serial communication.
 *          This function needs to be called explicitly to shut down
 *          the communication with the device.  Calling inv_dmp_close()
 *          won't affect the established serial communication.
 *  @return INV_SUCCESS; non-zero error code otherwise.
 */
inv_error_t inv_serial_stop(void)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() == INV_STATE_SERIAL_CLOSED)
        return INV_SUCCESS;

    result = inv_state_transition(INV_STATE_SERIAL_CLOSED);
    if (INV_SUCCESS != result) {
        MPL_LOGE("State Transition Failure in %s: %d\n", __func__, result);
    }
    result = inv_serial_close(g_mlsl_handle);
    if (INV_SUCCESS != result) {
        MPL_LOGE("Unable to close Serial Handle %s: %d\n", __func__, result);
    }
    return result;
}

/**
 *  @brief  Get the serial file handle to the device.
 *  @return The serial file handle.
 */
void *inv_get_serial_handle(void)
{
    INVENSENSE_FUNC_START;
    return g_mlsl_handle;
}


inv_error_t inv_read_compass_asa( long * asa )
{
    INVENSENSE_FUNC_START;
    int ii;
    long tmp[COMPASS_NUM_AXES];
    inv_error_t result;

    result = inv_compass_read_scale(tmp);
    if (result == INV_SUCCESS) {
        for (ii = 0; ii < COMPASS_NUM_AXES; ii++)
            asa[ii] = tmp[ii];
    }
    return result;
}

/**
 *  @brief  apply the choosen orientation and full scale range
 *          for gyroscopes, accelerometer, and compass.
 *  @return INV_SUCCESS if successful, a non-zero code otherwise.
 */
inv_error_t inv_apply_calibration(void)
{
    INVENSENSE_FUNC_START;
    signed char accelCal[9] = { 0 };
    signed char magCal[9] = { 0 };
    float accelScale = 0.f;
    float magScale = 0.f;

    inv_error_t result;
    int ii;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_ACCEL]) {
        for (ii = 0; ii < 9; ii++) {
            accelCal[ii] =
                mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_ACCEL]->orientation[ii];
        }
        RANGE_FIXEDPOINT_TO_FLOAT(
            mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->range, accelScale);
        inv_obj.accel->sens = (long)(accelScale * 65536L);
        /* sensitivity adjustment, typically = 2 (for +/- 2 gee) */
        if (ACCEL_ID_MPU6050 == mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->id) {
            /* the line below is an optimized version of:
               accel_sens      = accel_sens / 2 * (16384 / accel_sens_trim) */
            inv_obj.accel->sens = inv_obj.accel->sens /
                mldl_cfg->mpu_chip_info->accel_sens_trim * 8192;
        } else
            inv_obj.accel->sens /= 2;
    }
    if (mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_COMPASS]) {
        for (ii = 0; ii < 9; ii++) {
            magCal[ii] =
                mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_COMPASS]->orientation[ii];
        }
        RANGE_FIXEDPOINT_TO_FLOAT(
            mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]->range, magScale);
        inv_obj.mag->sens = (long)(magScale * 32768);
    }
    if (inv_get_state() == INV_STATE_DMP_OPENED) {
        inv_gyro_orient = inv_orientation_matrix_to_scalar(mldl_cfg->pdata->orientation);
        result = inv_gyro_dmp_cal();
        if (result != INV_SUCCESS) {
            MPL_LOGE("Unable to set Gyro DMP Calibration\n");
            return result;
        }
        result = inv_gyro_var_cal();
        if (result != INV_SUCCESS) {
            MPL_LOGE("Unable to set Gyro Variable Calibration\n");
            return result;
        }
        inv_set_accel_mounting(accelCal);
        result = inv_accel_dmp_cal();
        if (result != INV_SUCCESS) {
            MPL_LOGE("Unable to set Accel DMP Calibration\n");
            return result;
        }
        result = inv_accel_var_cal();
        if (result != INV_SUCCESS) {
            MPL_LOGE("Unable to set Accel Variable Calibration\n");
            return result;
        }
        if (mldl_cfg->slave[EXT_SLAVE_TYPE_COMPASS]) {
            result = inv_set_compass_calibration(magScale, magCal);
            if (INV_SUCCESS != result) {
                MPL_LOGE("Unable to set Mag Calibration\n");
                return result;
            }
        }
    }
    return INV_SUCCESS;
}

/**
 *  @brief  Setup the DMP to handle the accelerometer endianess.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_apply_endian_accel(void)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[4] = { 0 };
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    int endian;

    if (!mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]) {
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_CONFIGURATION);
        return INV_ERROR_INVALID_CONFIGURATION;
    }

    endian = mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->endian;

    if (mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_ACCEL]->bus !=
        EXT_SLAVE_BUS_SECONDARY) {
        endian = EXT_SLAVE_BIG_ENDIAN;
    }
    switch (endian) {
    case EXT_SLAVE_FS8_BIG_ENDIAN:
    case EXT_SLAVE_FS16_BIG_ENDIAN:
    case EXT_SLAVE_LITTLE_ENDIAN:
        regs[0] = 0;
        regs[1] = 64;
        regs[2] = 0;
        regs[3] = 0;
        break;
    case EXT_SLAVE_BIG_ENDIAN:
    default:
        regs[0] = 0;
        regs[1] = 0;
        regs[2] = 64;
        regs[3] = 0;
    }

    return inv_set_mpu_memory(KEY_D_1_236, 4, regs);
}

/**
 * @internal
 * @brief   This registers a function to be called for each time the DMP
 *          generates an an interrupt.
 *          It will be called after inv_register_fifo_rate_process() which gets called
 *          every time the FIFO data is processed.
 *          The FIFO does not have to be on for this callback.
 * @param func Function to be called when a DMP interrupt occurs.
 * @return INV_SUCCESS or non-zero error code.
 */
inv_error_t inv_register_dmp_interupt_cb(inv_obj_func func)
{
    INVENSENSE_FUNC_START;
    // Make sure we have not filled up our number of allowable callbacks
    if (mlxCallbackInterrupt.numInterruptProcesses <=
        MAX_INTERRUPT_PROCESSES - 1) {
        int kk;
        // Make sure we haven't registered this function already
        for (kk = 0; kk < mlxCallbackInterrupt.numInterruptProcesses; ++kk) {
            if (mlxCallbackInterrupt.processInterruptCb[kk] == func) {
                return INV_ERROR_INVALID_PARAMETER;
            }
        }
        // Add new callback
        mlxCallbackInterrupt.processInterruptCb[mlxCallbackInterrupt.
                                                numInterruptProcesses] = func;
        mlxCallbackInterrupt.numInterruptProcesses++;
        return INV_SUCCESS;
    }
    return INV_ERROR_MEMORY_EXAUSTED;
}

/**
 * @internal
 * @brief This unregisters a function to be called for each DMP interrupt.
 * @return INV_SUCCESS or non-zero error code.
 */
inv_error_t inv_unregister_dmp_interupt_cb(inv_obj_func func)
{
    INVENSENSE_FUNC_START;
    int kk, jj;
    // Make sure we haven't registered this function already
    for (kk = 0; kk < mlxCallbackInterrupt.numInterruptProcesses; ++kk) {
        if (mlxCallbackInterrupt.processInterruptCb[kk] == func) {
            // FIXME, we may need a thread block here
            for (jj = kk + 1; jj < mlxCallbackInterrupt.numInterruptProcesses;
                 ++jj) {
                mlxCallbackInterrupt.processInterruptCb[jj - 1] =
                    mlxCallbackInterrupt.processInterruptCb[jj];
            }
            mlxCallbackInterrupt.numInterruptProcesses--;
            return INV_SUCCESS;
        }
    }
    return INV_ERROR_INVALID_PARAMETER;
}

/**
 *  @internal
 *  @brief  Run the recorded interrupt process callbacks in the event
 *          of an interrupt.
 */
void inv_run_dmp_interupt_cb(void)
{
    int kk;
    for (kk = 0; kk < mlxCallbackInterrupt.numInterruptProcesses; ++kk) {
        if (mlxCallbackInterrupt.processInterruptCb[kk])
            mlxCallbackInterrupt.processInterruptCb[kk] (&inv_obj);
    }
}

/** @internal
* Resets the Motion/No Motion state which should be called at startup and resume.
*/
inv_error_t inv_reset_motion(void)
{
    unsigned char regs[8];
    inv_error_t result;

    /* reset the motion/no motion callback state to MOTION first */
    inv_set_motion_state(INV_MOTION);

    inv_obj.lite_fusion->motion_state = INV_MOTION;
    inv_obj.sys->flags[INV_MOTION_STATE_CHANGE] = INV_MOTION;
    inv_obj.lite_fusion->no_motion_accel_time = inv_get_tick_count();
    regs[0] = (unsigned char)((inv_obj.lite_fusion->motion_duration >> 8) & 0xff);
    regs[1] = (unsigned char)(inv_obj.lite_fusion->motion_duration & 0xff);
    result = inv_set_mpu_memory(KEY_D_1_106, 2, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    memset(regs, 0, 8);
    result = inv_set_mpu_memory(KEY_D_1_96, 8, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    result = inv_set_mpu_memory(KEY_D_0_96, 4, inv_int32_to_big8(   0x40000000, regs));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    /* Motion is being reset, need to push the bias's back down */
    if (inv_dmpkey_supported(KEY_D_2_96)) {
        unsigned char bias[4 * GYRO_NUM_AXES];
        long bias_value[GYRO_NUM_AXES], bias_tmp[GYRO_NUM_AXES];
        int ii;
        for (ii = 0; ii < GYRO_NUM_AXES; ii++) {
            bias_tmp[ii] = inv_q30_mult(inv_obj.gyro->bias[ii], 767603923L);
        }
        for (ii = 0; ii < GYRO_NUM_AXES; ii++) {
            bias_value[ii] =
                inv_q30_mult(bias_tmp[0],
                             inv_obj.calmat->gyro_orient[3 * ii]) +
                inv_q30_mult(bias_tmp[1],
                             inv_obj.calmat->gyro_orient[3 * ii + 1]) +
                inv_q30_mult(bias_tmp[2],
                             inv_obj.calmat->gyro_orient[3 * ii + 2]);
            inv_int32_to_big8(bias_value[ii], &bias[4 * ii]);
        }
        result = inv_set_mpu_memory(KEY_D_2_96, 12, bias);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    } else {
        result = inv_set_gyro_bias_in_dps(inv_obj.gyro->bias, 0);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }
    inv_set_motion_state(INV_MOTION);
    return result;
}

/**
 * @internal
 * @brief   inv_start_bias_calc starts the bias calculation on the MPU.
 */
void inv_start_bias_calc(void)
{
    INVENSENSE_FUNC_START;

    inv_obj.adv_fusion->biascalc_suspend = 1;
}

/**
 * @internal
 * @brief   inv_stop_bias_calc stops the bias calculation on the MPU.
 */
void inv_stop_bias_calc(void)
{
    INVENSENSE_FUNC_START;

    inv_obj.adv_fusion->biascalc_suspend = 0;
}

/**
 *  @brief  inv_update_data fetches data from the fifo and updates the
 *          motion algorithms.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          and inv_dmp_start() must have been called.
 *
 *  @note   Motion algorithm data is constant between calls to inv_update_data
 *
 * @return
 * - INV_SUCCESS
 * - Non-zero error code
 */
inv_error_t inv_update_data(void)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;
    int_fast8_t got, ftry;
    uint_fast8_t mpu_interrupt;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (inv_get_state() != INV_STATE_DMP_STARTED)
        return INV_ERROR_SM_IMPROPER_STATE;

    // Set the maximum number of FIFO packets we want to process
    if (mldl_cfg->inv_mpu_cfg->requested_sensors & INV_DMP_PROCESSOR) {
        ftry = 100;             // Large enough to process all packets
    } else {
        ftry = 1;
    }

    // Go and process at most ftry number of packets, probably less than ftry
    result = inv_read_and_process_fifo(ftry, &got);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    // Process all interrupts
    mpu_interrupt = inv_get_interrupt_trigger(INTSRC_AUX1);
    if (mpu_interrupt) {
        inv_clear_interrupt_trigger(INTSRC_AUX1);
    }
    // Check if interrupt was from MPU
    mpu_interrupt = inv_get_interrupt_trigger(INTSRC_MPU);
    if (mpu_interrupt) {
        inv_clear_interrupt_trigger(INTSRC_MPU);
    }
    // Take care of the callbacks that want to be notified when there was an MPU interrupt
    if (mpu_interrupt) {
        inv_run_dmp_interupt_cb();
    }

    result = inv_get_fifo_status();
    return result;
}

/**
 *  @brief  inv_check_flag returns the value of a flag.
 *          inv_check_flag can be used to check a number of flags,
 *          allowing users to poll flags rather than register callback
 *          functions. If a flag is set to True when inv_check_flag is called,
 *          the flag is automatically reset.
 *          The flags are:
 *          - INV_RAW_DATA_READY
 *          Indicates that new raw data is available.
 *          - INV_PROCESSED_DATA_READY
 *          Indicates that new processed data is available.
 *          - INV_GOT_GESTURE
 *          Indicates that a gesture has been detected by the gesture engine.
 *          - INV_MOTION_STATE_CHANGE
 *          Indicates that a change has been made from motion to no motion,
 *          or vice versa.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          and inv_dmp_start() must have been called.
 *
 *  @param flag The flag to check.
 *
 * @return true or false state of the flag
 */
int inv_check_flag(int flag)
{
    INVENSENSE_FUNC_START;
    int flagReturn = inv_obj.sys->flags[flag];

    inv_obj.sys->flags[flag] = 0;
    return flagReturn;
}

/**
 *  @brief  Enable generation of the DMP interrupt when Motion or no-motion
 *          is detected
 *  @param on
 *          Boolean to turn the interrupt on or off.
 *  @return INV_SUCCESS or non-zero error code.
 */
inv_error_t inv_set_motion_interrupt(unsigned char on)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    unsigned char regs[2] = { 0 };

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (on) {
        result = inv_set_dl_cfg_int(BIT_DMP_INT_EN);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        inv_obj.sys->interrupt_sources |= INV_INT_MOTION;
    } else {
        inv_obj.sys->interrupt_sources &= ~INV_INT_MOTION;
        if (!inv_obj.sys->interrupt_sources) {
            result = inv_set_dl_cfg_int(0);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    }

    if (on) {
        regs[0] = DINAFE;
    } else {
        regs[0] = DINAD8;
    }
    result = inv_set_mpu_memory(KEY_CFG_7, 1, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

/**
 * Enable generation of the DMP interrupt when a FIFO packet is ready
 *
 * @param on Boolean to turn the interrupt on or off
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_set_fifo_interrupt(unsigned char on)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    unsigned char regs[2] = { 0 };

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (on) {
        result = inv_set_dl_cfg_int(BIT_DMP_INT_EN);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        inv_obj.sys->interrupt_sources |= INV_INT_FIFO;
    } else {
        inv_obj.sys->interrupt_sources &= ~INV_INT_FIFO;
        if (!inv_obj.sys->interrupt_sources) {
            result = inv_set_dl_cfg_int(0);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    }

    if (on) {
        regs[0] = DINAFE;
    } else {
        regs[0] = DINAD8;
    }
    result = inv_set_mpu_memory(KEY_CFG_6, 1, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

/**
 * Enable generation of the DMP interrupt when data is ready for the DMP.
 *
 * This IRQ can be used to get a timestamp right before the DMP starts
 * processing the data.  The FIFO interrupt can be between 2 and 5 ms later.
 *
 * @param on Boolean to turn the interrupt on or off
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_set_dmp_dr_interrupt(unsigned char on)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    unsigned char regs[2] = { 0 };

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (!inv_dmpkey_supported(KEY_CFG_DR_INT)) {
        LOG_RESULT_LOCATION(INV_ERROR_FEATURE_NOT_IMPLEMENTED);
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    }

    if (on) {
        result = inv_set_dl_cfg_int(BIT_DMP_INT_EN);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        inv_obj.sys->interrupt_sources |= INV_INT_DMP_DR;
    } else {
        inv_obj.sys->interrupt_sources &= ~INV_INT_DMP_DR;
        if (!inv_obj.sys->interrupt_sources) {
            result = inv_set_dl_cfg_int(0);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    }

    if (on) {
        regs[0] = DIND40 + 1;
    } else {
        regs[0] = DINAD8;
    }
    result = inv_set_mpu_memory(KEY_CFG_DR_INT, 1, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

/**
 * @brief   Get the current set of DMP interrupt sources.
 *          These interrupts are generated by the DMP and can be
 *          routed to the MPU interrupt line via internal
 *          settings.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 * @return  Currently enabled interrupt sources.  The possible interrupts are:
 *          - INV_INT_FIFO,
 *          - INV_INT_MOTION,
 *          - INV_INT_TAP
 */
int inv_get_interrupts(void)
{
    INVENSENSE_FUNC_START;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return 0;               // error

    return inv_obj.sys->interrupt_sources;
}

void inv_mpu6050_accel(unsigned short orient, int mode, unsigned char *regs)
{
    unsigned char tmp[3];
    unsigned char tmp1;
    unsigned char tmp2;

    regs[0] = DINA90 + 7;
    regs[1] = DINA90 + 7;
    regs[2] = DINA90 + 7;
    regs[3] = DINA90 + 7;
    regs[4] = DINA90 + 7;
    regs[5] = DINA90 + 7;

    switch (mode) {
        case 0:
            tmp[0] = DINA0C;
            tmp[1] = DINAC9;
            tmp[2] = DINA2C;
            regs[0] = tmp[orient & 3];
            regs[1] = tmp[(orient >> 3) & 3];
            regs[2] = tmp[(orient >> 6) & 3];
            break;
        case 1:
            tmp[0] = DINA4C;
            tmp[1] = DINACD;
            tmp[2] = DINA6C;
            tmp1 = DINA80 + 1;
            regs[0] = tmp1;
            regs[1] = tmp[orient & 3];
            regs[2] = tmp[(orient >> 3) & 3];
            regs[3] = tmp[(orient >> 6) & 3];
            break;
        case 2:
            tmp[0] = DINACF;
            tmp[1] = DINA0C;
            tmp[2] = DINAC9;
            tmp1 = DINA80;
            tmp2 = DINA80 + 1;
            if (orient & 3) { //X is Z or Y
                regs[0] = tmp2;
                regs[1] = tmp[orient & 3];
                if ((orient >> 3) & 3) { //ZYX or YZX
                    regs[2] = tmp[(orient >> 3) & 3];
                    regs[3] = tmp1;
                    regs[4] = tmp[(orient >> 6) & 3];
                } else { //ZXY or YXZ
                    regs[2] = tmp1;
                    regs[3] = tmp[(orient >> 3) & 3];
                    regs[4] = tmp2;
                    regs[5] = tmp[(orient >> 6) & 3];
                }
            } else { //XYZ or XZY
                regs[0] = tmp1;
                regs[1] = tmp[orient & 3];
                regs[2] = tmp2;
                regs[3] = tmp[(orient >> 3) & 3];
                regs[4] = tmp[(orient >> 6) & 3];
            }
            break;
        default:
            break;
    }
}

inv_error_t inv_accel_dmp_cal(void)
{
    INVENSENSE_FUNC_START;
    unsigned long sf;
    inv_error_t result;
    unsigned char regs[6] = { 0, 0, 0, 0, 0, 0};
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (inv_get_state() != INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (!mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]) {
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_CONFIGURATION);
        return INV_ERROR_INVALID_CONFIGURATION;
    }

    /* Apply zero g-offset values */
    if (ACCEL_ID_KXSD9 == mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->id) {
        regs[0] = 0x80;
        regs[1] = 0x0;
        regs[2] = 0x80;
        regs[3] = 0x0;
    }

    if (inv_dmpkey_supported(KEY_D_1_152)) {
        result = inv_set_mpu_memory(KEY_D_1_152, 4, &regs[0]);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    if (mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->id) {
        unsigned char tmp[3] = { DINA0C, DINAC9, DINA2C };
        struct mldl_cfg *mldl_cfg = inv_get_dl_config();
        unsigned char regs[3];

        regs[0] = tmp[inv_accel_orient & 3];
        regs[1] = tmp[(inv_accel_orient >> 3) & 3];
        regs[2] = tmp[(inv_accel_orient >> 6) & 3];
        result = inv_set_mpu_memory(KEY_FCFG_2, 3, regs);

        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        regs[0] = DINA26;
        regs[1] = DINA46;
        if (MPL_PROD_KEY(mldl_cfg->mpu_chip_info->product_id,
                         mldl_cfg->mpu_chip_info->product_revision)
            == MPU_PRODUCT_KEY_B1_E1_5)
            regs[2] = DINA76;
        else
            regs[2] = DINA66;
        if (inv_accel_orient & 4)
            regs[0] |= 1;
        if (inv_accel_orient & 0x20)
            regs[1] |= 1;
        if (inv_accel_orient & 0x100)
            regs[2] |= 1;

        result = inv_set_mpu_memory(KEY_FCFG_7, 3, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

    }

    if (inv_obj.accel->sens != 0) {
        sf = (1073741824L / inv_obj.accel->sens);
    } else {
        sf = 0;
    }
    regs[0] = (unsigned char)((sf >> 8) & 0xff);
    regs[1] = (unsigned char)(sf & 0xff);
    result = inv_set_mpu_memory(KEY_D_0_108, 2, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

void inv_set_accel_mounting(signed char *mount)
{
    inv_accel_orient = inv_orientation_matrix_to_scalar(mount);
}

inv_error_t inv_accel_var_cal()
{
    INVENSENSE_FUNC_START;

    if (inv_get_state() != INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    memset(inv_obj.calmat->accel, 0, 9*sizeof(long));

    inv_obj.calmat->accel[inv_accel_orient & 0x03] = SIGNSET(inv_accel_orient & 0x004)*inv_obj.accel->sens;
    inv_obj.calmat->accel[((inv_accel_orient & 0x18) >> 3) + 3] = SIGNSET(inv_accel_orient & 0x004)*inv_obj.accel->sens;
    inv_obj.calmat->accel[((inv_accel_orient & 0xc0) >> 6) + 6] = SIGNSET(inv_accel_orient & 0x100)*inv_obj.accel->sens;

    return INV_SUCCESS;
}

inv_error_t inv_gyro_var_cal()
{
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (inv_get_state() != INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (mldl_cfg->mpu_chip_info->gyro_sens_trim != 0) {
        inv_obj.gyro->sens = ((250L << mldl_cfg->mpu_gyro_cfg->full_scale) * (((131L << 15)) / mldl_cfg->mpu_chip_info->gyro_sens_trim));
    } else {
        inv_obj.gyro->sens = 2000L << 15;
    }
    memset(inv_obj.calmat->gyro, 0, 9*sizeof(long));
    memset(inv_obj.calmat->gyro_orient, 0, 9*sizeof(long));

    inv_obj.calmat->gyro[inv_gyro_orient & 0x03] = SIGNSET(inv_gyro_orient & 0x004)*inv_obj.gyro->sens;
    inv_obj.calmat->gyro[((inv_gyro_orient & 0x18) >> 3) + 3] = SIGNSET(inv_gyro_orient & 0x020)*inv_obj.gyro->sens;
    inv_obj.calmat->gyro[((inv_gyro_orient & 0xc0) >> 6) + 6] = SIGNSET(inv_gyro_orient & 0x100)*inv_obj.gyro->sens;
    inv_obj.calmat->gyro_orient[inv_gyro_orient & 0x03] = SIGNSET(inv_gyro_orient & 0x004)*(1L<<30);
    inv_obj.calmat->gyro_orient[((inv_gyro_orient & 0x18) >> 3)+3] = SIGNSET(inv_gyro_orient & 0x020)*(1L<<30);
    inv_obj.calmat->gyro_orient[((inv_gyro_orient & 0xc0) >> 6)+6] = SIGNSET(inv_gyro_orient & 0x100)*(1L<<30);

    //sf = (gyroSens) * (0.5 * (pi/180) / 200.0) * 16384
    inv_obj.gyro->sf = inv_q30_mult(inv_obj.gyro->sens, 767603923L);

    return INV_SUCCESS;
}

inv_error_t inv_gyro_dmp_cal()
{
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    unsigned char regs[4];
    inv_error_t result;
        unsigned char tmpD = DINA4C;
        unsigned char tmpE = DINACD;
        unsigned char tmpF = DINA6C;

    if (inv_get_state() != INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if ( (inv_gyro_orient & 3) == 0 )
        regs[0] = tmpD;
    else if ( (inv_gyro_orient & 3) == 1 )
        regs[0] = tmpE;
    else if ( (inv_gyro_orient & 3) == 2 )
        regs[0] = tmpF;
    if ( (inv_gyro_orient & 0x18) == 0 )
        regs[1] = tmpD;
    else if ( (inv_gyro_orient & 0x18) == 0x8 )
        regs[1] = tmpE;
    else if ( (inv_gyro_orient & 0x18) == 0x10 )
        regs[1] = tmpF;
    if ( (inv_gyro_orient & 0xc0) == 0 )
        regs[2] = tmpD;
    else if ( (inv_gyro_orient & 0xc0) == 0x40 )
        regs[2] = tmpE;
    else if ( (inv_gyro_orient & 0xc0) == 0x80 )
        regs[2] = tmpF;

    result = inv_set_mpu_memory(KEY_FCFG_1, 3, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (inv_gyro_orient & 4)
        regs[0] = DINA36 | 1;
    else
        regs[0] = DINA36;
    if (inv_gyro_orient & 0x20)
        regs[1] = DINA56 | 1;
    else
        regs[1] = DINA56;
    if (inv_gyro_orient & 0x100)
        regs[2] = DINA76 | 1;
    else
        regs[2] = DINA76;

    result = inv_set_mpu_memory(KEY_FCFG_3, 3, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (mldl_cfg->mpu_chip_info->gyro_sens_trim != 0) {
        inv_obj.gyro->sens = (250L << mldl_cfg->mpu_gyro_cfg->full_scale) *
                     ((131L << 15) / mldl_cfg->mpu_chip_info->gyro_sens_trim);
    } else {
        inv_obj.gyro->sens = 2000L << 15;
    }

    inv_obj.gyro->sf = inv_q30_mult(inv_obj.gyro->sens, 767603923L);
    result = inv_set_mpu_memory(KEY_D_0_104, 4,
                inv_int32_to_big8(inv_obj.gyro->sf, regs));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return INV_SUCCESS;
}

/**
 *  @brief  Sets up the Compass calibration and scale factor.
 *
 *          Please refer to the provided "9-Axis Sensor Fusion Application
 *          Note" document provided.  Section 5, "Sensor Mounting Orientation"
 *          offers a good coverage on the mounting matrices and explains
 *          how to use them.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *  @pre    inv_dmp_start() must have <b>NOT</b> been called.
 *
 *  @see    inv_set_gyro_calibration().
 *  @see    inv_set_accel_calibration().
 *
 *  @param[in] range
 *                  The range of the compass.
 *  @param[in] orientation
 *                  A 9 element matrix that represents how the compass is
 *                  oriented with respect to the device they are mounted in.
 *                  A typical set of values are {1, 0, 0, 0, 1, 0, 0, 0, 1}.
 *                  This example corresponds to a 3 x 3 identity matrix.
 *                  The matrix describes how to go from the chip mounting to
 *                  the body of the device.
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_set_compass_calibration(float range, signed char *orientation)
{
    INVENSENSE_FUNC_START;
    float cal[9];
    float scale = range / 32768.f;
    int kk;
    unsigned short compassId = 0;
    int errors = 0;

    compassId = inv_get_compass_id();

    if ((compassId == COMPASS_ID_YAS529) || (compassId == COMPASS_ID_HMC5883)
        || (compassId == COMPASS_ID_LSM303DLH)  || (compassId == COMPASS_ID_LSM303DLM)) {
        scale /= 32.0f;
    }

    for (kk = 0; kk < 9; ++kk) {
        cal[kk] = scale * orientation[kk];
        inv_obj.calmat->compass[kk] = (long)(cal[kk] * (float)(1L << 30));
    }

    inv_obj.mag->sens = (long)(scale * 1073741824L);

    if (inv_dmpkey_supported(KEY_CPASS_MTX_00)) {
        unsigned char reg0[4] = { 0, 0, 0, 0 };
        unsigned char regp[4] = { 64, 0, 0, 0 };
        unsigned char regn[4] = { 64 + 128, 0, 0, 0 };
        unsigned char *reg;
        int_fast8_t kk;
        unsigned short keyList[9] =
            { KEY_CPASS_MTX_00, KEY_CPASS_MTX_01, KEY_CPASS_MTX_02,
            KEY_CPASS_MTX_10, KEY_CPASS_MTX_11, KEY_CPASS_MTX_12,
            KEY_CPASS_MTX_20, KEY_CPASS_MTX_21, KEY_CPASS_MTX_22
        };

        for (kk = 0; kk < 9; ++kk) {

            if (orientation[kk] == 1)
                reg = regp;
            else if (orientation[kk] == -1)
                reg = regn;
            else
                reg = reg0;
            errors += inv_set_mpu_memory(keyList[kk], 4, reg);
        }
    }
    if (errors) {
        LOG_RESULT_LOCATION(INV_ERROR_MEMORY_SET);
        return INV_ERROR_MEMORY_SET;
    }

    return INV_SUCCESS;
}

/**
 *  @brief  @e inv_set_dead_zone_high is used to set a large gyro dead zone.
 *          This setting is typically used when high amounts of jitter are
 *          expected.For the 3050, calibrated
 *          gyro data can be zero as a result. For 6050, only the quaternion
 *          will be affected.
 *
 *  @return INV_SUCCESS if successful.
 */
inv_error_t inv_set_dead_zone_high(void) {
    inv_error_t result;
    unsigned char reg = 0x08, is_registered;

    /* Unregister callback if needed. */
    result = inv_check_state_callback(inv_dead_zone_CB, &is_registered);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    if (is_registered) {
        result = inv_unregister_state_callback(inv_dead_zone_CB);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    MPL_LOGV("Dead zone enabled (high).\n");

    result = inv_set_mpu_memory(KEY_D_0_163, 1, &reg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return INV_SUCCESS;
}

/**
 *  @brief  @e inv_set_dead_zone_zero is used to disable the gyro dead zone.
 *
 *  @return INV_SUCCESS if successful.
 */
inv_error_t inv_set_dead_zone_zero(void) {
    inv_error_t result;
    unsigned char reg = 0, is_registered;

    /* Unregister callback if needed. */
    result = inv_check_state_callback(inv_dead_zone_CB, &is_registered);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    if (is_registered) {
        result = inv_unregister_state_callback(inv_dead_zone_CB);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    MPL_LOGV("Dead zone disabled.\n");

    result = inv_set_mpu_memory(KEY_D_0_163, 1, &reg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return INV_SUCCESS;
}

/**
 *  @brief      @e inv_set_dead_zone_normal is used to enable the gyro dead
 *              zone.
 *              This setting can be configured such that the dead zone is only
 *              enabled when the compass is not used. For the 3050, calibrated
 *              gyro data can be zero as a result. For 6050, only the quaternion
 *              will be affected.
 *
 *  @param[in]  check_compass   If 1, only enable dead zone if compass is not
 *                              present.
 *
 *  @return     INV_SUCCESS if successful.
 */
inv_error_t inv_set_dead_zone_normal(int check_compass) {
    inv_error_t result;
    unsigned char reg, is_registered;

    if (!check_compass || !inv_compass_present()) {
        MPL_LOGV("Dead zone enabled.\n");
        reg = 0x02;
    } else {
        MPL_LOGV("Dead zone disabled.\n");
        reg = 0x00;
    }

    /* Check for dead zone callback. */
    result = inv_check_state_callback(inv_dead_zone_CB, &is_registered);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (check_compass) {
        if (!is_registered) {
            result = inv_register_state_callback(inv_dead_zone_CB);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    } else {
        if (is_registered) {
            result = inv_unregister_state_callback(inv_dead_zone_CB);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    }    

    result = inv_set_mpu_memory(KEY_D_0_163, 1, &reg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return INV_SUCCESS;
}

/**
 *  @brief      @e inv_dead_zone_CB changes the dead zone on power state
 *              changes.
 *              This callback is registered when the user calls
 *              @e inv_set_dead_zone_normal(true) and an external compass is
 *              connected. If the compass is turned off, the gyro dead zone
 *              is enabled; otherwise, the dead zone is set to zero.
 *
 *  @param[in]  new_state   New power state.
 *
 *  @return     INV_SUCCESS if successful.
 */
inv_error_t inv_dead_zone_CB(unsigned char new_state)
{
    inv_error_t result;
    unsigned char reg;
    if (new_state == INV_STATE_DMP_STARTED) {
        if (!inv_compass_present()) {
            MPL_LOGV("Dead zone enabled.\n");
            reg = 0x02;
        } else {
            MPL_LOGV("Dead zone disabled.\n");
            reg = 0x00;
        }

        result = inv_set_mpu_memory(KEY_D_0_163, 1, &reg);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    return INV_SUCCESS;
}

/**
 *  @brief      @e inv_disable_bias_from_LPF disables the algorithm to
 *              calculate gyroscope bias from LPF.
 *
 *  @return     INV_SUCCESS if successful.
 */
inv_error_t inv_disable_bias_from_LPF(void)
{
    inv_error_t result;
    unsigned char regs[4] = {DINA80 + 7, DINA2D, DINA35, DINA3D};
    unsigned char is_registered;

    /* Unregister callback if needed. */
    result = inv_check_state_callback(inv_bias_from_LPF_CB, &is_registered);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    if (is_registered) {
        result = inv_unregister_state_callback(inv_bias_from_LPF_CB);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    result = inv_set_mpu_memory(KEY_FCFG_5, 4, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    MPL_LOGV("Bias from LPF disabled.\n");

    return INV_SUCCESS;
}

/**
 *  @brief      @e inv_enable_bias_from_LPF enables the algorithm to calculate
 *              gyroscope bias from LPF.
 *
 *  @param[in]  check_compass   If 1, only update bias from LPF if compass is
 *                              not present.
 *
 *  @return     INV_SUCCESS if successful.
 */
inv_error_t inv_enable_bias_from_LPF(int check_compass)
{
    inv_error_t result;
    unsigned char regs[4], is_registered;

    if (!check_compass || !inv_compass_present()) {
        MPL_LOGV("Bias from LPF enabled.\n");
        regs[0] = DINA80 + 2;
        regs[1] = DINA2D;
        regs[2] = DINA55;
        regs[3] = DINA7D;
    } else {
        MPL_LOGV("Bias from LPF disabled.\n");
        regs[0] = DINA80 + 7;
        regs[1] = DINA2D;
        regs[2] = DINA35;
        regs[3] = DINA3D;
    }

    /* Check for bias from LPF callback. */
    result = inv_check_state_callback(inv_bias_from_LPF_CB, &is_registered);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (check_compass) {
        if (!is_registered) {
            result = inv_register_state_callback(inv_bias_from_LPF_CB);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    } else {
        if (is_registered) {
            result = inv_unregister_state_callback(inv_bias_from_LPF_CB);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    }

    result = inv_set_mpu_memory(KEY_FCFG_5, 4, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return INV_SUCCESS;
}

/**
 *  @brief      @e inv_bias_from_LPF_CB enables/disables gyro bias from LPF
                algorithm on power state changes.
 *              This callback is registered when the user calls
 *              @e inv_enable_bias_from_LPF(true). If the compass is turned 
 *              off, the bias from LPF algorithm is enabled; otherwise the
 *              algorithm is turned off.
 *
 *  @param[in]  new_state   New power state.
 *
 *  @return     INV_SUCCESS if successful.
 */
inv_error_t inv_bias_from_LPF_CB(unsigned char new_state)
{
    inv_error_t result;
    unsigned char regs[4];
    if (new_state == INV_STATE_DMP_STARTED) {
        if (!inv_compass_present()) {
            MPL_LOGV("Bias from LPF enabled.\n");
            regs[0] = DINA80 + 2;
            regs[1] = DINA2D;
            regs[2] = DINA55;
            regs[3] = DINA7D;
        } else {
            MPL_LOGV("Bias from LPF disabled.\n");
            regs[0] = DINA80 + 7;
            regs[1] = DINA2D;
            regs[2] = DINA35;
            regs[3] = DINA3D;
        }

        result = inv_set_mpu_memory(KEY_FCFG_5, 4, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    return INV_SUCCESS;
}

/**
 *  @brief      @e inv_enable_bias_from_gravity turns on the algorithm to
 *              produce gyro data from the 6-axis quaternion. 
 *              This function determines which type of data (raw gyro or 
 *              accel-compensated gyro) will appear in the FIFO.
 *
 *  @param[in]  check_compass   If 1, algorithm is only used when compass is
 *                              not used.
 *
 *  @return INV_SUCCESS if successful.
 */
inv_error_t inv_enable_bias_from_gravity(int check_compass)
{
    inv_error_t result;
    unsigned char is_registered;

    if (!check_compass || !inv_compass_present()) {
        MPL_LOGV("Bias from Gravity enabled.\n");
        result = inv_set_gyro_data_source(INV_GYRO_FROM_QUATERNION);
    } else {
        MPL_LOGV("Bias from Gravity disabled.\n");
        result = inv_set_gyro_data_source(INV_GYRO_FROM_RAW);
    }
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    /* Check for bias from gravity callback. */
    result = inv_check_state_callback(inv_bias_from_gravity_CB, 
        &is_registered);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (check_compass) {
        if (!is_registered) {
            /* Register callback. */
            result = inv_register_state_callback(inv_bias_from_gravity_CB);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    } else {
        if (is_registered) {
            /* Unregister callback. */
            result = inv_unregister_state_callback(inv_bias_from_gravity_CB);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    }

    return INV_SUCCESS;
}

/**
 *  @brief  @e inv_disable_bias_from_gravity turns off the algorithm to
 *          produce gyro data from the 6-axis quaternion.
 *
 *  @return INV_SUCCESS if successful.
 */
inv_error_t inv_disable_bias_from_gravity(void)
{
    inv_error_t result;
    unsigned char is_registered;

    result = inv_set_gyro_data_source(INV_GYRO_FROM_RAW);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    /* Check for bias from gravity callback. */
    result = inv_check_state_callback(inv_bias_from_gravity_CB, 
        &is_registered);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (is_registered) {
        /* Unregister callback. */
        result = inv_unregister_state_callback(inv_bias_from_gravity_CB);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    MPL_LOGV("Bias from Gravity disabled.\n");

    return INV_SUCCESS;
}

/**
 *  @brief      @e inv_bias_from_gravity_CB enables/disables the gyro bias from
 *              gravity algorithm on power state changes.
 *              This callback is registered when the user calls
 *              @e inv_enable_bias_from_gravity(true). If the compass is turned
 *              off, the algorithm is enabled; otherwise the algorithm is
 *              disabled.
 *
 *  @param[in]  new_state   New power state.
 *
 *  @return     INV_SUCCESS if successful.
 */
inv_error_t inv_bias_from_gravity_CB(unsigned char new_state)
{
    inv_error_t result;

    if (inv_compass_present()) {
        MPL_LOGV("Bias from Gravity disabled.\n");
        result = inv_set_gyro_data_source(INV_GYRO_FROM_RAW);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    } else {
        MPL_LOGV("Bias from Gravity enabled.\n");
        result = inv_set_gyro_data_source(INV_GYRO_FROM_QUATERNION);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    return INV_SUCCESS;
}

/**
 *  @brief  inv_get_motion_state is used to determine if the device is in
 *          a 'motion' or 'no motion' state.
 *          inv_get_motion_state returns INV_MOTION of the device is moving,
 *          or INV_NO_MOTION if the device is not moving.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          and inv_dmp_start()
 *          must have been called.
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
int inv_get_motion_state(void)
{
    INVENSENSE_FUNC_START;
    return inv_obj.lite_fusion->motion_state;
}

/**
 *  @brief  inv_set_no_motion_thresh is used to set the threshold for
 *          detecting INV_NO_MOTION
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  thresh  A threshold scaled in degrees per second.
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_set_no_motion_thresh(float thresh)
{
    inv_error_t result = INV_SUCCESS;
    unsigned char regs[4] = { 0 };
    long tmp;
    INVENSENSE_FUNC_START;

    tmp = (long)(thresh * thresh * 2.045f);
    if (tmp < 0) {
        return INV_ERROR;
    } else if (tmp > 8180000L) {
        return INV_ERROR;
    }

    regs[0] = (unsigned char)(tmp >> 24);
    regs[1] = (unsigned char)((tmp >> 16) & 0xff);
    regs[2] = (unsigned char)((tmp >> 8) & 0xff);
    regs[3] = (unsigned char)(tmp & 0xff);

    result = inv_set_mpu_memory(KEY_D_1_108, 4, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_reset_motion();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

/**
 *  @brief  inv_set_no_motion_threshAccel is used to set the threshold for
 *          detecting INV_NO_MOTION with accelerometers when Gyros have
 *          been turned off
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  thresh  A threshold in g's scaled by 2^32
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_set_no_motion_threshAccel(long thresh)
{
    INVENSENSE_FUNC_START;

    inv_obj.lite_fusion->no_motion_accel_threshold = thresh;
    inv_obj.lite_fusion->no_motion_accel_sqrt_threshold = 1 + (long)sqrtf((float)thresh);
    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_no_motion_time is used to set the time required for
 *          detecting INV_NO_MOTION
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  time    A time in seconds.
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_set_no_motion_time(float time)
{
    inv_error_t result = INV_SUCCESS;
    unsigned char regs[2] = { 0 };
    long tmp;

    INVENSENSE_FUNC_START;

    tmp = (long)(time * 200);
    if (tmp < 0) {
        return INV_ERROR;
    } else if (tmp > 65535L) {
        return INV_ERROR;
    }
    inv_obj.lite_fusion->motion_duration = (unsigned short)tmp;

    regs[0] = (unsigned char)((inv_obj.lite_fusion->motion_duration >> 8) & 0xff);
    regs[1] = (unsigned char)(inv_obj.lite_fusion->motion_duration & 0xff);
    result = inv_set_mpu_memory(KEY_D_1_106, 2, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_reset_motion();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
	
    return result;
}

/**
 *  @brief  inv_get_version is used to get the ML version.
 *
 *  @pre    inv_get_version can be called at any time.
 *
 *  @param  version     inv_get_version writes the ML version
 *                      string pointer to version.
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_get_version(unsigned char **version)
{
    INVENSENSE_FUNC_START;

    *version = (unsigned char *)mlVer;  //fixme we are wiping const

    return INV_SUCCESS;
}

/**
 * @brief Check for the presence of the gyro sensor.
 *
 * This is not a physical check but a logical check and the value can change
 * dynamically based on calls to inv_set_mpu_sensors().
 *
 * @return  true if the gyro is enabled false otherwise.
 */
int inv_get_gyro_present(void)
{
    return inv_get_dl_config()->inv_mpu_cfg->requested_sensors &
        (INV_X_GYRO | INV_Y_GYRO | INV_Z_GYRO);
}

static unsigned short inv_row_2_scale(const signed char *row)
{
    unsigned short b;

    if (row[0] > 0)
        b = 0;
    else if (row[0] < 0)
        b = 4;
    else if (row[1] > 0)
        b = 1;
    else if (row[1] < 0)
        b = 5;
    else if (row[2] > 0)
        b = 2;
    else if (row[2] < 0)
        b = 6;
    else
        b = 7;                  // error
    return b;
}

unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx)
{
    unsigned short scalar;
    /*
       XYZ  010_001_000 Identity Matrix
       XZY  001_010_000
       YXZ  010_000_001
       YZX  000_010_001
       ZXY  001_000_010
       ZYX  000_001_010
     */

    scalar = inv_row_2_scale(mtx);
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;

    return scalar;
}

/* Setups up the Freescale 16-bit accel for Sensor Fusion
* @param[in] orient A scalar representation of the orientation
*  that describes how to go from the Chip Orientation
*  to the Board Orientation often times called the Body Orientation in Invensense Documentation.
*  inv_orientation_matrix_to_scalar() will turn the transformation matrix into this scalar.
*/
inv_error_t inv_freescale_sensor_fusion_16bit()
{
    unsigned char rr[3];
    inv_error_t result;
    unsigned short orient;

    orient = inv_accel_orient & 0xdb;
    switch (orient) {
    default:
        // Typically 0x88
        rr[0] = DINACC;
        rr[1] = DINACF;
        rr[2] = DINA0E;
        break;
    case 0x50:
        rr[0] = DINACE;
        rr[1] = DINA0E;
        rr[2] = DINACD;
        break;
    case 0x81:
        rr[0] = DINACE;
        rr[1] = DINACB;
        rr[2] = DINA0E;
        break;
    case 0x11:
        rr[0] = DINACC;
        rr[1] = DINA0E;
        rr[2] = DINACB;
        break;
    case 0x42:
        rr[0] = DINA0A;
        rr[1] = DINACF;
        rr[2] = DINACB;
        break;
    case 0x0a:
        rr[0] = DINA0A;
        rr[1] = DINACB;
        rr[2] = DINACD;
        break;
    }
    result = inv_set_mpu_memory(KEY_FCFG_AZ, 3, rr);
    return result;
}

/* Setups up the Freescale 8-bit accel for Sensor Fusion
* @param[in] orient A scalar representation of the orientation
*  that describes how to go from the Chip Orientation
*  to the Board Orientation often times called the Body Orientation in Invensense Documentation.
*  inv_orientation_matrix_to_scalar() will turn the transformation matrix into this scalar.
*/
inv_error_t inv_freescale_sensor_fusion_8bit()
{
    unsigned char regs[27];
    inv_error_t result;
    uint_fast8_t kk;

    kk = 0;

    regs[kk++] = DINAC3;
    regs[kk++] = DINA90 + 14;
    regs[kk++] = DINAA0 + 9;
    regs[kk++] = DINA3E;
    regs[kk++] = DINA5E;
    regs[kk++] = DINA7E;

    regs[kk++] = DINAC2;
    regs[kk++] = DINAA0 + 9;
    regs[kk++] = DINA90 + 9;
    regs[kk++] = DINAF8 + 2;

    switch (inv_accel_orient & 0xdb) {
    default:
        // Typically 0x88
        regs[kk++] = DINACB;

        regs[kk++] = DINA54;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;

        regs[kk++] = DINACD;
        break;
    case 0x50:
        regs[kk++] = DINACB;

        regs[kk++] = DINACF;

        regs[kk++] = DINA7C;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        break;
    case 0x81:
        regs[kk++] = DINA2C;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;

        regs[kk++] = DINACD;

        regs[kk++] = DINACB;
        break;
    case 0x11:
        regs[kk++] = DINA2C;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINACB;
        regs[kk++] = DINACF;
        break;
    case 0x42:
        regs[kk++] = DINACF;
        regs[kk++] = DINACD;

        regs[kk++] = DINA7C;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        break;
    case 0x0a:
        regs[kk++] = DINACD;

        regs[kk++] = DINA54;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;

        regs[kk++] = DINACF;
        break;
    }

    regs[kk++] = DINA90 + 7;
    regs[kk++] = DINAF8 + 3;
    regs[kk++] = DINAA0 + 9;
    regs[kk++] = DINA0E;
    regs[kk++] = DINA0E;
    regs[kk++] = DINA0E;

    regs[kk++] = DINAF8 + 1;    // filler

    result = inv_set_mpu_memory(KEY_FCFG_FSCALE, kk, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

/**
 * Controlls each sensor and each axis when the motion processing unit is
 * running.  When it is not running, simply records the state for later.
 *
 * NOTE: In this version only full sensors controll is allowed.  Independent
 * axis control will return an error.
 *
 * @param sensors Bit field of each axis desired to be turned on or off
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_set_mpu_sensors(unsigned long sensors)
{
    INVENSENSE_FUNC_START;
    unsigned char state = inv_get_state();
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    inv_error_t result = INV_SUCCESS;

    if (state < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (((sensors & INV_THREE_AXIS_ACCEL) != INV_THREE_AXIS_ACCEL) &&
        ((sensors & INV_THREE_AXIS_ACCEL) != 0)) {
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    }
    if (((sensors & INV_THREE_AXIS_ACCEL) != 0) &&
        (!mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_ACCEL])) {
        return INV_ERROR_SERIAL_DEVICE_NOT_RECOGNIZED;
    }

    if (((sensors & INV_THREE_AXIS_COMPASS) != INV_THREE_AXIS_COMPASS) &&
        ((sensors & INV_THREE_AXIS_COMPASS) != 0)) {
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    }
    if (((sensors & INV_THREE_AXIS_COMPASS) != 0) &&
        (!mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_COMPASS])) {
        return INV_ERROR_SERIAL_DEVICE_NOT_RECOGNIZED;
    }

    if (((sensors & INV_THREE_AXIS_PRESSURE) != INV_THREE_AXIS_PRESSURE) &&
        ((sensors & INV_THREE_AXIS_PRESSURE) != 0)) {
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    }
    if (((sensors & INV_THREE_AXIS_PRESSURE) != 0) &&
        (!mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_PRESSURE])) {
        return INV_ERROR_SERIAL_DEVICE_NOT_RECOGNIZED;
    }

    /* DMP was off, and is turning on */
    if (sensors & INV_DMP_PROCESSOR &&
        !(mldl_cfg->inv_mpu_cfg->requested_sensors & INV_DMP_PROCESSOR)) {
        struct ext_slave_config config;
        long odr;
        config.key = MPU_SLAVE_CONFIG_ODR_RESUME;
        config.len = sizeof(long);
        config.apply = !(mldl_cfg->inv_mpu_state->status &
                         MPU_ACCEL_IS_SUSPENDED);
        config.data = &odr;

        odr = (inv_mpu_get_sampling_rate_hz(mldl_cfg->mpu_gyro_cfg) * 1000);
        result = inv_mpu_config_accel(mldl_cfg,
                                      inv_get_serial_handle(),
                                      inv_get_serial_handle(), &config);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        config.key = MPU_SLAVE_CONFIG_IRQ_RESUME;
        odr = MPU_SLAVE_IRQ_TYPE_NONE;
        result = inv_mpu_config_accel(mldl_cfg,
                                      inv_get_serial_handle(),
                                      inv_get_serial_handle(), &config);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        inv_init_fifo_hardare();
    }
    if (IS_INV_ADVFEATURES_ENABLED(inv_obj) &&
        (inv_obj.adv_fusion->mode_change_cb)) {
        result = inv_obj.adv_fusion->mode_change_cb(
            mldl_cfg->inv_mpu_cfg->requested_sensors, sensors);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    if (mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_ACCEL]) {
        unsigned char regs[6];
        unsigned short orient;

        orient = inv_orientation_matrix_to_scalar(
            mldl_cfg->pdata_slave[EXT_SLAVE_TYPE_ACCEL]->orientation);

        if (sensors & INV_THREE_AXIS_ACCEL) {
            if (ACCEL_ID_MPU6050 == mldl_cfg->slave[EXT_SLAVE_TYPE_ACCEL]->id) {
                inv_mpu6050_accel(orient, 0, regs);
                result = inv_set_mpu_memory(KEY_FCFG_2, 6, regs);
            } else {
                if (sensors & INV_THREE_AXIS_COMPASS) {
                    inv_mpu6050_accel(orient, 1, regs);
                    result = inv_set_mpu_memory(KEY_FCFG_2, 6, regs);
                } else {
                    inv_mpu6050_accel(orient, 2, regs);
                    result = inv_set_mpu_memory(KEY_FCFG_2, 6, regs);
                }
            }
        }
    }

    mldl_cfg->inv_mpu_cfg->requested_sensors = sensors;

    /* inv_dmp_start will turn the sensors on */
    if (state == INV_STATE_DMP_STARTED) {
        result = inv_dl_start(sensors);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result = inv_reset_motion();
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result = inv_dl_stop(~sensors);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    /* Set output rate to default FIFO rate */
    if ((sensors & INV_THREE_AXIS_ACCEL) || (sensors & INV_DMP_PROCESSOR))
        inv_set_fifo_rate(0xffff);

    if (!(sensors & INV_DMP_PROCESSOR) && (sensors & INV_THREE_AXIS_ACCEL)) {
        struct ext_slave_config config;
        long data;

        config.len = sizeof(long);
        config.key = MPU_SLAVE_CONFIG_IRQ_RESUME;
        config.apply = !(mldl_cfg->inv_mpu_state->status &
                         MPU_ACCEL_IS_SUSPENDED);
        config.data = &data;
        data = MPU_SLAVE_IRQ_TYPE_DATA_READY;
        result = inv_mpu_config_accel(mldl_cfg,
                                      inv_get_serial_handle(),
                                      inv_get_serial_handle(), &config);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    return result;
}

void inv_set_mode_change(inv_error_t(*mode_change_func)
                         (unsigned long, unsigned long))
{
    inv_obj.adv_fusion->mode_change_cb = mode_change_func;
}

/**
* MPU6050 setup
*/
inv_error_t inv_set_mpu_6050_config(void)
{
    long temp;
    inv_error_t result;
    unsigned char big8[4];
    unsigned char atc[4];
    long s[3], s2[3];
    int kk;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    result = inv_serial_read(inv_get_serial_handle(), inv_get_mpu_slave_addr(),
                             0x0d, 4, atc);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    temp = atc[3] & 0x3f;
    if (temp >= 32)
        temp = temp - 64;
    temp = (temp << 21) | 0x100000;
    temp += (1L << 29);
    temp = -temp;
    result = inv_set_mpu_memory(KEY_D_ACT0, 4, inv_int32_to_big8(temp, big8));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    for (kk = 0; kk < 3; ++kk) {
        s[kk] = atc[kk] & 0x3f;
        if (s[kk] > 32)
            s[kk] = s[kk] - 64;
        s[kk] *= 2516582L;
    }

    for (kk = 0; kk < 3; ++kk) {
        s2[kk] = mldl_cfg->pdata->orientation[kk * 3] * s[0] +
            mldl_cfg->pdata->orientation[kk * 3 + 1] * s[1] +
            mldl_cfg->pdata->orientation[kk * 3 + 2] * s[2];
    }
    result = inv_set_mpu_memory(KEY_D_ACSX, 4, inv_int32_to_big8(s2[0], big8));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_set_mpu_memory(KEY_D_ACSY, 4, inv_int32_to_big8(s2[1], big8));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_set_mpu_memory(KEY_D_ACSZ, 4, inv_int32_to_big8(s2[2], big8));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

/**
 * @}
 */
