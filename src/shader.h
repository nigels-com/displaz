// Copyright 2015, Christopher J. Foster and the other displaz contributors.
// Use of this code is governed by the BSD-style license found in LICENSE.txt

#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include <memory>

#ifdef DISPLAZ_USE_QT4
    #include <QtCore/QMap>
    #include <QtOpenGL/QGLShader>
    #include <QtOpenGL/QGLShaderProgram>
#else
    #include <QMap>
    #include <QOpenGLShader>
    #include <QOpenGLShaderProgram>
#endif

/// Representation of a shader "parameter" (uniform variable or attribute)
struct ShaderParam
{
    enum Type {
        Float,
        Int,
        Vec3
    };

    Type type;             ///< Variable type
    QByteArray name;       ///< Name of the variable in the shader
    QVariant defaultValue; ///< Default value
    QMap<QString,QString> kvPairs; ///< name,value pairs with additional metadata
    int ordering;          ///< Ordering in source file

    QString uiName() const
    {
        return kvPairs.value("uiname", name);
    }

    double getDouble(QString name, double defaultVal) const
    {
        if (!kvPairs.contains(name))
            return defaultVal;
        bool convOk = false;
        double val = kvPairs[name].toDouble(&convOk);
        if (!convOk)
            return defaultVal;
        return val;
    }

    int getInt(QString name, int defaultVal) const
    {
        if (!kvPairs.contains(name))
            return defaultVal;
        bool convOk = false;
        double val = kvPairs[name].toInt(&convOk);
        if (!convOk)
            return defaultVal;
        return val;
    }

    ShaderParam(Type type=Float, QByteArray name="",
                QVariant defaultValue = QVariant())
        : type(type), name(name), defaultValue(defaultValue), ordering(0) {}
};


inline bool operator==(const ShaderParam& p1, const ShaderParam& p2)
{
    return p1.name == p2.name && p1.type == p2.type &&
        p1.defaultValue == p2.defaultValue &&
        p1.kvPairs == p2.kvPairs && p1.ordering == p2.ordering;
}


// Operator for ordering in QMap
inline bool operator<(const ShaderParam& p1, const ShaderParam& p2)
{
    if (p1.name != p2.name)
        return p1.name < p2.name;
    return p1.type < p2.type;
}


/// Wrapper for QGLShader (QOpenGLShader), with functionality added to parse
/// the list of uniform parameters.
class Shader
{
    public:
#ifdef DISPLAZ_USE_QT4
        Shader(QGLShader::ShaderType type, const QGLContext* context)
            : m_shader(type, context)
        { }
#else
        Shader(QOpenGLShader::ShaderType type, const QOpenGLContext* context)
                : m_shader(type, context)
        { }
#endif

        /// Return list of uniform shader parameters
        const QList<ShaderParam>& uniforms() const
        {
            return m_uniforms;
        }

        /// Return original source code.
        QByteArray sourceCode() const
        {
            return m_source;
        }

        /// Access to underlying shader
#ifdef DISPLAZ_USE_QT4
        QGLShader* shader()
        {
            return &m_shader;
        }
#else
        QOpenGLShader* shader()
        {
            return &m_shader;
        }
#endif

        bool compileSourceCode(const QByteArray& src);

    private:
        QList<ShaderParam> m_uniforms;
#ifdef DISPLAZ_USE_QT4
        QGLShader m_shader;  ///< Underlying shader
#else
        QOpenGLShader m_shader;  ///< Underlying shader
#endif
        QByteArray m_source; ///< Non-mangled source code
};


/// Wrapper around QGLShaderProgram (QOpenGLShaderProgram) providing parameter tweaking UI
///
/// When compiling a new shader, the shader source code is scanned for
/// annotations in the comments which indicate which uniform values should be
/// tweakable by the user.  An appropriate UI editor widget is automatically
/// created for each such uniform value by a call to setupParameterUI().
///
class ShaderProgram : public QObject
{
    Q_OBJECT

    public:
        ShaderProgram(const QGLContext * context, QObject* parent = 0);

        /// Access to the underlying shader program
#idef DISPLAZ_USE_QT4
        QGLShaderProgram& shaderProgram() { return *m_shaderProgram; }
#else
        QOpenGLShaderProgram& shaderProgram() { return *m_shaderProgram; }
#endif

        /// Set up UI for the shader
        void setupParameterUI(QWidget* parentWidget);
        /// Send current uniform values to the underlying OpenGL shader
        void setUniforms();

        /// Reset the context
#idef DISPLAZ_USE_QT4
        void setContext(const QGLContext* context);
#else
        void setContext(const QOpenGLContext* context);
#endif

        /// Read shader source from given file and call setShader()
        bool setShaderFromSourceFile(QString fileName);

        /// Get shader source code
        QByteArray shaderSource() const;

        /// Return true if shader program is ready to use
        bool isValid() const { return static_cast<bool>(m_shaderProgram); }

    public slots:
        /// Set, compile and link shader source.
        ///
        /// Retain old shader if compilation or linking fails
        ///
        /// The shader source should contain both vertex and fragment shaders,
        /// separated inside #ifdef blocks using the macros VERTEX_SHADER and
        /// FRAGMENT_SHADER, which will be defined as appropriate when
        /// compiling the individual shader types.
        bool setShader(QString src);

    signals:
        /// Emitted when the list of user-settable uniform parameters to this
        /// shader change.
        ///
        /// The listening object should take this as a hint to update the UI.
        void paramsChanged();

        /// Emitted when the shader source code is updated.
        void shaderChanged();

        /// Emitted when a value of one of the current parameters changes
        ///
        /// Listeners should take this as a hint that the scene should be
        /// updated.
        void uniformValuesChanged();

    private slots:
        void setUniformValue(int value);
        void setUniformValue(double value);

    private:
        void setupParameters();

#idef DISPLAZ_USE_QT4
        const QGLContext* m_context;
#else
        const QOpenGLContext* m_context;
#endif
        double m_pointSize;
        double m_exposure;
        double m_contrast;
        int m_selector;
        typedef QMap<ShaderParam,QVariant> ParamMap;
        ParamMap m_params;
        std::unique_ptr<Shader> m_vertexShader;
        std::unique_ptr<Shader> m_fragmentShader;
#idef DISPLAZ_USE_QT4
        std::unique_ptr<QGLShaderProgram> m_shaderProgram;
#else
        std::unique_ptr<QOpenGLShaderProgram> m_shaderProgram;
#endif
};


#endif // SHADER_H_INCLUDED
