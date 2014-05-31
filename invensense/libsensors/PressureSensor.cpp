/*
 * Copyright (C) 2011 Samsung
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <pthread.h>

#include "PressureSensor.h"

/*
 * The BMP driver gives pascal values.
 * It needs to be changed into hectoPascal
 */
#define PRESSURE_HECTO (1.0f/100.0f)

PressureSensor::PressureSensor()
    : SamsungSensorBase(NULL, BMP180_INPUT_NAME, NULL)
{
    mPendingEvent[Pressure].sensor = ID_PR;
    mPendingEvent[Pressure].type = SENSOR_TYPE_PRESSURE;
#ifdef WITH_AMBIENT_TEMPERATURE
    mPendingEvent[Temperature].sensor = ID_T;
    mPendingEvent[Temperature].type = SENSOR_TYPE_AMBIENT_TEMPERATURE;
#ifdef A4
	mSensors[Pressure] = ABS_X;
#else
	mSensors[Pressure] = ABS_PRESSURE;
#endif
#endif
#ifdef WITH_AMBIENT_TEMPERATURE
#ifdef A4
	mSensors[Temperature] = ABS_Y;
#else
	mSensors[Temperature] = ABS_X;
#endif
#endif
	setSensors(mSensors);
}

bool PressureSensor::handleEvent(input_event const *event) {
#ifdef A4
	if (event->code == ABS_X)
#else
	if (event->code == ABS_PRESSURE)
#endif
	    mPendingEvent[Pressure].pressure = event->value * PRESSURE_HECTO;
#ifdef WITH_AMBIENT_TEMPERATURE
#ifdef A4
	else if (event->code == ABS_Y)
#else
	else if (event->code == ABS_X)
#endif
	    mPendingEvent[Temperature].temperature = event->value * (1.0f/10.0f);
#endif	
    return true;
}
