## Developers Hardware specs
- GPU: Nvidia rtx 4070
- CPU: Ryzen 7 7800x3d
- RAM: 32GB DDR5 5600MHz
- PCB: b650 tomahawk
- SSD: Samsung 990 pro
- ENV: Arch Linux Wayland Pipewire

## Reading list:
A Tour of C++           [x] (little of value, unless you like iterators)
Data-Oriented Design    [ ]
Real-Time Rendering     [ ]

wayland-book            [ ]

# TODO:
// have a dynamic checklist for both readinglist and engine features at the top.
// The engine checklist would change overtime. Treating it as longterm plan list, that isnt fixed in place and changed frequently.
// or just a list of Achievements from the devlog, that links to that devlog where it was achieved, with a explination.

## Devlog_1: Intro
09.06.2026

#### Initial Plans:
Audio:
    the audio side of the linux kernel seems flawed
    and build around abstractions fighting eachother.
    Options:
    - 1: ALSA directly hopefully, i would have to disable pipewire,
         grab the ALSA lock and mix the audio.
    - 2: Support bluetooth audio only, using BlueZ, make/maintain/reconnect a connection,
         compress the mixed audio and send it as packages.
    This would require the engine to own the bluetooth connection to that audio device.
    Leaning on option 2, as i personally use bluetooth headset,
    and i/the user wouldnt have to disconnect and disable pipewire to free up ANSI each
    time to have audio, only need to disconnect the bluetooth headset.
    Its a bit out of my depth but seems like a fun challange,
    so i will give it a real shot, but leave it to a later date if i hit a wall.
Window:
    - Wayland Protocol(libwayland)
Engine:
    - Hot reload-able
    - Shared Object (Lib x API mixture, not sure yet)
    - Unity Build
    - Script to build and run
    - Memory manager
    - Custom immediate-mode UI editor
    - Job system
    - Performance Profiler in the editor
    - GLTF Asset prebaker
    - ECS
    - Starting with cpu rendering or Opengl, then Vulkan
Misc:
    - stb_truetype for texture bitmapping for the early period of development to render text.

# TODO: A bit ranty and disorginised, rewrite.
Starting work on a handmade game engine, written in c++, but in c style,
and avoiding c++'s Standard library and alot of the main features, such as classes.
    I am switching from modern c++ to c style to remove alot of abstractions and complexity that modern
c++ standard library hides from you in a hope to learn more about how to make software better aligned with
the capacity of the hardware, i dont claim to be able to implement custom features better than c++ ISO,
but i atleast want to know how its made, and can custom fit it to the engines needs. This is where DOD comes in.
    Most of the CS studies ive attended have heavily focused on OOP, SOLID, DRY and higher level launguages,
but with quite litte about how the cpu and gpu actually work with data and the program we write. So
the last 6 months i've been learning modern c++ on my freetime, since i believed it to be a very low level
language. However i recently tried to make a game engine in modern c++ using modules and realised that c++ was 
actually a low level language with a standard library that tried to be high level, but doing so on a 40+ old language
that need backwards compability introduced alot of complexity, templates bloat, unreadable source code,
layered abstractions, dynamic dispatch, hidden heap allocations, and so on, all dependent on the compiler to 
compile it down to "zero or negative overhead", but expload compile times and binary size.
Therefore i chose to try out c style c++, and might do a rewrite/integration in Jai,
if it gets opensourced within the next few months (Jai seems like a very capable laungage).
That way i can have full control off both memory, data-layout, data-manipulation and the meta programming.
    My goal for this project is not to make a generalist game engine. My first priority is making it customised
for my hardware and OS, keeping compile time very low, and therefore iterationspeed very high. I will also make
the engine a Unity cpp file, this will make the build.sh script very simple and very fast, it will make it easy
for the compiler to inline funtions and will have quite alot more runtime performance on debug builds compared to
c++ modules or seperate implementation and header files. Writing a script will also avoid using any external build
system such as CMake and Ninja.



    
    

