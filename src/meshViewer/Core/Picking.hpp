#pragma once

#include <QMatrix4x4>
#include <QVector2D>

struct Ray
{
    QVector3D origin;
    QVector3D direction;  // unit vector
};

struct PendingPickRequest
{
    bool pending = false;
    int userCode = 0;
    QVector2D mousePos;
};

struct LayerPickResult
{
    int userCode = 0;
    bool hit = false;
    float distance = 0.0f;
    QVector3D worldPoint;
};

Ray unprojectRay(const QMatrix4x4& projection, const QMatrix4x4& transform, const QVector2D& mousePos, float viewportWidth, float viewportHeight);