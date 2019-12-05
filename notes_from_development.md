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