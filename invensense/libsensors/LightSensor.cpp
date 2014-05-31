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

#include "LightSensor.h"

/*
 * The BMP driver gives pascal values.
 * It needs to be changed into hectoPascal
 */
#define PRESSURE_HECTO (1.0f/100.0f)

LightSensor::LightSensor()
    : SamsungSensorBase(NULL, "light", NULL)
{
    mPendingEvent[Light].sensor = ID_L;
    mPendingEvent[Light].type = SENSOR_TYPE_LIGHT;
	mSensors[Light] = ABS_X;
	setSensors(mSensors);
}

bool LightSensor::handleEvent(input_event const *event) {
	if (event->code == ABS_X) {
	    mPendingEvent[Light].light = (float)(event->value * 1.0f);
	}
    return true;
}
