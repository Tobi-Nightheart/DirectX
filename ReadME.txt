D3D12 Framework:
This is a work in progress to build a framework on DX12.
The framework is based on the book: Introduction to 3D Game Programming with DirectX 12 by Frank D. Luna.
Once the framework is ready for lighting I will add advanced lighting techniques to the framework.
Hold 1 for wireframe
use left click and drag mouse to turn camera

Dynamic Snow Shader (legacy):
#
#This project is going to be converted to the DX 11.1 Framework Rebuild which will be written without the usage of the Microsoft DirectX 11 SDK (June 2010) and might use some features introduced in DX11.1
#There will be various bugfixes, improvements and general code cleaning going on as this is only loosly based on the framework I learned DirectX with
#
It is a demo build with the Microsoft DirectX 11 SDK (June 2010) as this is the DirectX version my courses were on.
The demo includes a particle system with a hightmap pass to determin collision and simulation in Compute.
There is also a snowtexture which gets deformed. The deformed parts are rendered with wood texture just to
clearly mark the deformed parts. The mesh for the texture is tessellated to enhance the detail of the deformation.
The snow shader is based on the approach of Rise of the Tomb Raider as described by Anton Kai Michaels and
Peter Sikachev in Deferred Snow Deformation in Rise of the Tomb Raider published in GPU PRO7.
The tessellation stage is inspired by the adaptive tessellation demo in the June 2010 SDK package.
Currently there is an error in the linkage between the dynamic objects and the deformation shader thus it is not
working correctly.

The inputs on the Dynamic Snow Shader demo are:
W = camera forward
S = camera backward
A = camera strafe left
D = camera strafe right
Q = camera turn left
E = camera turn right
Z = camera turn up (clamped at 85)
X = camera turn down (clamped at -85)
I = camera elevate
K = camera lowering

1 = particles off
2 = particles 50%
3 = particles 100%
4 = dynamic objects toggle
5 = snow mesh wireframe toggle

ESC = close application

The exe needs to be placed into the folder with the solution to start