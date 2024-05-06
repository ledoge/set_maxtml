# About

This is a simple tool for overriding the MaxTML value that Windows 10/11 reports for a display, i.e. what you'd normally use the Windows 11 HDR calibration app for. That app is not supported on Windows 10, and its generated ICC profiles seem to cause various issues, so you can use this instead.

# Usage

```
set_maxtml
```
to get a list of the currently active monitors and their current maxTML values

```
set_maxtml <monitor index> <nits>
```
to set a new MaxTML value, e.g. `set_maxtml 1 1000` to set 1000 nits for the first listed monitor.

A similar `set_sdrwhite` tool for setting the SDR brightness slider value is also included. Using `0` as the monitor index will apply the SDR white value to all monitors.

# Notes
* In addition to MaxTML, this tool sets MaxFFTML = MaxTML and MinTML = 0 (as that is how tone mapping works on any sane display), and the white point coordinates to the standard D65 values of (0.3127, 0.3290).
* As long as you don't reboot, the set value seems to persist between switching from HDR to SDR and back, as long as you don't have an "Advanced Color Profile" assigned in Windows.
