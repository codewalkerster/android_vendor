/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: gesture.h 5629 2011-06-11 03:13:08Z mcaramello $
 *
 ******************************************************************************/

/** 
 *  @defgroup INV_GESTURE
 *  @brief    The Gesture Library processes gyroscopes and accelerometers to 
 *            provide recognition of a set of gestures, including tapping, 
 *            shaking along various axes, and rotation about a horizontal axis.
 *
 *   @{
 *       @file mlgesture.h
 *       @brief Header file for the Gesture Library.
 *
**/

#ifndef GESTURE_H
#define GESTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#include "stdint_invensense.h"

    /* ------------ */
    /* - Defines. - */
    /* ------------ */

    /**************************************************************************/
    /* Gesture Types.                                                         */
    /**************************************************************************/

#define INV_PITCH_SHAKE                  0x01
#define INV_ROLL_SHAKE                   0x02
#define INV_YAW_SHAKE                    0x04
#define INV_TAP                          0x08
#define INV_YAW_IMAGE_ROTATE             0x10
#define INV_SHAKE_ALL                    0x07
#define INV_GESTURE_ALL                       \
    INV_PITCH_SHAKE                      | \
    INV_ROLL_SHAKE                       | \
    INV_YAW_SHAKE                        | \
    INV_TAP                              | \
    INV_YAW_IMAGE_ROTATE

    /**************************************************************************/
    /* Shake Functions.                                                       */
    /**************************************************************************/

#define INV_SOFT_SHAKE                   0x0000
#define INV_HARD_SHAKE                   0x0001
#define INV_NO_RETRACTION                0x0000
#define INV_RETRACTION                   0x0002

    /**************************************************************************/
    /* Data Enumerations.                                                     */
    /**************************************************************************/
#define INV_NUM_TAP_AXES (3)

#define INV_TAP_AXIS_X                        0x1
#define INV_TAP_AXIS_Y                        0x2
#define INV_TAP_AXIS_Z                        0x4
#define INV_TAP_AXIS_ALL                      \
    (INV_TAP_AXIS_X                            |   \
     INV_TAP_AXIS_Y                            |   \
     INV_TAP_AXIS_Z)
/** The direction of a tap */
#define     INV_GSTR_TAP_DIRECTION_NO_TAP (0)
#define     INV_GSTR_TAP_DIRECTION_NEGETIVE_X (-1)
#define     INV_GSTR_TAP_DIRECTION_NEGETIVE_Y (-2)
#define     INV_GSTR_TAP_DIRECTION_NEGETIVE_Z (-3)
#define     INV_GSTR_TAP_DIRECTION_X (1)
#define     INV_GSTR_TAP_DIRECTION_Y (2)
#define     INV_GSTR_TAP_DIRECTION_Z (3)

    /**************************************************************************/
    /* Data selection options.                                                */
    /**************************************************************************/

#define INV_GSTR_YAW_ROTATION        0x0000
#define INV_GSTR_DATA_STRUCT         0x0001

    /**************************************************************************/
    /* INV_GSTR_Params_t structure default values.                              */
    /**************************************************************************/

#define INV_GSTR_TAP_THRESH_DEFAULT                           (2046)
#define INV_GSTR_TAP_TIME_DEFAULT                             (40)
#define INV_GSTR_NEXT_TAP_TIME_DEFAULT                        (200)
#define INV_GSTR_MAX_TAPS_DEFAULT                             (3)
#define INV_GSTR_TAP_INTERPOLATION_DEFAULT                    (2)
#define INV_GSTR_SHAKE_MASK_DEFAULT                           (0)
#define INV_GSTR_SHAKE_MAXIMUM_DEFAULT                        (3)
#define INV_GSTR_SHAKE_THRESHOLD_0_DEFAULT                    (4.0)
#define INV_GSTR_SHAKE_THRESHOLD_1_DEFAULT                    (4.0)
#define INV_GSTR_SHAKE_THRESHOLD_2_DEFAULT                    (4.0)
#define INV_GSTR_SNAP_THRESHOLD_0_DEFAULT                     (1000.0)
#define INV_GSTR_SNAP_THRESHOLD_1_DEFAULT                     (1000.0)
#define INV_GSTR_SNAP_THRESHOLD_2_DEFAULT                     (1000.0)
#define INV_GSTR_SHAKE_REJECT_THRESHOLD_0_DEFAULT             (2.0f)
#define INV_GSTR_SHAKE_REJECT_THRESHOLD_1_DEFAULT             (2.0f)
#define INV_GSTR_SHAKE_REJECT_THRESHOLD_2_DEFAULT             (2.0f)
#define INV_GSTR_SHAKE_REJECT_THRESHOLD_3_DEFAULT             (1.500f)
#define INV_GSTR_SHAKE_REJECT_THRESHOLD_4_DEFAULT             (1.501f)
#define INV_GSTR_SHAKE_REJECT_THRESHOLD_5_DEFAULT             (1.502f)
#define INV_GSTR_LINEAR_SHAKE_DEADZONE_0_DEFAULT              (0.1f)
#define INV_GSTR_LINEAR_SHAKE_DEADZONE_1_DEFAULT              (0.1f)
#define INV_GSTR_LINEAR_SHAKE_DEADZONE_2_DEFAULT              (0.1f)
#define INV_GSTR_SHAKE_TIME_DEFAULT                           (160)
#define INV_GSTR_NEXT_SHAKE_TIME_DEFAULT                      (160)
#define INV_GSTR_YAW_ROTATE_THRESHOLD_DEFAULT                 (70.0)
#define INV_GSTR_YAW_ROTATE_TIME_DEFAULT                      (10)
#define INV_GSTR_GESTURE_MASK_DEFAULT                         (0)
#define INV_GSTR_GESTURE_CALLBACK_DEFAULT                     (0)

    /* --------------- */
    /* - Structures. - */
    /* --------------- */

    /**************************************************************************/
    /* Gesture Structure                                                      */
    /**************************************************************************/

    /** Gesture Description */
    typedef struct  {

        unsigned short type;
        short          strength;
        short          speed;
        unsigned short num;
        short          meta;
        short          reserved;

    }   tGesture,               // new type definition
        tGestureShake,          // Shake structure
        tGestureTap,            // Tap structure
        tGestureYawImageRotate, // Yaw Image Rotate Structure
        gesture_t;              // backward-compatible definition

    /**************************************************************************/
    /* Gesture Parameters Structure.                                          */
    /**************************************************************************/

    typedef struct {

        unsigned short tapThresh[INV_NUM_TAP_AXES];// The threshold for detecting a tap.
        unsigned short tapTime;            // The delay before a tap will be registered.
        unsigned short nextTapTime;        // The time interval required for the tap number to increase.
        unsigned short maxTaps;            // The max taps to record before reporting and resetting the count
        unsigned int   tapInterpolation;
        unsigned long  tapElements;
        unsigned short shakeMask;          // The shake detection functions.
        unsigned int   shakeMax[3];        // Pitch, Roll, and Yaw axis shake detection maximums
        float          shakeThreshold[3];           // Pitch, Roll, and Yaw axis shake detection thresholds.
        float          snapThreshold[3];           // Pitch, Roll, and Yaw axis shake detection thresholds.
        float          shakeRejectThreshold[6];     // Pitch, Roll, and Yaw axis shake detection thresholds.
        float          linearShakeDeadzone[3];     // Pitch, Roll, and Yaw axis shake detection thresholds.
        unsigned short shakeTime;          // The delay before a shake will be registered.
        unsigned short nextShakeTime;      // The time interval required for the shake number to increase.

        float          yawRotateThreshold;          // The threshold for detecting a yaw image rotation.
        unsigned short yawRotateTime;      // The time threshold for detecting a yaw image rotation.

        unsigned short gestureMask;        // A gesture or bitwise OR of gestures to be detected.
        void (*gestureCallback)(           // User defined callback function that will be run when a gesture is detected.
            tGesture *gesture);            // Gesture data structure.
        void (*gesturePedometerCallback)(  // Pedometer callback function that will be run when a gesture is detected.
            tGesture *gesture);            // Gesture data structure.
        int_fast16_t   suspend;            // Used to turn off gesture engine

    }   tMLGstrParams,      // new type definition
        INV_GSTR_Params_t;    // backward-compatible definition


    /* --------------------- */
    /* - Function p-types. - */
    /* --------------------- */

    /**************************************************************************/
    /* ML Gesture Functions                                                   */
    /**************************************************************************/


    /*API For detecting tapping*/
    int inv_set_tap_threshold(unsigned int axis, unsigned short threshold);
    /* Deprecated.  Use inv_set_tap_threshold */
#define MLSetTapThresh(threshold) \
        inv_set_tap_threshold(INV_TAP_AXIS_X | INV_TAP_AXIS_Y | INV_TAP_AXIS_Z, \
                             threshold)
    int inv_set_next_tap_time(unsigned short time);
    int MLSetNextTapTime(unsigned short time);
    int inv_set_max_taps(unsigned short max);
    int inv_reset_tap(void);
    int inv_set_tap_shake_reject(float value);

    inv_error_t MLSetTapInterrupt(unsigned char on);

    /*API for detecting shaking*/
    int inv_set_shake_func(unsigned short function);
    int inv_set_shake_thresh(unsigned short axis, unsigned short threshold);
    int inv_set_hard_shake_thresh(unsigned short axis, unsigned short threshold);
    int inv_set_shake_time(unsigned short time);
    int inv_set_next_shake_time(unsigned short time);
    int inv_set_max_shakes(int axis, int max);
    int inv_reset_shake(int axis);

    inv_error_t inv_enable_shake_pitch_interrupt(unsigned char on);
    inv_error_t inv_enable_shake_roll_interrupt(unsigned char on);
    inv_error_t inv_enable_shake_yaw_interrupt(unsigned char on);

    /*API for detecting yaw image rotation*/
    int inv_set_yaw_rotate_thresh(unsigned short threshold);
    int inv_set_yaw_rotate_time(unsigned short time);
    int inv_get_yaw_rotation();

    /*API for registering gestures to be detected*/
    int inv_set_gesture(unsigned short gestures);
    int inv_enable_gesture();
    int inv_disable_gesture();
    int inv_set_gesture_cb(void (*callback)(tGesture *gesture) );
    int inv_get_gesture(tGesture *gesture);
    int inv_get_gestureState(int *state);
    int inv_set_gesture_pedometer_cb( void (*callback)(tGesture *gesture) );
    int inv_disable_gesturePedometer();

    inv_error_t inv_gesture_tap_set_quantized(void);
#ifdef __cplusplus
}
#endif

#endif // GESTURE_H
