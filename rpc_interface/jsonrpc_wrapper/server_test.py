import asyncio
import websockets
from jsonrpcclient.clients.websockets_client import WebSocketsClient

DEFAULT_LISTENING_PORT = 19019

async def call_function(function, **kwargs): 
    async with websockets.connect("ws://localhost:" + str(DEFAULT_LISTENING_PORT)) as ws:
        try:
            response = await WebSocketsClient(ws).request(function, **kwargs)
            print("Response from "+ function +": \n" + str(response.text))
        except Exception as e:
            print("Error in rpc call " + function + " : " + str(e))


async def main():
    # Call all functions with some reasonable arguments and print the response
    await call_function("GetSamplerate")
    await call_function("GetPlayingMode")
    await call_function("SetPlayingMode", mode = "PLAYING")
    await call_function("GetSyncMode")
    await call_function("SetSyncMode", mode = "INTERNAL")
    await call_function("GetTempo")
    await call_function("SetTempo", tempo = 125)
    await call_function("GetTimeSignature")
    await call_function("SetTimeSignature", signature = {"numerator": 4, "denominator": 4})
    await call_function("GetTracks")

    # Keyboard control functions
    await call_function("SendNoteOn", track_id = 0, note = 45, channel = 0, velocity = 0.5)
    await call_function("SendNoteOff", track_id = 0, note = 45, channel = 0, velocity = 0.5)
    await call_function("SendNoteAftertouch", track_id = 0, note = 45, channel = 0, value = 0.5)
    await call_function("SendAftertouch", track_id = 0, channel = 0, value = 0.5)
    await call_function("SendPitchBend", track_id = 0, channel = 0, value = 0.5)
    await call_function("SendModulation", track_id = 0, channel = 0, value = 0.5)

    # Timings functions
    await call_function("GetEngineTimings")
    await call_function("GetTrackTimings", track_id = 0)
    await call_function("GetProcessorTimings", processor_id = 2)
    await call_function("ResetAllTimings")
    await call_function("ResetTrackTimings", track_id = 0)
    await call_function("ResetProcessorTimings", processor_id = 1)

    # Track controls
    await call_function("GetTrackId", track_name = "analog_synth")
    await call_function("GetTrackInfo", track_id = 0)
    await call_function("GetTrackProcessors", track_id = 0)
    await call_function("GetTrackParameters", track_id = 0)

    # Processor controls
    await call_function("GetProcessorId", processor_name = "jx10")
    await call_function("GetProcessorInfo", processor_id = 2)
    await call_function("GetProcessorBypassState", processor_id = 2)
    await call_function("SetProcessorBypassState", processor_id = 2, value = True)
    await call_function("GetProcessorCurrentProgram", processor_id = 2)
    await call_function("GetProcessorCurrentProgramName", processor_id = 2)
    await call_function("GetProcessorProgramName", processor_id = 2, program = 4)
    await call_function("GetProcessorPrograms", processor_id = 1)
    await call_function("SetProcessorProgram", processor_id = 1, program = 4)
    await call_function("GetProcessorParameters", processor_id = 1)

    # Parameter functions
    await call_function("GetParameterId", processor_id = 1, parameter_name = "VCF Freq")
    await call_function("GetParameterInfo", processor_id = 1, parameter_id = 2)
    await call_function("GetParameterValue", processor_id = 1, parameter_id = 2)
    await call_function("GetParameterValueNormalised", processor_id = 1, parameter_id = 2)
    await call_function("GetParameterValueAsString", processor_id = 1, parameter_id = 6)
    await call_function("GetStringPropertyValue", processor_id = 1, property_id = 2)
    await call_function("SetParameterValue", processor_id = 1, parameter_id = 2, value = 0.5)
    await call_function("SetParameterValueNormalised", processor_id = 1, parameter_id = 2, value = 0.5)
    await call_function("SetStringPropertyValue", processor_id = 1, property_id = 2, value = "string")

   
asyncio.get_event_loop().run_until_complete(main())