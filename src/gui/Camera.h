// Copyright 2015, Christopher J. Foster and the other displaz contributors.
// Use of this code is governed by the BSD-style license found in LICENSE.txt

/// \author Chris Foster [chris42f [at] gmail (dot) com]

#pragma once

#ifdef _MSC_VER
#   ifndef _USE_MATH_DEFINES
#       define _USE_MATH_DEFINES
#   endif
#endif

#include <cmath>
#include <chrono>

#include <QRect>
#include <QtMath>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#endif

#include <Imath/ImathVec.h>
#include <Imath/ImathMatrix.h>

#ifdef __clang__
#pragma GCC diagnostic pop
#endif

// TMP DEBUG
#include "tinyformat.h"

/// Camera controller for mouse and keyboard scene navigation
///
/// Two examiner modes for inspecting objects.
/// An additional navigation mode with keyboard WSAD and mouse-look.
/// Integrated and unified here for ease of switching between these as needed.
///
/// In "examiner" modes the formulation is a center of interest,
/// a quaternion viewing orientation and a scalar distance.
///
/// Two rotation models:
///
/// 1) The virtual trackball model - this does not impose any particular "up
///    vector" on the user.
/// 2) The turntable model, which is potentially more intuitive when the data
///    has a natural vertical direction.
///
/// In "navigation" mode a position of the camera, horizontal yaw angle
/// and vertical pitch angle.
///
/// Keyboard:
///    W move forward
///    S move backward
///    A move left
///    D move right
///    Q move down
///    E move up
///
/// Mouse:
///    Yaw and pitch angle adjustment

enum CameraMode
{
    TRACKBALL = 0,
    TURNTABLE = 1,
    NAVIGATION = 2
};

class Camera : public QObject
{
    Q_OBJECT

    public:
        /// Construct camera; if reverseHandedness is true, the viewing
        /// transformation will invert the z-axis.  If used with OpenGL (which
        /// is right handed by default) this gives us a left handed coordinate
        /// system.
        Camera() = default;

        /// Get the projection from camera to screen coordinates
        Imath::M44d projectionMatrix() const;

        /// Get view transformation from world to camera coordinates
        Imath::M44d viewMatrix() const;

        /// Get transformation from screen coords to viewport coords
        ///
        /// The viewport coordinates are in pixels, with 0,0 at the top left
        /// and width,height at the bottom right.
        Imath::M44d viewportMatrix() const;

        /// Get view rotation-only matrix
        Imath::M44d rotationMatrix() const;


        // Examiner parameters
        QVector3D   m_center;               ///< center of view for camera
        QQuaternion m_rotation;             ///< camera rotation about center
        float       m_distance = 5.0f;      ///< distance from center of view

        // Navigation parameter
        QVector3D   m_position;             ///< camera position
        float       m_yaw   = 0.0f;         ///< XY plane yaw angle (degrees)
        float       m_pitch = 0.0f;         ///< pitch angle (degrees) towards +Z or -Z

        size_t      m_speedMode = 2;
        const std::vector<float> m_speed = { 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 50.0, 100.0, 200.0 }; ///< speed in m/s

        std::chrono::time_point<std::chrono::steady_clock> m_navigationTime;

        // Projection variables
        float       m_fieldOfView = 60.0f;  ///< field of view in degrees
        QRect       m_viewport;             ///< rectangle we'll drag inside

        // Additional modes
        bool        m_reverseHandedness = false; ///< Reverse the handedness of the coordinate system
        CameraMode  m_mode = TURNTABLE;          ///< True for trackball style, false for turntable

        Imath::V3d center()   const { return qt2exr(m_center);   }
        Imath::V3d position() const { return qt2exr(m_position); }

        /// Grab and move a point in the 3D space with the mouse.
        ///
        /// p is the point to move in world coordinates.  mouseMovement is a
        /// vector moved by the mouse inside the 2D viewport.  If zooming is
        /// true, the point will be moved along the viewing direction rather
        /// than perpendicular to it.
        Imath::V3d mouseMovePoint(Imath::V3d p, QPoint mouseMovement, bool zooming) const;

    public slots:
        void setViewport(QRect rect);
        void setFieldOfView(float fov);
        void setCenter(Imath::V3d center);
        void setEyeToCenterDistance(float distance);
        void setRotation(QQuaternion rotation);
        void setRotation(QMatrix3x3 rot3x3);
        void setTrackballInteraction(bool trackballInteraction);

        /// Move the camera using a drag of the mouse.
        ///
        /// The previous and current positions of the mouse during the move are
        /// given by prevPos and currPos.  By default this rotates the camera
        /// around the center, but if zoom is true, the camera position is
        /// zoomed in toward the center instead.
        void mouseDrag(QPoint prevPos, QPoint currPos, bool zoom = false);

    signals:
        /// The projection matrix has changed
        void projectionChanged();
        /// The view matrix has changed
        void viewChanged();

    private:
        /// Perform "turntable" style rotation on current orientation
        ///
        /// currPos is the new position of the mouse pointer; prevPos is the
        /// previous position.  initialRot is the current camera orientation,
        /// which will be modified by the mouse movement and returned.
        QQuaternion turntableRotation(QPoint prevPos, QPoint currPos,
                                      QQuaternion initialRot) const;

        /// Get rotation of trackball.
        ///
        /// currPos is the new position of the mouse pointer; prevPos is the
        /// previous position.  For the parameters chosen here, moving the
        /// mouse around any closed curve will give a composite rotation of the
        /// identity.  This is rather important for the predictability of the
        /// user interface.
        QQuaternion trackballRotation(QPoint prevPos, QPoint currPos) const;

        /// Get position on surface of a virtual trackball
        ///
        /// The classic trackball camera control projects a position on the
        /// screen orthogonally onto a sphere to compute a 3D cursor position.
        /// The sphere is centred at the middle of the screen, with some
        /// diameter chosen to taste but roughly the width of the screen.
        ///
        /// This projection doesn't make sense at all points in the plane, so
        /// we join a cone smoothly to the sphere at distance r/sqrt(2) so that
        /// all the points at larger radii are projected onto the cone instead.
        ///
        /// Historical note: The trackball code for blender's default camera
        /// seems to have been inspired by GLUT's trackball.c by Gavin Bell
        /// (aka Gavin Andresen).  Those codes use a hyperboloid rather than a
        /// cone, but I've used a cone here to improve mouse sensitivity near
        /// the edge of the view port without resorting to the no-asin() hack
        /// used by blender. - CJF
        QVector3D trackballVector(QPoint pos, float r) const;

        //----------------------------------------------------------------------
        // Qt->Ilmbase vector/matrix conversions.
        // TODO: Refactor everything to use a consistent set of matrix/vector
        // classes (replace Ilmbase with Eigen?)
        template<typename T>
        static inline QVector3D exr2qt(const Imath::Vec3<T>& v)
        {
            return QVector3D(v.x, v.y, v.z);
        }

        static inline Imath::V3d qt2exr(const QVector3D& v)
        {
            return Imath::V3d(v.x(), v.y(), v.z());
        }

        static inline Imath::M44d qt2exr(const QMatrix4x4& m)
        {
            Imath::M44d mOut;
            for(int j = 0; j < 4; ++j)
            for(int i = 0; i < 4; ++i)
                mOut[j][i] = m.constData()[4*j + i];
            return mOut;
        }
};
