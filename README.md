# 2DMeshSandbox
Physical modeling of circular membrane using 2D waveguides.

## Building

To clone the repository, run the following command:

```bash
git clone https://github.com/Segfault1602/2DMeshSandbox.git --recurse-submodules
```

If you forgot to clone the submodules, you can run the following command to fetch them:

```bash
git submodule update --init --recursive
```

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
