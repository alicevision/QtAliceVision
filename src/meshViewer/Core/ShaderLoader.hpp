#pragma once

#include <rhi/qshader.h>

#include <QFile>
#include <QString>

/**
 * @brief Loads a serialized shader (.qsb) produced by qt_add_shaders.
 * @param path Resource path of the shader, e.g. ":/shaders/simpleColor.vert.qsb".
 * @return The deserialized shader.
 * @warning Aborts with qFatal() when the file cannot be opened: shaders are compiled-in resources.
 */
inline QShader loadShader(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qFatal("Cannot open shader %s", qPrintable(path));
    }

    return QShader::fromSerialized(file.readAll());
}
