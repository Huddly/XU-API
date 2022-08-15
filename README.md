# Huddly IQ UVC XU API

This document describes details about Huddly IQs API features available over UVC Extension Units.

# API Prerequisites

**This API is introduced with Huddly IQ firmware version 1.4.0 and will not work with any earlier versions**

## UVC Extension Units
UVC Extension Units is a way for UVC compliant cameras to expose custom features that are not already present in the UVC standard. These features are available in user mode through the operating systems native UVC drivers and do not require exclusive access to the UVC camera. The last point here is a significant difference from other proprietary USB communications methods. A major drawback with UVC Extension Units is that the transport does not allow high speed data transfers and is therefore not suitable for transferring larger amounts of data, such as binaries and images.

## Huddly IQ features available over UVC XU

Huddly IQ currently exposes the following features over UVC XU

Reading the camera firmware version
Reading and controlling the people counting feature
Controlling Genius Framing feature (AutoZoom)


| Feature |	UVC Extension Unit GUID |	Unit ID | Property ID | Length (in bytes) | Operation | Value |
| ------- | ----------------------- | --------  | ----------- | ----------------- | --------- | ----- |
| Software version | 6acc829-acdb-e511-8424-f39068f75511| 0x4      | 0x19        | 8                 | GET       | Semantic version = [Byte 3].[Byte 2].[Byte 1]<br>Note: Byte 0, 4,5,6,7 are proprietary |
| Genius features control | a8bd5df2-1a98-474e-8dd0-d92672d194fa | 0x6 | 0x2 | 1 | GET/SET | 0x0 Genius features Off <br>0x1 Genius features On |
| People count control | a8bd5df2-1a98-474e-8dd0-d92672d194fa | 0x6 | 0x7 | 1 | GET/SET | 0x0 People count Off<br>0x1 People count always on<br>0x2 People count on when streaming |
| People count read	| a8bd5df2-1a98-474e-8dd0-d92672d194fa | 0x6 | 0x8 | 1 | GET | People count reading 0-255 |
| Genius feature mode | a8bd5df2-1a98-474e-8dd0-d92672d194fa | 0x6 | 0xB | GET/SET | 0x0 Genius Framing mode  <br>0x1 Gallery View mode |

