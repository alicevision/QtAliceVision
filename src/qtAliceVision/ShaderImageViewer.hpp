#pragma once

#include <memory>

#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGSimpleMaterial>
#include <QSGSimpleMaterialShader>
#include <QSGTexture>


namespace qtAliceVision
{
    namespace
    {
        struct ShaderData
        {
            float gamma = 1.f;
            float gain = 0.f;
            QVector2D fisheyeCircleCoord = QVector2D(0, 0);
            float fisheyeCircleRadius = 0.f;
            float aspectRatio = 0.f;
            QVector4D channelOrder = QVector4D(0, 1, 2, 3);
            std::unique_ptr<QSGTexture> texture;
        };

        class ImageViewerShader : public QSGSimpleMaterialShader<ShaderData>
        {
            QSG_DECLARE_SIMPLE_SHADER(ImageViewerShader, ShaderData)

        public:
            const char* vertexShader() const override
            {
                return
                    "attribute highp vec4 vertex;               \n"
                    "attribute highp vec2 texCoord;             \n"
                    "uniform highp mat4 qt_Matrix;              \n"
                    "varying highp vec2 vTexCoord;              \n"
                    "void main() {                              \n"
                    "    gl_Position = qt_Matrix * vertex;      \n"
                    "    vTexCoord = texCoord;                  \n"
                    "}";
            }

            const char* fragmentShader() const override
            {
                return
                    "uniform lowp float qt_Opacity;                                                  \n"
                    "uniform highp sampler2D texture;                                                \n"
                    "uniform lowp float gamma;                                                       \n"
                    "uniform lowp float gain;                                                        \n"
                    "uniform vec2 fisheyeCircleCoord;                                                \n"
                    "uniform float fisheyeCircleRadius;                                              \n"
                    "uniform float aspectRatio;                                                      \n"
                    "uniform vec4 channelOrder;                                                      \n"
                    "varying highp vec2 vTexCoord;                                                   \n"
                    "void main() {                                                                   \n"
                    "    vec4 color = texture2D(texture, vTexCoord);                                 \n"
                    "    color.rgb = pow(pow((color.rgb * vec3(gain)), vec3(1.0/gamma)), vec3(1.0 / 2.2)); \n"
                    "    gl_FragColor.r = color[int(channelOrder[0])];                               \n"
                    "    gl_FragColor.g = color[int(channelOrder[1])];                               \n"
                    "    gl_FragColor.b = color[int(channelOrder[2])];                               \n"
                    "    gl_FragColor.a = int(channelOrder[3]) == -1 ? 1.0 : color[int(channelOrder[3])]; \n"
                    "    gl_FragColor.a *= qt_Opacity; \n"
                    "    if(distance(vec2(vTexCoord.x * aspectRatio ,vTexCoord.y), vec2(fisheyeCircleCoord.x * aspectRatio , fisheyeCircleCoord.y)) > fisheyeCircleRadius && fisheyeCircleRadius > 0.0) {    \n"
                    "       gl_FragColor.a *= 0.001;                                                  \n"
                    "    }                                                                            \n"
                    "    gl_FragColor.rgb *= gl_FragColor.a;                                          \n"
                    "}";
            }

            QList<QByteArray> attributes() const override
            {
                return QList<QByteArray>() << "vertex" << "texCoord";
            }

            void updateState(const ShaderData* data, const ShaderData*) override
            {
                program()->setUniformValue(_gammaId, data->gamma);
                program()->setUniformValue(_gainId, data->gain);
                program()->setUniformValue(_channelOrder, data->channelOrder);

                program()->setUniformValue(_fisheyeCircleCoordId, data->fisheyeCircleCoord);
                program()->setUniformValue(_fisheyeCircleRadiusId, data->fisheyeCircleRadius);
                program()->setUniformValue(_aspectRatio, data->aspectRatio);

                if (data->texture)
                {
                    data->texture->bind();
                }
            }

            void resolveUniforms() override
            {
                _textureId = program()->uniformLocation("texture");
                _gammaId = program()->uniformLocation("gamma");
                _gainId = program()->uniformLocation("gain");
                _fisheyeCircleCoordId = program()->uniformLocation("fisheyeCircleCoord");
                _fisheyeCircleRadiusId = program()->uniformLocation("fisheyeCircleRadius");
                _aspectRatio = program()->uniformLocation("aspectRatio");
                _channelOrder = program()->uniformLocation("channelOrder");

                // We will only use texture unit 0, so set it only once.
                program()->setUniformValue(_textureId, 0);
            }

        private:
            int _textureId = -1;
            int _gammaId = -1;
            int _gainId = -1;
            int _channelOrder = -1;
            int _fisheyeCircleCoordId = -1;
            int _fisheyeCircleRadiusId = -1;
            int _aspectRatio = -1;
        };

    }

}  // qtAliceVision
