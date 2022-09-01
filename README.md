# Huddly IQ UVC XU API

This document describes details about Huddly IQs API features available over UVC Extension Units. 

# API Prerequisites

**This API is introduced with Huddly IQ firmware version 1.4.0 and will not work with any earlier versions**

## UVC Extension Units
UVC Extension Units is a way for UVC compliant cameras to expose custom features that are not already present in the UVC standard. These features are available in user mode through the operating systems native UVC drivers and do not require exclusive access to the UVC camera. The last point here is a significant difference from other proprietary USB communications methods. A major drawback with UVC Extension Units is that the transport does not allow high speed data transfers and is therefore not suitable for transferring larger amounts of data, such as binaries and images.

## Huddly IQ features available over UVC XU

Huddly IQ currently exposes the following features over UVC XU

Reading Genious Framing availability
Toggling between Gallery View or normal framing
Reading the camera firmware version
Reading and controlling the people counting feature
Controlling Genius Framing feature (AutoZoom) 

Note that if Genious Framing is Off, setting Autozoom Mode will be retained but has no effect until Genious Framing is turned on again.

Property 0x4:0xa and 0x6:0x2 both control the same property, Genious Framing control, but from two different GUIDs.

| Feature                | UVC Extension Unit GUID              | Unit ID  | Property ID | Length (bytes) | Operation | Value |
| ---------------------- | -----------------------------------  | -------- | ----------- | -------------- | --------- | ----- |
| Autozoom Available     | f6acc829-acdb-e511-8424-f39068f75511 |      0x4 | 0x9         | 1              | GET       | 0x0 Genius Framing Unavailable <br>0x1 Genius Framing Available |
| Autozoom (GF) control  | f6acc829-acdb-e511-8424-f39068f75511 |      0x4 | 0xa         | 1              | GET/SET   | 0x0 Genius Framing Off <br>0x1 Genius Framing On |
| Autozoom Mode          | f6acc829-acdb-e511-8424-f39068f75511 |      0x4 | 0xb         | 1              | GET/SET   | 0x0 Normal Framing<br>0x1 Gallery View<br>0x2 Gallery Duplicate (for testing) |
| Software version | f6acc829-acdb-e511-8424-f39068f75511| 0x4      | 0x13        | 8                 | GET       | Semantic version = [Byte 3].[Byte 2].[Byte 1]<br>Note: Byte 0, 4,5,6,7 are proprietary | 
| Genius framing control | a8bd5df2-1a98-474e-8dd0-d92672d194fa | 0x6 | 0x2 | 1 | GET/SET | 0x0 Genius Framing Off <br>0x1 Genius Framing On |
| People count control | a8bd5df2-1a98-474e-8dd0-d92672d194fa | 0x6 | 0x7 | 1 | GET/SET | 0x0 People count Off<br>0x1 People count always on<br>0x2 People count on when streaming |
| People count read	| a8bd5df2-1a98-474e-8dd0-d92672d194fa | 0x6 | 0x8 | 1 | GET | People count reading 0-255 |

