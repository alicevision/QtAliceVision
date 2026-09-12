#pragma once

struct Vertex {
    float x,  y,  z;   // position
    float nx, ny, nz;  // normal
};

struct PositionVertex {
    float x, y, z;   // position
};

struct ColoredVertex {
    float x, y, z;   // position
    float r, g, b;   // color
};

struct QuadVertex
{
    float x, y;
};

struct WireVertex {
    float x, y, z;    // position
    float bx, by, bz; // barycentric coordinates
};