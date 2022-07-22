/**
 * @brief Mock object for ProcessorContainer
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_MOCK_PROCESSOR_CONTAINER_H
#define SUSHI_MOCK_PROCESSOR_CONTAINER_H

#include <gmock/gmock.h>

#include "engine/base_processor_container.h"

using namespace sushi;
using namespace sushi::engine;

class MockProcessorContainer : public engine::BaseProcessorContainer
{
public:
    MOCK_METHOD(bool,
                add_processor,
                (std::shared_ptr<Processor>),
                (override));

    MOCK_METHOD(bool,
                add_track,
                (std::shared_ptr<Track>),
                (override));

    MOCK_METHOD(bool,
                remove_processor,
                (ObjectId),
                (override));

    MOCK_METHOD(bool,
                remove_track,
                (ObjectId),
                (override));

    MOCK_METHOD(bool,
                add_to_track,
                (std::shared_ptr<Processor>, ObjectId, std::optional<ObjectId>),
                (override));

    MOCK_METHOD(bool,
                remove_from_track,
                (ObjectId, ObjectId),
                (override));

    MOCK_METHOD(bool,
                processor_exists,
                (ObjectId),
                (const override));

    MOCK_METHOD(bool,
                processor_exists,
                (const std::string&),
                (const override));

    MOCK_METHOD(std::vector<std::shared_ptr<const Processor>>,
                all_processors,
                (),
                (const override));

    MOCK_METHOD(std::vector<std::shared_ptr<const Processor>>,
                all_processors,
                (std::shared_ptr<Track>),
                (const override));

    MOCK_METHOD(std::shared_ptr<const Processor>,
                processor,
                (ObjectId),
                (const override));

    MOCK_METHOD(std::shared_ptr<const Processor>,
                processor,
                (const std::string& ),
                (const override));

    MOCK_METHOD(std::shared_ptr<Processor>,
                mutable_processor,
                (ObjectId),
                (const override));

    MOCK_METHOD(std::shared_ptr<Processor>,
                mutable_processor,
                (const std::string& ),
                (const override));

    MOCK_METHOD(std::shared_ptr<const Track>,
                track,
                (ObjectId),
                (const override));

    MOCK_METHOD(std::shared_ptr<const Track>,
                track,
                (const std::string&),
                (const override));

    MOCK_METHOD(std::shared_ptr<Track>,
                mutable_track,
                (ObjectId),
                (const override));

    MOCK_METHOD(std::shared_ptr<Track>,
                mutable_track,
                (const std::string&),
                (const override));

    MOCK_METHOD(std::vector<std::shared_ptr<const Processor>>,
                processors_on_track,
                (ObjectId),
                (const override));

    MOCK_METHOD(std::vector<std::shared_ptr<const Track>>,
                all_tracks,
                (),
                (const override));

};

#endif //SUSHI_MOCK_PROCESSOR_CONTAINER_H
