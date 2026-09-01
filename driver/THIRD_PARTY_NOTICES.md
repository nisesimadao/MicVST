# Third-party notices for the MicVST virtual audio driver

The MicVST application and the MicVST virtual audio driver are separate components with separate licensing obligations.

## VirtualDrivers/Virtual-Audio-Driver

The driver preparation scripts use source from:

- https://github.com/VirtualDrivers/Virtual-Audio-Driver
- pinned commit `bb34fba15faf569a6ae9bdea360bc1cf4821354e`

The upstream repository identifies its original code as MIT-licensed and also contains code derived from Microsoft Windows Driver Samples. The upstream repository's `THIRD_PARTY_NOTICES.md` states that Microsoft sample-derived portions are provided under the Microsoft Public License (MS-PL).

The preparation process keeps the upstream LICENSE and THIRD_PARTY_NOTICES files in the prepared driver source tree. Any distributed driver package must preserve the notices and terms that apply to that source.

## Microsoft Windows Driver Samples / WDK

The upstream driver is based in part on Microsoft audio driver samples such as SysVAD / Simple Audio Sample and is built with the Windows Driver Kit. Refer to the notices shipped by the upstream driver source and Microsoft's applicable license terms.

This file is informational and is not a replacement for the upstream license texts.
