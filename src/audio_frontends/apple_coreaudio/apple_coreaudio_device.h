/*
* Copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk
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
* @brief C++ representation of the AudioObject as used in the CoreAudio apis.
* @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/

#ifndef SUSHI_APPLE_COREAUDIO_DEVICE_H
#define SUSHI_APPLE_COREAUDIO_DEVICE_H


#include "logging.h"
#include "apple_coreaudio_object.h"

namespace apple_coreaudio {

/**
 * This class represents a CoreAudio device as a thin C++ wrapper around AudioHardware.h APIs.
 */
class AudioDevice : public AudioObject
{
public:
    enum class Scope
    {
        UNDEFINED = 0,
        INPUT,
        OUTPUT,
        INPUT_OUTPUT,
    };

    /**
     * Baseclass for other classes who want to receive the audio callbacks from this device.
     */
    class AudioCallback
    {
    public:
        /**
         * Called when the device needs new audio data.
         * @param scope The scope of this callback.
         * @param now Current time.
         * @param input_data The audio input data.
         * @param input_time The time of the first sample of the input data.
         * @param output_data The audio output data.
         * @param output_time The time of the first sample of the output data.
         */
        virtual void audio_callback([[maybe_unused]] Scope scope,
                                    [[maybe_unused]] const AudioTimeStamp* now,
                                    [[maybe_unused]] const AudioBufferList* input_data,
                                    [[maybe_unused]] const AudioTimeStamp* input_time,
                                    [[maybe_unused]] AudioBufferList* output_data,
                                    [[maybe_unused]] const AudioTimeStamp* output_time) {}

        /**
         * Called when the device changed its sample rate.
         * Warning! This call gets made from a random background thread, and there is no synchronisation or whatsoever.
         * @param new_sample_rate The new sample rate of the device.
         */
        virtual void sample_rate_changed([[maybe_unused]] double new_sample_rate) {}
    };

    explicit AudioDevice(AudioObjectID audio_object_id) : AudioObject(audio_object_id) {}

    virtual ~AudioDevice()
    {
        stop_io();
    }

    SUSHI_DECLARE_NON_COPYABLE(AudioDevice)

    AudioDevice(AudioDevice&& other) noexcept : AudioObject(0)
    {
        *this = std::move(other); // Call into move assignment operator.
    }

    AudioDevice& operator=(AudioDevice&& other) noexcept
    {
        // Since we're going to adopt another AudioDeviceID we must stop any audio IO proc.
        stop_io();

        // Don't transfer ownership of _io_proc_id because CoreAudio has registered the pointer
        // to other as client data, so let other stop the callbacks when it goes out of scope.
        _scope = other._scope;
        AudioObject::operator=(std::move(other));
        return *this;
    }

    /**
     * Starts IO on this device.
     * @return True if successful, or false if an error occurred.
     */
    bool start_io(AudioCallback* audio_callback, Scope for_scope);

    /**
     * Stops IO on this device.
     * @return True if successful, or false if an error occurred.
     */
    bool stop_io();

    /**
     * @return The name of the device.
     */
    [[nodiscard]] std::string get_name() const;

    /**
     * Returns the UID of this device. The UID is persistent across system boots and cannot be shared with other systems.
     * For more information, read the documentation of kAudioDevicePropertyDeviceUID inside AudioHardware.h
     * @return The UID of the device.
     */
    [[nodiscard]] std::string get_uid() const;

    /**
     * @param for_input When true the amount of channels for the input will be returned, when false the amount
     * of channels for the output will be returned.
     * @return The amount of channels for the input or output.
     */
    [[nodiscard]] int get_num_channels(bool for_input) const;

    /**
     * @param buffer_frame_size The number of frames in the io buffers.
     * @return True if succeeded, or false if buffer frame size could not be set.
     */
    [[nodiscard]] bool set_buffer_frame_size(uint32_t buffer_frame_size) const;

    /**
     * Sets the sample rate of this device
     * @param sample_rate The new sample rate. Apple's API seems to accept a value with a max deviation of 0.000000000001.
     * @return True if setting the sample rate succeeded, or false if an error occurred.
     */
    [[nodiscard]] bool set_nominal_sample_rate(double sample_rate) const;

    /**
     * Gets the nominal sample rate of this device.
     * @return The nominal sample rate, or 0.0 if an error occurred.
     */
    [[nodiscard]] double get_nominal_sample_rate() const;

    /**
     * @param for_input True for input or false for output.
     * @return The device latency in samples. Note that stream latency must be added to this number in order to get the total latency.
     */
    [[nodiscard]] UInt32 get_device_latency(bool for_input) const;

    /**
     * @param stream_index The index of the stream to get the latency for.
     * @return The latency of the stream for given index in samples, or 0 if the stream for index does not exist.
     */
    [[nodiscard]] UInt32 get_stream_latency(UInt32 stream_index, bool for_input) const;

    /**
     * @return An UInt32 whose value indicates to which clock domain this device belongs.
     * All devices with the same value belong to the same clock domain.
     * A value of 0 means no information about the clock domain is given.
     */
    [[nodiscard]] UInt32 get_clock_domain_id() const;

    /**
     *
     * @return A list of AudioObjectIDs of devices which are related to this device. AudioDevices are related if they share the same IOAudioDevice object.
     */
    [[nodiscard]] std::vector<UInt32> get_related_devices() const;

    static AudioDevice create_aggregate_device(const std::string& input_device_uid, const std::string& output_device_uid);

protected:
    void property_changed(const AudioObjectPropertyAddress& address) override;

private:
    /**
     * Static function which gets called by an audio device to provide and get audio data.
     * @return The return value is currently unused and should always be 0 (see AudioDeviceIOProc in AudioHardware.h).
     */
    static OSStatus audio_device_io_proc(AudioObjectID audio_object_id,
                                         const AudioTimeStamp* now,
                                         const AudioBufferList* input_data,
                                         const AudioTimeStamp* input_time,
                                         AudioBufferList* output_data,
                                         const AudioTimeStamp* output_time,
                                         void* client_data);

    /// Holds the identifier for the io proc audio callbacks.
    AudioDeviceIOProcID _io_proc_id{nullptr};
    AudioCallback* _audio_callback{nullptr};
    Scope _scope{Scope::UNDEFINED};
};

/**
 * Tries to find an audio device with given UID.
 * @param audio_devices The list of audio device to search in.
 * @param uid The UID of the device to find.
 * @return Pointer to the found device, or nullptr if no device with given UID was found.
 */
const AudioDevice* get_device_for_uid(const std::vector<AudioDevice>& audio_devices, const std::string& uid);

} // namespace apple_coreaudio

#endif // SUSHI_APPLE_COREAUDIO_DEVICE_H
