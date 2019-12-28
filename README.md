# clPingPong
OpenCL devices and GPUs in particular, typically own a separate physical memory and they require memory tranfers by communicating with the main host processor.
This fact induces overhead when running light-weight kernels requiring interaction between the host CPU and the device and vice versa.
This communication potentially poses a significant overhead which should be taken into account.

This benchmark measures host to OpenCL device round-trip delay times.
It applies a light-weight kernel execution performing a trivial computation (i.e. addition) on a minor piece of data (i.e. one single 32bit integer), followed by a similar operation on the host device.
This flow is applied thousands of times sequentially, resembling a ping-pong game between the CPU and the OpenCL device.
The average round-trip delay time is measured and reported by utilizing either device or host allocated memory.
Communication is performed either by explicit memory transfers or memory mapped access.
If delay times are significant to the actual computation then this might provide a hint to restructuring the application or even abandoning the use of an accelerator device for this purpose.

Compilation
--------------

Program is written in C++/OpenCL and can be build by using cmake, as follows:

```
mkdir build
cd build
cmake ../
```

For a usage example please refer to paragraph below.

Invocation
--------------

Run the program without arguments or optionally you may choose the platform index and the device index *a priori*:

See the example below:

```
$ ./clpingpong
clPingPong - OpenCL host to device overhead evaluation

Platform/Device selection
Total platforms: 1
Intel(R) OpenCL HD Graphics
        1. Intel(R) Gen9 HD Graphics NEO/Intel(R) Corporation

Device info
Device:         Intel(R) Gen9 HD Graphics NEO
Driver version: 19.21.13045

Warming up... OK

Device <-> Device reference experiments
Device allocated memory w.sync: 638.56 msecs (31.928 usecs/invocation)
Host allocated memory w.sync:   638.469 msecs (31.9235 usecs/invocation)

Host <-> Device experiments
Experiment #1: Device allocated memory w. mapping:  PASSED!
Experiment #2: Device allocated memory w. transfer: PASSED!
Experiment #3: Host allocated memory w. mapping:    PASSED!
Experiment #4: Host allocated memory w. transfer:   PASSED!

Benchmark summary (Host <-> Device)
                        Time measurements (usecs/loop)
Communication type  |  Regular buffer  |  Host allocated  |
--------------------|------------------|------------------|
Memory mapped       |         33.2562  |         33.2504  |
Explicit transfer   |         33.2487  |         33.2981  |

Host overhead: 4.1513%
```

