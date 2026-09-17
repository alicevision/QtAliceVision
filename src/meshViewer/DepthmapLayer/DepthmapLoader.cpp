#include <DepthmapLayer/DepthmapLoader.hpp>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/io.hpp>
#include <aliceVision/image/jetColorMap.hpp>
#include <aliceVision/mvsData/Matrix3x3.hpp>
#include <aliceVision/mvsData/Point2d.hpp>
#include <aliceVision/mvsData/Point3d.hpp>
#include <aliceVision/numeric/numeric.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

using namespace aliceVision;

namespace {

/** @brief Rejects degenerate/sliver triangles, same heuristic as DepthMapEntity. */
bool validTriangleRatio(const Vec3f& a, const Vec3f& b, const Vec3f& c)
{
    std::vector<double> distances = {DistanceL2(a, b), DistanceL2(b, c), DistanceL2(c, a)};
    double mi = std::min({distances[0], distances[1], distances[2]});
    double ma = std::max({distances[0], distances[1], distances[2]});
    if (ma == 0.0)
        return false;
    return (mi / ma) > 1.0 / 5.0;
}

/** @brief Reads the camera center ("AliceVision:CArr") metadata, as in DepthMapEntity. */
std::optional<Eigen::Vector3d> getCenter(const oiio::ImageSpec& inSpec)
{
    Eigen::Vector3d center;
    const oiio::ParamValue* cParam = inSpec.find_attribute("AliceVision:CArr");

    if (!cParam)
    {
        return std::nullopt;
    }

    if (cParam->type().aggregate != oiio::TypeDesc::AGGREGATE::VEC3)
    {
        return std::nullopt;
    }

    if (cParam->type().basetype == oiio::TypeDesc::BASETYPE::DOUBLE)
    {
        const double* d = static_cast<const double*>(cParam->data());
        for (int i = 0; i < 3; ++i)
        {
            center(i) = d[i];
        }

        return center;
    }

    if (cParam->type().basetype == oiio::TypeDesc::BASETYPE::FLOAT)
    {
        const float* d = static_cast<const float*>(cParam->data());
        for (int i = 0; i < 3; ++i)
        {
            center(i) = d[i];
        }

        return center;
    }

    return std::nullopt;
}

/** @brief Reads the inverse projection matrix ("AliceVision:iCamArr") metadata, as in DepthMapEntity. */
std::optional<Eigen::Matrix3d> getInverseProjection(const oiio::ImageSpec& inSpec)
{
    Eigen::Matrix3d inverseProjectionMatrix;
    const oiio::ParamValue* icParam = inSpec.find_attribute("AliceVision:iCamArr");

    if (!icParam)
    {
        return std::nullopt;
    }

    if (icParam->type().aggregate != oiio::TypeDesc::AGGREGATE::MATRIX33)
    {
        return std::nullopt;
    }

    if (icParam->type().basetype == oiio::TypeDesc::BASETYPE::DOUBLE)
    {
        const double* d = static_cast<const double*>(icParam->data());
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                inverseProjectionMatrix(i, j) = d[i * 3 + j];
            }
        }

        return inverseProjectionMatrix;
    }

    if (icParam->type().basetype == oiio::TypeDesc::BASETYPE::FLOAT)
    {
        const float* d = static_cast<const float*>(icParam->data());
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                inverseProjectionMatrix(i, j) = d[i * 3 + j];
            }
        }

        return inverseProjectionMatrix;
    }

    return std::nullopt;
}

/** @brief Falls back to a "fov" metadata attribute to derive the inverse projection, as in DepthMapEntity. */
std::optional<Eigen::Matrix3d> getInverseProjectionFromFov(const oiio::ImageSpec& inSpec)
{
    double fovDegrees = -1.0;

    // Try to find fov metadata
    for (const auto& param : inSpec.extra_attribs)
    {
        std::string name = param.name().string();
        size_t pos = name.find_last_of(":");
        if (pos != std::string::npos)
        {
            name = name.erase(0, pos + 1);
        }

        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (name != "fov")
        {
            continue;
        }

        fovDegrees = double(param.get_float(90.0));
    }

    if (fovDegrees < 0)
    {
        return std::nullopt;
    }

    double fovRadians = aliceVision::degreeToRadian(fovDegrees);

    // Simple trigonometry
    // tan(fov / 2) = (width / 2) / f
    // f = (width / 2) / (tan(fov / 2))
    double w = double(inSpec.width);
    double f = (w * 0.5) / std::tan(fovRadians * 0.5);

    Eigen::Matrix3d inverseProjectionMatrix = Eigen::Matrix3d::Identity();
    inverseProjectionMatrix(0, 0) = 1.0 / f;
    inverseProjectionMatrix(1, 1) = 1.0 / f;

    return inverseProjectionMatrix;
}

/** @brief Resolves the companion depthMap/simMap paths from a single source path, as in DepthMapEntity::setSource. */
bool resolveCompanionPaths(const QString& path, QString& depthMapPath, QString& simMapPath, QString& errorString)
{
    QFileInfo fileInfo(path);
    QString filename = fileInfo.fileName();

    if (filename.contains("depthMap"))
    {
        depthMapPath = path;
        simMapPath = QFileInfo(fileInfo.dir(), QString(filename).replace("depthMap", "simMap")).filePath();
    }
    else if (filename.contains("simMap"))
    {
        simMapPath = path;
        depthMapPath = QFileInfo(fileInfo.dir(), QString(filename).replace("simMap", "depthMap")).filePath();
    }
    else if (filename.contains("depth"))
    {
        depthMapPath = path;
        simMapPath.clear();
    }
    else
    {
        errorString = QStringLiteral("Source filename must contain depthMap or simMap: %1").arg(filename);
        return false;
    }

    return true;
}

}  // namespace

std::unique_ptr<DepthmapData> DepthmapLoader::load(const QString& path)
{
    auto result = std::make_unique<DepthmapData>();

    QString depthMapPath;
    QString simMapPath;
    if (!resolveCompanionPaths(path, depthMapPath, simMapPath, result->errorString))
    {
        qWarning() << "DepthmapLoader:" << result->errorString;
        return result;
    }

    // Load depth map and metadata

    qDebug() << "DepthmapLoader: load depth map:" << depthMapPath;
    image::Image<float> depthMap;
    try
    {
        image::readImage(depthMapPath.toStdString(), depthMap, image::EImageColorSpace::LINEAR);
    }
    catch (const std::runtime_error& error)
    {
        result->errorString = QStringLiteral("Could not load depth map: %1").arg(error.what());
        qWarning() << "DepthmapLoader:" << result->errorString;
        return result;
    }

    oiio::ImageBuf inBuf;
    image::getBufferFromImage(depthMap, inBuf);

    qDebug() << "DepthmapLoader: image size:" << depthMap.width() << "x" << depthMap.height();

    oiio::ImageSpec inSpec = image::readImageSpec(depthMapPath.toStdString());

    Eigen::Vector3d center = Eigen::Vector3d::Zero();
    std::optional<Eigen::Vector3d> optCenter = getCenter(inSpec);
    if (optCenter.has_value())
    {
        center = optCenter.value();
    }

    Eigen::Matrix3d inverseProjectionMatrix = Eigen::Matrix3d::Identity();
    std::optional<Eigen::Matrix3d> optInverseProjectionMatrix = getInverseProjection(inSpec);

    if (optInverseProjectionMatrix.has_value())
    {
        inverseProjectionMatrix = optInverseProjectionMatrix.value();
    }
    else
    {
        optInverseProjectionMatrix = getInverseProjectionFromFov(inSpec);
        if (optInverseProjectionMatrix.has_value())
        {
            inverseProjectionMatrix = optInverseProjectionMatrix.value();
        }
        else
        {
            result->errorString = QStringLiteral("Could not find camera projection metadata (AliceVision:iCamArr or fov)");
            qWarning() << "DepthmapLoader:" << result->errorString;
            return result;
        }
    }

    // Load optional sim map

    image::Image<float> simMap;
    if (!simMapPath.isEmpty())
    {
        qDebug() << "DepthmapLoader: load sim map:" << simMapPath;
        try
        {
            image::readImage(simMapPath.toStdString(), simMap, image::EImageColorSpace::LINEAR);
        }
        catch (const std::runtime_error& error)
        {
            qWarning() << "DepthmapLoader: sim map could not be loaded:" << error.what();
        }
    }
    else
    {
        qWarning() << "DepthmapLoader: failed to find associated sim map";
    }

    const bool validSimMap = (simMap.width() == depthMap.width()) && (simMap.height() == depthMap.height());

    // 3D points position and color (using jetColorMap)

    qDebug() << "DepthmapLoader: computing positions and colors";

    std::vector<int> indexPerPixel(static_cast<std::size_t>(depthMap.width() * depthMap.height()), -1);
    std::vector<Vec3f> positions;
    std::vector<image::RGBfColor> colors;

    oiio::ImageBufAlgo::PixelStats stats = oiio::ImageBufAlgo::computePixelStats(inBuf);

    for (int y = 0; y < depthMap.height(); ++y)
    {
        for (int x = 0; x < depthMap.width(); ++x)
        {
            float depthValue = depthMap(y, x);
            if (!std::isfinite(depthValue) || depthValue <= 0.f)
                continue;

            Vec2 pixelCoordinates;
            pixelCoordinates.x() = x;
            pixelCoordinates.y() = y;

            Vec3 meterCoordinates = inverseProjectionMatrix * pixelCoordinates.homogeneous();
            Vec3 point3d = meterCoordinates.normalized() * depthValue;

            Vec3 p = center + point3d;
            Vec3f position(static_cast<float>(p.x()), static_cast<float>(-p.y()), static_cast<float>(-p.z()));

            indexPerPixel[static_cast<std::size_t>(y * depthMap.width() + x)] = static_cast<int>(positions.size());
            positions.push_back(position);

            if (validSimMap)
            {
                float simValue = simMap(y, x);
                colors.push_back(getColorFromJetColorMap(simValue));
            }
            else
            {
                const float range = stats.max[0] - stats.min[0];
                float normalizedDepthValue = range != 0.0f ? (depthValue - stats.min[0]) / range : 1.0f;
                colors.push_back(getColorFromJetColorMap(normalizedDepthValue));
            }
        }
    }

    // Build indexed triangles (2 per pixel quad), filtered by the triangle quality ratio.
    // Unlike DepthMapEntity, indices reference the (already deduplicated) vertex list directly
    // instead of flattening/duplicating vertices per-face, since no per-face normal is stored.

    qDebug() << "DepthmapLoader: building triangle indices";

    std::vector<quint32> indices;
    indices.reserve(static_cast<std::size_t>(2 * 3) * positions.size());

    for (int y = 0; y < depthMap.height() - 1; ++y)
    {
        for (int x = 0; x < depthMap.width() - 1; ++x)
        {
            int pixelIndexA = indexPerPixel[static_cast<std::size_t>(y * depthMap.width() + x)];
            int pixelIndexB = indexPerPixel[static_cast<std::size_t>((y + 1) * depthMap.width() + x)];
            int pixelIndexC = indexPerPixel[static_cast<std::size_t>((y + 1) * depthMap.width() + x + 1)];
            int pixelIndexD = indexPerPixel[static_cast<std::size_t>(y * depthMap.width() + x + 1)];

            if (pixelIndexA != -1 && pixelIndexB != -1 && pixelIndexC != -1 &&
                validTriangleRatio(positions[static_cast<std::size_t>(pixelIndexA)],
                                   positions[static_cast<std::size_t>(pixelIndexB)],
                                   positions[static_cast<std::size_t>(pixelIndexC)]))
            {
                indices.push_back(static_cast<quint32>(pixelIndexA));
                indices.push_back(static_cast<quint32>(pixelIndexB));
                indices.push_back(static_cast<quint32>(pixelIndexC));
            }

            if (pixelIndexC != -1 && pixelIndexD != -1 && pixelIndexA != -1 &&
                validTriangleRatio(positions[static_cast<std::size_t>(pixelIndexC)],
                                   positions[static_cast<std::size_t>(pixelIndexD)],
                                   positions[static_cast<std::size_t>(pixelIndexA)]))
            {
                indices.push_back(static_cast<quint32>(pixelIndexC));
                indices.push_back(static_cast<quint32>(pixelIndexD));
                indices.push_back(static_cast<quint32>(pixelIndexA));
            }
        }
    }

    qDebug() << "DepthmapLoader: nb vertices:" << positions.size() << "nb triangles:" << indices.size() / 3;

    // Fill the vertex buffer: one entry per valid depth pixel (position + color).
    result->vertices.reserve(static_cast<int>(positions.size()));
    for (std::size_t i = 0; i < positions.size(); ++i)
    {
        const Vec3f& p = positions[i];
        const image::RGBfColor& c = colors[i];
        result->vertices.append({p.x(), p.y(), p.z(), c.r(), c.g(), c.b()});
    }

    result->indices.reserve(static_cast<int>(indices.size()));
    for (quint32 index : indices)
    {
        result->indices.append(index);
    }

    computeBounds(*result);
    result->valid = true;

    return result;
}

void DepthmapLoader::computeBounds(DepthmapData& data)
{
    if (data.vertices.isEmpty())
    {
        data.minX = data.maxX = 0.0f;
        data.minY = data.maxY = 0.0f;
        data.minZ = data.maxZ = 0.0f;
        return;
    }

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();

    for (const ColoredVertex& v : data.vertices)
    {
        minX = std::min(minX, v.x);
        maxX = std::max(maxX, v.x);
        minY = std::min(minY, v.y);
        maxY = std::max(maxY, v.y);
        minZ = std::min(minZ, v.z);
        maxZ = std::max(maxZ, v.z);
    }

    data.minX = minX;
    data.maxX = maxX;
    data.minY = minY;
    data.maxY = maxY;
    data.minZ = minZ;
    data.maxZ = maxZ;
}
