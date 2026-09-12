#pragma once

#include <QMatrix4x4>
#include <QVector3D>

/** @brief Per-frame data snapshot passed from SceneRenderer to every IRenderable. */
struct SceneState
{
    int viewportWidth = 0;   /**< Viewport width in pixels. */
    int viewportHeight = 0;  /**< Viewport height in pixels. */

    QMatrix4x4 orthogonalMatrix;
    QMatrix4x4 viewProjection;          /**< Combined view-projection matrix. */
    QMatrix4x4 normalMatrix; /**< Inverse-transposed model matrix for normal transformation. */
    
    float projectionScaleY = 1.0f;      /**< Projection matrix vertical scale for point sprite sizing. */
};
