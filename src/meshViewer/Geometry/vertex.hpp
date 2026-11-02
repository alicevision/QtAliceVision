#pragma once

/** @brief Vertex with a position and a normal. */
struct Vertex {
    float x,  y,  z;   /**< Position. */
    float nx, ny, nz;  /**< Normal. */
};

/** @brief Texture coordinates. */
struct UVVertex {
    float u, v;        /**< Texture coordinates. */
};

/** @brief Vertex with a position only. */
struct PositionVertex {
    float x, y, z;     /**< Position. */
};

/** @brief Vertex with a position and an RGB color. */
struct ColoredVertex {
    float x, y, z;     /**< Position. */
    float r, g, b;     /**< Color. */
};

/** @brief 2D vertex, typically in NDC for screen-aligned quads. */
struct QuadVertex
{
    float x, y;        /**< Position. */
};

/** @brief Vertex for wireframe rendering, carrying barycentric coordinates. */
struct WireVertex {
    float x, y, z;     /**< Position. */
    float bx, by, bz;  /**< Barycentric coordinates within the triangle. */
};
