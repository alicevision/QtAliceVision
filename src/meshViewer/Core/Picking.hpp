#pragma once

#include <QMatrix4x4>
#include <QVector2D>

/** @brief Half-line in world space used for picking. */
struct Ray
{
    QVector3D origin;     /**< Ray origin, on the near clipping plane. */
    QVector3D direction;  /**< Unit direction vector. */
};

/** @brief Pick request queued by the GUI thread and resolved by SceneRenderer on the next synchronization. */
struct PendingPickRequest
{
    bool pending = false;  /**< True while the request has not been processed. */
    int userCode = 0;      /**< Caller-defined code forwarded to the pick result. */
    QVector2D mousePos;    /**< Screen position, in item pixels (origin top-left). */
};

/** @brief Result of a pick query against a single layer. */
struct LayerPickResult
{
    int userCode = 0;      /**< Caller-defined code copied from the request. */
    bool hit = false;      /**< True when the ray hit the layer content. */
    float distance = 0.0f; /**< Distance from the ray origin to the hit point. */
    QVector3D worldPoint;  /**< Hit point, in world coordinates. */
};

/**
 * @brief Build the world-space ray going through a screen position.
 * @param[in] projection Projection matrix.
 * @param[in] transform View (world to camera) matrix.
 * @param[in] mousePos Screen position, in pixels (origin top-left).
 * @param[in] viewportWidth Viewport width, in pixels.
 * @param[in] viewportHeight Viewport height, in pixels.
 * @return The ray from the near to the far clipping plane, or a default ray if the matrices are not invertible.
 */
Ray unprojectRay(const QMatrix4x4& projection, const QMatrix4x4& transform, const QVector2D& mousePos, float viewportWidth, float viewportHeight);