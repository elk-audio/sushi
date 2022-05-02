/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief No-op version of the Ableton Link interface for use when sushi is
 * not compiled with Link support.
 */
#ifndef SUSHI_LINK_DUMMY_H
#define SUSHI_LINK_DUMMY_H

#include <chrono>

namespace sushi::engine {
class SushiLink
{
public:
  using Clock = int;
  class SessionState;

  explicit SushiLink(double /*bpm*/) {}
  bool isEnabled() const {return false;}
  void enable(bool /*bEnable*/) {}
  bool isStartStopSyncEnabled() const {return false;}
  void enableStartStopSync(bool /*bEnable*/) {}
  std::size_t numPeers() const {return 0;}

  template <typename Callback>
  void setNumPeersCallback(Callback /*callback*/) {}

  template <typename Callback>
  void setTempoCallback(Callback /*callback*/) {}

  template <typename Callback>
  void setStartStopCallback(Callback /*callback*/) {}

  Clock clock() const {return 0;}
  SessionState captureAudioSessionState() const {return {};}
  void commitAudioSessionState(SessionState /*state*/) {}
  SessionState captureAppSessionState() const {return {};}
  void commitAppSessionState(SessionState /*state*/) {}
  class SessionState
  {
  public:
    SessionState() = default;
    double tempo() const {return 120;}
    void setTempo(double /*bpm*/, std::chrono::microseconds /*atTime*/) {}
    double beatAtTime(std::chrono::microseconds /*time*/, double /*quantum*/) const {return 0;}
    double phaseAtTime(std::chrono::microseconds /*time*/, double /*quantum*/) const {return 0;}
    std::chrono::microseconds timeAtBeat(double /*beat*/, double /*quantum*/) const {return std::chrono::seconds(0);}
    void requestBeatAtTime(double /*beat*/, std::chrono::microseconds /*time*/, double /*quantum*/) {}
    void forceBeatAtTime(double /*beat*/, std::chrono::microseconds /*time*/, double /*quantum*/) {}
    void setIsPlaying(bool /*isPlaying*/, std::chrono::microseconds /*time*/) {}
    bool isPlaying() const {return false;}
    std::chrono::microseconds timeForIsPlaying() const {return std::chrono::seconds(0);}
    void requestBeatAtStartPlayingTime(double /*beat*/, double /*quantum*/) {}
    void setIsPlayingAndRequestBeatAtTime(bool /*isPlaying*/, std::chrono::microseconds /*time*/,
                                          double /*beat*/, double /*quantum*/) {}

  private:
    friend SushiLink;
  };
private:
};
} // end namespace sushi::engine


#endif //SUSHI_LINK_DUMMY_H
