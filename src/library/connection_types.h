/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @Brief Datatypes and associated functions for managine audio and cv/cate connections
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_CONNECTION_TYPES_H
#define SUSHI_CONNECTION_TYPES_H

#include "id_generator.h"

namespace sushi {

/**
 * @brief Data for routing Audio to and from tracks
 */
struct AudioConnection
{
    int      engine_channel;
    int      track_channel;
    ObjectId track;
};

/**
 * @brief Data for routing control voltage to and from processor parameters
 */
struct CvConnection
{
    ObjectId processor_id;
    ObjectId parameter_id;
    int      cv_id;
};

/**
 * @brief Data for routing gate events to and from processors
 */
struct GateConnection
{
    ObjectId processor_id;
    int      gate_id;
    int      note_no;
    int      channel;
};

bool inline operator==(const AudioConnection& l, const AudioConnection& r)
{
    return l.engine_channel == r.engine_channel &&
           l.track_channel  == r.track_channel &&
           l.track          == r.track;
}

bool inline operator==(const CvConnection& l, const CvConnection& r)
{
    return l.processor_id == r.processor_id &&
           l.parameter_id == r.parameter_id &&
           l.cv_id        == r.cv_id;
}

bool inline operator==(const GateConnection& l, const GateConnection& r)
{
    return l.processor_id == r.processor_id &&
           l.gate_id      == r.gate_id &&
           l.note_no      == r.note_no &&
           l.channel      == r.channel;
}

} // namespace sushi
#endif //SUSHI_CONNECTION_TYPES_H
