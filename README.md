# 2DMeshSandbox
Physical modeling of circular membrane using 2D waveguides.

## Building

This project uses CMake to generate the build files. To build the project, run the following commands:

```bash
cmake -GNinja -Bbuild
cmake --build build
```

This project was tested and compiled on MacOS 15.1 using AppleClang 16.0.0 but should work on other platforms as well.

## References

The 2D waveguide mesh implementation is heavily inspired by Damian T. Murphy's PhD thesis: _"Digital Waveguide Mesh Topologies in Room Acoustics Modelling"_ (2000)

The math and theory behind the modeling of the circular membrane is based on Joel A. Laird PhD thesis: _"The physical modelling of drums using digital waveguides"_ (2001)

The nonlinear allpass filter design is based on John R. Pierce and Scott A. Van Duyne's paper: _"A passive nonlinear digital filter design which facilitates physics-based sound synthesis of highly nonlinear musical instruments"_ (1997)
