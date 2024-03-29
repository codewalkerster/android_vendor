/*
 * Copyright 2010 Ericsson AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Indrek Peri <Indrek.Peri@ericsson.com>  */

#ifndef _LIBMBMGPS_LOG_H
#define _LIBMBMGPS_LOG_H 1

#ifndef LOG_TAG
#define  LOG_TAG  "libmbm-gps"
#endif

#include <cutils/log.h>

#ifdef DEBUG
#define ENTER ALOGV("%s: enter", __FUNCTION__)
#define EXIT ALOGV("%s: exit", __FUNCTION__)
#else
#define ENTER
#define EXIT
#endif

#endif                          /* end _LIBMBMGPS_LOG_H_ */
