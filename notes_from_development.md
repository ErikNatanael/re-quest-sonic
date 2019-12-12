# Notes from the development


Calls between scripts are too common to have the script changes be a fundamental change in sonification, it gets chaotic as in v0.1. One way to change this is to heavily quantize function call events. Another is to let script changes affect a different parameter.

## What is the essence of the data?

- Interconnectedness
- Complexity
- Speed
- Time

## What is lacking from the data analysis

- Time spent in different functions from the profiling. E.g. the time spent idle is not reflected at all.
- [x] The type of script (built in, fetched, extension)
- The shape/contour of the data
- [x] The origin of the data (whispering "Google", "Grammarly", "KTH" ??)
- How the low level function call events interact with the higher level user events and the network events

## Challenges identified:

- There is a much larger range of activity/inactivity than our brains have the capacity to fathom. The high activity events are very very close together separated by (relatively) long durations of inactivity.

## Ideas for improvement or experimentation

- Multiple layers: 
  - one "real-time" layer that consists of the data points being triggered when they happen
  - one layer whose parameters depend on the entirety of the data trace, or alternatively is expanded by the data in the trace as it is being triggered, but where the data modifies its parameters instead of making any direct impact

## v0.5

Taking a microphone input and filtering it as the basis for all sound synthesis means that you can play on the data and superimpose a structure on the data sonification. As an installation it also means that active action is needed to reveal what is hidden. Blowing noise is one of the simplest ways, analogous to blowing smoke into lasers to make them visible.
