#ifndef SUSHI_LIBRARY_AUDIO_GRAPH_ACCESSOR_H
#define SUSHI_LIBRARY_AUDIO_GRAPH_ACCESSOR_H

#include "engine/audio_graph.h"

namespace sushi::internal::engine
{

class AudioGraphAccessor
{
public:
    explicit AudioGraphAccessor(AudioGraph& f) : _friend(f) {}

    [[nodiscard]] const std::vector<std::vector<Track*>>& audio_graph()
    {
        return _friend._audio_graph;
    }

private:
    AudioGraph& _friend;
};

} // end namespace sushi::internal::engine

#endif //SUSHI_LIBRARY_AUDIO_GRAPH_ACCESSOR_H
