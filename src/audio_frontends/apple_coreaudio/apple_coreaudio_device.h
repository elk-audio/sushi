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
         * @param input_data The audio input data.
         * @param num_input_channels Number of input channels in the input buffer.
         * @param output_data The audio output data.
         * @param num_output_channels Number of output channels in the output buffer.
         * @param input_host_time The time at which the input data was captured.
         */
        virtual void audio_callback([[maybe_unused]] const float* input_data,
                                    [[maybe_unused]] int num_input_channels,
                                    [[maybe_unused]] float* output_data,
                                    [[maybe_unused]] int num_output_channels,
                                    [[maybe_unused]] int num_frames,
                                    [[maybe_unused]] uint64_t input_host_time) {}

        /**
         * Called when the device changed its sample rate.
         * Warning! This call gets made from a random background thread, and there is no synchronisation or whatsoever.
         * @param new_sample_rate The new sample rate of the device.
         */
        virtual void sample_rate_changed([[maybe_unused]] double new_sample_rate) {}
    };

    AudioDevice() : AudioObject(0) {}
    explicit AudioDevice(AudioObjectID audio_object_id) : AudioObject(audio_object_id) {}

    virtual ~AudioDevice();

    SUSHI_DECLARE_NON_COPYABLE(AudioDevice)

    AudioDevice(AudioDevice&& other) noexcept : AudioObject(0)
    {
        *this = std::move(other); // Call into move assignment operator.
    }

    AudioDevice& operator=(AudioDevice&& other) noexcept;

    /**
     * Starts IO on this device.
     * @return True if successful, or false if an error occurred.
     */
    bool start_io(apple_coreaudio::AudioDevice::AudioCallback* audio_callback);

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
     * Gets the name of the device. When the device is an aggregate there will be different names
     * for the input device and output device, hence the ability to choose the scope.
     * @param scope The scope for which to get the name for.
     * @return The name of the device.
     */
    [[nodiscard]] virtual std::string get_name(Scope scope) const;

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
    [[nodiscard]] virtual int get_num_channels(bool for_input) const;

    /**
     * @param for_input True to get the number of input streams, or false to get the number of output streams.
     * @return The number of streams, or -1 if an error occurred.
     */
    size_t get_num_streams(bool for_input) const;

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
     *
     * @param for_input True to get the latency for the selected input stream, or false to get the latency for the selected output stream.
     * @return The latency of the selected stream in samples, or 0 if the stream for index does not exist.
     */
    [[nodiscard]] UInt32 get_selected_stream_latency(bool for_input) const;

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
     * @return A list of AudioObjectIDs of devices which are related to this device.
     * AudioDevices are related if they share the same IOAudioDevice object.
     */
    [[nodiscard]] std::vector<UInt32> get_related_devices() const;

    /**
     * Creates an aggregate device from given input- and output device.
     * This aggregate device is opinionated in the sense that the input channels of the input_device will be used
     * as input and the output channels of the output device as output. This discards the the output channels
     * of the input device and the input channels of the output device.
     * Said otherwise, while an aggregate device normally can have many sub devices, this particular instance will only have 2.
     * @param input_device The device to use as intput.
     * @param output_device The device to use as output.
     * @return An aggregate device, or nullptr if an error occurred.
     */
    static std::unique_ptr<AudioDevice> create_aggregate_device(const AudioDevice& input_device, const AudioDevice& output_device);

    /**
     * @return True if this audio device is an aggregate device, or false if not.
     */
    [[nodiscard]] bool is_aggregate_device() const;

protected:
    void property_changed(const AudioObjectPropertyAddress& address) override;

    /**
     * Selects an input or output stream.
     * Note: when this device is an aggregate device the number of streams will be the total of all streams of all devices.
     * @param for_input True to select an input stream, or false to select an output stream.
     * @param selected_stream_index The index of the stream to select.
     */
    void select_stream(bool for_input, size_t selected_stream_index);

private:
    /**
     * Static function which gets called by an audio device to provide and get audio data.
     * @return The return value is currently unused and should always be 0 (see AudioDeviceIOProc in AudioHardware.h).
     */
    static OSStatus _audio_device_io_proc(AudioObjectID audio_object_id,
                                          const AudioTimeStamp* now,
                                          const AudioBufferList* input_data,
                                          const AudioTimeStamp* input_time,
                                          AudioBufferList* output_data,
                                          const AudioTimeStamp* output_time,
                                          void* client_data);

    /**
     * @param for_input True to get input streams, or false to get output streams.
     * @return An array with AudioObjectIDs of streams for given input.
     */
    [[nodiscard]] std::vector<UInt32> _get_stream_ids(bool for_input) const;

    /// Holds the identifier for the io proc audio callbacks.
    AudioDeviceIOProcID _io_proc_id{nullptr};
    AudioCallback* _audio_callback{nullptr};

    size_t _selected_input_stream_index{0};
    size_t _selected_output_stream_index{0};
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
