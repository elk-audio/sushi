#ifndef SUSHI_LIBRARY_TRACK_ACCESSOR_H
#define SUSHI_LIBRARY_TRACK_ACCESSOR_H

#include "engine/track.h"

namespace sushi::internal::engine {

class TrackAccessor
{
public:
    explicit TrackAccessor(Track& plugin) : _plugin(plugin) {}

    [[nodiscard]] const std::vector<Processor*>& processors()
    {
        return _plugin._processors;
    }

private:
    Track& _plugin;
};

}

#endif //SUSHI_LIBRARY_TRACK_ACCESSOR_H
