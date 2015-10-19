// Copyright 2015, Christopher J. Foster and the other displaz contributors.
// Use of this code is governed by the BSD-style license found in LICENSE.txt

#ifndef DISPLAZ_VIEW3D_H_INCLUDED
#define DISPLAZ_VIEW3D_H_INCLUDED

#include <cmath>
#include <vector>
#include <memory>

#ifdef DISPLAZ_USE_QT4
    #include <GL/glew.h>
    #include <QGLWidget>
#else
    #include <QOpenGLFunctions_3_2_Core>
    //#include <QOpenGLFunctions>
    #include <QOpenGLWidget>
#endif

#include <QModelIndex>

#include "DrawCostModel.h"
#include "interactivecamera.h"
#include "geometrycollection.h"
#include "glutil.h"

#ifdef DISPLAZ_USE_QT4
    class QGLShaderProgram;
    class QGLFramebufferObject;
#else
    class QOpenGLShaderProgram;
    class QOpenGLFramebufferObject;
#endif
class QItemSelectionModel;
class QTimer;

class ShaderProgram;
struct TransformState;

//------------------------------------------------------------------------------
/// OpenGL-based viewer widget for point clouds
#ifdef DISPLAZ_USE_QT4
class View3D : public QGLWidget
#else
class View3D : public QOpenGLWidget
#endif
{
    Q_OBJECT
    public:
        View3D(GeometryCollection* geometries, QWidget *parent = NULL);
        ~View3D();

        /// Return shader used for displaying points
        ShaderProgram& shaderProgram() const { return *m_shaderProgram; }

        void setShaderParamsUIWidget(QWidget* widget);

        InteractiveCamera& camera() { return m_camera; }

        QColor background() const { return m_backgroundColor; }

        Imath::V3d cursorPos() const { return m_cursorPos; }

        /// Return current selection of loaded files
        const QItemSelectionModel* selectionModel() const { return m_selectionModel; }
        QItemSelectionModel* selectionModel() { return m_selectionModel; }
        void setSelectionModel(QItemSelectionModel* selectionModel);

#ifndef DISPLAZ_USE_QT4
    signals:
        void initialisedGL();
#endif

    public slots:
        /// Set the backgroud color
        void setBackground(QColor col);
        void toggleDrawBoundingBoxes();
        void toggleDrawCursor();
        void toggleDrawAxes();
        void toggleCameraMode();
        /// Centre on loaded geometry file at the given index
        void centerOnGeometry(const QModelIndex& index);

    protected:
        // Qt OpenGL callbacks
        void initializeGL();
        void resizeGL(int w, int h);
        void paintGL();

        // Qt event callbacks
        void mousePressEvent(QMouseEvent* event);
        void mouseMoveEvent(QMouseEvent* event);
        void wheelEvent(QWheelEvent* event);
        void keyPressEvent(QKeyEvent* event);

    private slots:
        void restartRender();
        void setupShaderParamUI();

        void geometryChanged();
        void geometryInserted(const QModelIndex&, int firstRow, int lastRow);

    private:
#ifdef DISPLAZ_USE_QT4
        std::unique_ptr<QGLFramebufferObject> allocIncrementalFramebuffer(int w, int h) const;
#else
        std::unique_ptr<QOpenGLFramebufferObject> allocIncrementalFramebuffer(int w, int h) const;
#endif

        void drawCursor(const TransformState& transState, const V3d& P,
                        float cursorRadius, float centerPointRadius) const;

        void drawAxes() const;

        DrawCount drawPoints(const TransformState& transState,
                             const std::vector<const Geometry*>& geoms,
                             double quality, bool incrementalDraw);

        void drawMeshes(const TransformState& transState,
                        const std::vector<const Geometry*>& geoms) const;

        Imath::V3d guessClickPosition(const QPoint& clickPos);

        bool snapToGeometry(const Imath::V3d& pos, double normalScaling,
                            Imath::V3d* newPos, QString* pointInfo);

        std::vector<const Geometry*> selectedGeometry() const;

        /// Mouse-based camera positioning
        InteractiveCamera m_camera;
        QPoint m_prevMousePos;
        Qt::MouseButton m_mouseButton;
        bool m_middleButton;
        /// Position of 3D cursor
        V3d m_cursorPos;
        V3d m_prevCursorSnap;
        /// Background color for drawing
        QColor m_backgroundColor;
        /// Option to draw bounding boxes of point clouds
        bool m_drawBoundingBoxes;
        bool m_drawCursor;
        bool m_drawAxes;
        /// If true, OpenGL initialization didn't work properly
        bool m_badOpenGL;
        /// Shader for point clouds
        std::unique_ptr<ShaderProgram> m_shaderProgram;
        /// Shaders for polygonal geometry
        std::unique_ptr<ShaderProgram> m_meshFaceShader;
        std::unique_ptr<ShaderProgram> m_meshEdgeShader;
        /// Collection of geometries
        GeometryCollection* m_geometries;
        QItemSelectionModel* m_selectionModel;
        /// UI widget for shader
        QWidget* m_shaderParamsUI;
        /// Timer for next incremental frame
        QTimer* m_incrementalFrameTimer;
#ifdef DISPLAZ_USE_QT4
        std::unique_ptr<QGLFramebufferObject> m_incrementalFramebuffer;
#else
        std::unique_ptr<QOpenGLFramebufferObject> m_incrementalFramebuffer;
#endif
        bool m_incrementalDraw;
        /// Controller for amount of geometry to draw
        DrawCostModel m_drawCostModel;
        /// GL textures
        Texture m_drawAxesBackground;
        Texture m_drawAxesLabelX;
        Texture m_drawAxesLabelY;
        Texture m_drawAxesLabelZ;
};



#endif // DISPLAZ_VIEW3D_H_INCLUDED

// vi: set et:
