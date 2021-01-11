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
                    "attribute highp vec4 vertex;              \n"
                    "attribute highp vec2 texCoord;            \n"
                    "uniform highp mat4 qt_Matrix;              \n"
                    "varying highp vec2 vTexCoord;              \n"
                    "void main() {                              \n"
                    "    gl_Position = qt_Matrix * vertex;      \n"
                    "    vTexCoord = texCoord;      \n"
                    "}";
            }

            const char* fragmentShader() const override
            {
                return
                    "uniform lowp float qt_Opacity;                                                  \n"
                    "uniform highp sampler2D texture;                                                \n"
                    "uniform lowp float gamma;                                                       \n"
                    "uniform lowp float gain;                                                        \n"
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
                    "    gl_FragColor.rgb *= gl_FragColor.a; \n"
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
                _channelOrder = program()->uniformLocation("channelOrder");

                // We will only use texture unit 0, so set it only once.
                program()->setUniformValue(_textureId, 0);
            }

        private:
            int _textureId = -1;
            int _gammaId = -1;
            int _gainId = -1;
            int _channelOrder = -1;
        };

    }

}  // qtAliceVision