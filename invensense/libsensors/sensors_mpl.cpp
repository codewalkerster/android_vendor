/*
 * Copyright (C) 2011 Invensense, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "Sensors"
#define FUNC_LOG ALOGV("%s", __PRETTY_FUNCTION__)

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"
#include "MPLSensor.h"

#include "MPLSensorSysApi.h"

// ADD HardKernel
#include "LightSensor.h"
#include "PressureSensor.h"

/*****************************************************************************/

#define DELAY_OUT_TIME 0x7FFFFFFF

#define LIGHT_SENSOR_POLLTIME    2000000000

#define SENSORS_ROTATION_VECTOR  (1<<ID_RV)
#define SENSORS_LINEAR_ACCEL     (1<<ID_LA)
#define SENSORS_GRAVITY          (1<<ID_GR)
#define SENSORS_GYROSCOPE        (1<<ID_GY)
#define SENSORS_ACCELERATION     (1<<ID_A)
#define SENSORS_MAGNETIC_FIELD   (1<<ID_M)
#define SENSORS_ORIENTATION      (1<<ID_O)
// ADD Hardkerel
#define SENSORS_LIGHT            (1<<ID_L)
#define SENSORS_PROXIMITY        (1<<ID_P)
#ifdef BOARD_HAVE_BMP180
#define SENSORS_PRESSURE         (1<<ID_PR)
#ifdef WITH_AMBIENT_TEMPERATURE
#define SENSORS_TEMPERATURE      (1<<ID_T)
#endif
#endif
//

#define SENSORS_ROTATION_VECTOR_HANDLE  (ID_RV)
#define SENSORS_LINEAR_ACCEL_HANDLE     (ID_LA)
#define SENSORS_GRAVITY_HANDLE          (ID_GR)
#define SENSORS_GYROSCOPE_HANDLE        (ID_GY)
#define SENSORS_ACCELERATION_HANDLE     (ID_A)
#define SENSORS_MAGNETIC_FIELD_HANDLE   (ID_M)
#define SENSORS_ORIENTATION_HANDLE      (ID_O)
// ADD Hardkernel
#define SENSORS_LIGHT_HANDLE            (ID_L)
#define SENSORS_PROXIMITY_HANDLE        (ID_P)
#ifdef BOARD_HAVE_BMP180
#define SENSORS_PRESSURE_HANDLE         (ID_PR)
#ifdef WITH_AMBIENT_TEMPERATURE
#define SENSORS_TEMPERATURE_HANDLE      (ID_T)
#endif
#endif
//

#define AKM_FTRACE 0
#define AKM_DEBUG 0
#define AKM_DATA 0

/*****************************************************************************/

/* The SENSORS Module */
static const struct sensor_t sSensorList[] = {
      { "MPL rotation vector",
        "Invensense",
        1, SENSORS_ROTATION_VECTOR_HANDLE,
        SENSOR_TYPE_ROTATION_VECTOR, 10240.0f, 1.0f, 0.5f, 10000,{ } },
      { "MPL linear accel",
        "Invensense",
        1, SENSORS_LINEAR_ACCEL_HANDLE,
        SENSOR_TYPE_LINEAR_ACCELERATION, 10240.0f, 1.0f, 0.5f, 10000,{ } },
      { "MPL gravity",
        "Invensense",
        1, SENSORS_GRAVITY_HANDLE,
        SENSOR_TYPE_GRAVITY, 10240.0f, 1.0f, 0.5f, 10000,{ } },
      { "MPL Gyro",
        "Invensense",
        1, SENSORS_GYROSCOPE_HANDLE,
        SENSOR_TYPE_GYROSCOPE, 10240.0f, 1.0f, 0.5f, 10000,{ } },
      { "MPL accel",
        "Invensense",
        1, SENSORS_ACCELERATION_HANDLE,
        SENSOR_TYPE_ACCELEROMETER, 10240.0f, 1.0f, 0.5f, 10000,{ } },
      { "MPL magnetic field",
        "Invensense",
        1, SENSORS_MAGNETIC_FIELD_HANDLE,
        SENSOR_TYPE_MAGNETIC_FIELD, 10240.0f, 1.0f, 0.5f, 10000,{ } },
      { "MPL Orientation (android deprecated format)",
        "Invensense",
        1, SENSORS_ORIENTATION_HANDLE,
        SENSOR_TYPE_ORIENTATION, 360.0f, 1.0f, 9.7f, 10000,{ } },
	// ADD Hardkernel
      { "Ambient Light sensor",
        "ROHM",
        1, SENSORS_LIGHT_HANDLE,
        SENSOR_TYPE_LIGHT, 65535.0f, 1.0f, 0.2f, 10000, { } },
#ifdef BOARD_HAVE_BMP180
	  { "BMP180 Pressure sensor",
          "Bosch",
          1, SENSORS_PRESSURE_HANDLE,
          SENSOR_TYPE_PRESSURE, 1100.0f, 0.01f, 0.67f, 20000, { } },
#ifdef WITH_AMBIENT_TEMPERATURE
	  { "BMP180 Temperature sensor",
          "Bosch",
          1, SENSORS_TEMPERATURE_HANDLE,
          SENSOR_TYPE_AMBIENT_TEMPERATURE, 600.0f, 1.0f, 0.67f, 20000, { } },
#endif
#endif
};


static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);


static int sensors__get_sensors_list(struct sensors_module_t* module,
                                     struct sensor_t const** list)
{
    *list = sSensorList;
    return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
        open: open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
        common: {
                tag: HARDWARE_MODULE_TAG,
                version_major: 1,
                version_minor: 0,
                id: SENSORS_HARDWARE_MODULE_ID,
                name: "Invensense module",
                author: "Invensense Inc.",
                methods: &sensors_module_methods,
        },
        get_sensors_list: sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);

private:
    enum {
        mpl               = 0,  //all mpl entries must be consecutive and in this order
        mpl_accel,
        mpl_timer,
        light,					// ADD Hardkernel
#ifdef BOARD_HAVE_BMP180
        pressure,				// ADD Hardkernel
#ifdef WITH_AMBIENT_TEMPERATURE
        temperature,			// ADD Hardkernel
#endif
#endif
        numSensorDrivers,       // wake pipe goes here
        mpl_power,              //special handle for MPL pm interaction
        numFds,
    };

    static const size_t wake = numFds - 2;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_RV:
            case ID_LA:
            case ID_GR:
            case ID_GY:
            case ID_A:
            case ID_M:
            case ID_O:
                return mpl;
            case ID_L:			// ADD Hardkernel
                return light;
#ifdef BOARD_HAVE_BMP180
			case ID_PR:
				return pressure;
#ifdef WITH_AMBIENT_TEMPERATURE
			case ID_T:
				return temperature;
#endif
#endif
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    FUNC_LOG;
    MPLSensor *p_mplsen = new MPLSensorSysApi();
    
    // setup the callback object for handing mpl callbacks
    setCallbackObject(p_mplsen); 

    mSensors[mpl] = p_mplsen;
    mPollFds[mpl].fd = mSensors[mpl]->getFd();
    mPollFds[mpl].events = POLLIN;
    mPollFds[mpl].revents = 0;

    mSensors[mpl_accel] = mSensors[mpl];
    mPollFds[mpl_accel].fd = ((MPLSensor*)mSensors[mpl])->getAccelFd();
    mPollFds[mpl_accel].events = POLLIN;
    mPollFds[mpl_accel].revents = 0;

    mSensors[mpl_timer] = mSensors[mpl];
    mPollFds[mpl_timer].fd = ((MPLSensor*)mSensors[mpl])->getTimerFd();
    mPollFds[mpl_timer].events = POLLIN;
    mPollFds[mpl_timer].revents = 0;

	// ADD Hardkernel
    mSensors[light] = new LightSensor();
    mPollFds[light].fd = mSensors[light]->getFd();
    mPollFds[light].events = POLLIN;
    mPollFds[light].revents = 0;

#ifdef BOARD_HAVE_BMP180
	// ADD Hardkernel
    mSensors[pressure] = new PressureSensor();
    mPollFds[pressure].fd = mSensors[pressure]->getFd();
    mPollFds[pressure].events = POLLIN;
    mPollFds[pressure].revents = 0;

#ifdef WITH_AMBIENT_TEMPERATURE
	// ADD Hardkernel
    mSensors[temperature] = mSensors[pressure];
    mPollFds[temperature].fd = mSensors[pressure]->getFd();
    mPollFds[temperature].events = POLLIN;
    mPollFds[temperature].revents = 0;
#endif
#endif

    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;

    //setup MPL pm interaction handle
    mPollFds[mpl_power].fd = ((MPLSensor*)mSensors[mpl])->getPowerFd();
    mPollFds[mpl_power].events = POLLIN;
    mPollFds[mpl_power].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t()
{
    FUNC_LOG;
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled)
{
    FUNC_LOG;
    int index = handleToDriver(handle);

    if (index < 0) return index;
    int err =  mSensors[index]->enable(handle, enabled);
    if (!err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns)
{
    FUNC_LOG;
    int index = handleToDriver(handle);

    if (index < 0) return index;
    return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    //FUNC_LOG;
    int nbEvents = 0;
    int n = 0;
    int polltime = -1;

    do {
        // see if we have some leftover from the last poll()
        for (int i=0 ; count && i<numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                count -= nb;
                nbEvents += nb;
                data += nb;

                //special handling for the mpl, which has multiple handles
                if(i==mpl) {
                    i+=2; //skip accel and timer
                    mPollFds[mpl_accel].revents = 0;
                    mPollFds[mpl_timer].revents = 0;
                }
                if(i==mpl_accel) {
                    i+=1; //skip timer
                    mPollFds[mpl_timer].revents = 0;
                }
            }
        }

        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            int i;

            n = poll(mPollFds, numFds, nbEvents ? 0 : polltime);
            if (n<0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                ALOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
            if(mPollFds[mpl_power].revents & POLLIN) {
                ((MPLSensor*)mSensors[mpl])->handlePowerEvent();
                mPollFds[mpl_power].revents = 0;
            }
        }
        // if we have events and space, go read them
    } while (n && count);

    return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
                          int handle, int enabled)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
                          int handle, int64_t ns)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
                      sensors_event_t* data, int count)
{
    //FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
    FUNC_LOG;
    int status = -EINVAL;
    sensors_poll_context_t *dev = new sensors_poll_context_t();

    memset(&dev->device, 0, sizeof(sensors_poll_device_t));

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = 0;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;

    *device = &dev->device.common;
    status = 0;

    return status;
}


