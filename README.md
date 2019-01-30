# dcAudioGraph
An audio/control processing graph for use in interactive sound software.

**Please note: this library is currently in heavy, early development. Many things may change on the `dev` branch without warning, and backward compatibility is not guaranteed.**

## Why?

There's not a small, dependency-free audio graph library out there, at least that we know of. Hopefully it's helpful.

### Features
* Audio and control graph mechanism that allows nested graphs
* Simple module interface for making new audio and control processors
* Sample-accurate parameter modulation
* Sample-accurate event triggering (MIDI-style notes and generic triggers)
* Thread safe and lock-free (or at least we are working toward it, let us know if you run into an issue)
* Runtime mutable everything (modules in graphs, parameters and I/O on modules)
* Message queue for modules that might need to pass info between the main and audio threads

### Limitations
* Feedback loops, even with control/events, are not currently allowed
* It is assumed that this library is being used from two threads at maximum (main/GUI/whatever and audio). It may barf if you are operating on modules or graphs from more than that.
* Module accessors should not be used from the audio thread (as in the `process()` method). Instead, use the context that is passed in.
* The graph is designed to be processed in one thread (the "audio" thread). It does not currently support multithreaded processing.
* Sample type is currently hard-coded to single precision floats.

## Dependencies
* **None!**

## Prerequisites
* A C++14-compatible compiler.

## Installation
1. Clone this repository
2. Copy the contents of the subfolder `dcAudioGraph` to your project
3. `#include "dcAudioGraph.h"`

Alternatively, you could build it as a static library. See the vs2017 project for an example of how to do that. Honestly, it's more straightforward to just include the source in your project.

## Contributing
Yes please! Help is super appreciated. 

Get in touch with Charlie ([@charliehuge](https://twitter.com/charlieHUGE)) if you'd like to help improve the library.

If you find a bug, please file an issue or submit a pull request.

## License
**dcAudioGraph** is licensed under the [3-Clause BSD License](https://opensource.org/licenses/BSD-3-Clause). See the `LICENSE` file for details. 

TLDR: It's yours, for free. Don't say we never gave you nothing. *Also, don't say we endorse what you do with it in any way.*

One request: if you use the library, let us know!

Ok, two requests, actually: if you use the library and end up fixing something in it, please share your fixes!
