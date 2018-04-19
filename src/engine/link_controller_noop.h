/**
 * @Brief Empty, dummy version of Abletons Link Interface for conveniently disabling functionality
 */

#ifndef SUSHI_LINK_CONTROLLER_NOOP_H
#define SUSHI_LINK_CONTROLLER_NOOP_H

#include <chrono>

namespace ableton
{

class Clock
{
public:
    std::chrono::microseconds micros() const
    {
        return std::chrono::microseconds(0);
    }
};

class Link
{
public:
    class SessionState;

    Link(double /*bpm*/) {}

    bool isEnabled() const {return false;}

    void enable(bool /*bEnable*/) {}

    bool isStartStopSyncEnabled() const;

    void enableStartStopSync(bool /*bEnable*/) {}

    std::size_t numPeers() const {return 0;}

    template <typename Callback>
    void setNumPeersCallback(Callback /*callback*/) {}

    template <typename Callback>
    void setTempoCallback(Callback /*callback*/) {}

    template <typename Callback>
    void setStartStopCallback(Callback /*callback*/) {}

    Clock clock() const {return Clock();}

    SessionState captureAudioSessionState() const {return SessionState();}

    void commitAudioSessionState(SessionState /*state*/) {}

    SessionState captureAppSessionState() const {return SessionState();}

    void commitAppSessionState(SessionState /*state*/) {}

    class SessionState
    {
    public:
        SessionState() {}

        double tempo() const {return 1;}

        void setTempo(double /*bpm*/, std::chrono::microseconds /*atTime*/) {}

        double beatAtTime(std::chrono::microseconds /*time*/, double /*quantum*/) const {return 0;}

        double phaseAtTime(std::chrono::microseconds /*time*/, double /*quantum*/) const {return 0;}

        std::chrono::microseconds timeAtBeat(double /*beat*/, double /*quantum*/) const {return std::chrono::microseconds(0);}

        void requestBeatAtTime(double /*beat*/, std::chrono::microseconds /*time*/, double /*quantum*/) {}

        void forceBeatAtTime(double /*beat*/, std::chrono::microseconds /*time*/, double /*quantum*/) {}

        void setIsPlaying(bool /*isPlaying*/, std::chrono::microseconds /*time*/) {}

        bool isPlaying() const {return false;}

        std::chrono::microseconds timeForIsPlaying() const {return std::chrono::microseconds(0);}

        void requestBeatAtStartPlayingTime(double /*beat*/, double /*quantum*/) {}

        void setIsPlayingAndRequestBeatAtTime(
                bool /*isPlaying*/, std::chrono::microseconds /*time*/, double /*beat*/, double /*quantum*/);
    };
};

} // ableton

#endif //SUSHI_LINK_CONTROLLER_NOOP_H
