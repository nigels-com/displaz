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
/// a quaternion orientation and a scalar distance.
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
/// Keyboard:  WSAD - move camera forward (W), back (S), left (A) and right (D)
/// Mouse:     Yaw and pitch angle adjustment

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
        Imath::M44d projectionMatrix() const
        {
            // TODO: If height is zero?
            const float aspect = static_cast<float>(m_viewport.width()) / m_viewport.height();

            switch (m_mode)
            {
                case TRACKBALL:
                case TURNTABLE:
                default:
                {
                    // Simple heuristic for clipping planes: use a large range of
                    // depths scaled by the distance of interest m_dist.  The large
                    // range must be traded off against finite precision of the depth
                    // buffer which can lead to z-fighting when rendering objects at a
                    // similar depth.
                    float clipNear = 1e-2*m_distance;
                    float clipFar = 1e+5*m_distance;
                    QMatrix4x4 m;
                    m.perspective(m_fieldOfView, aspect, clipNear, clipFar);
                    return qt2exr(m);
                }

                case NAVIGATION:
                {
                    QMatrix4x4 m;
                    m.perspective(m_fieldOfView, aspect, 0.01, 500.0);  // 1cm, 500m
                    return qt2exr(m);
                }
            }
        }

        /// Get view transformation from world to camera coordinates
        Imath::M44d viewMatrix() const
        {
            switch (m_mode)
            {
                case TRACKBALL:
                case TURNTABLE:
                default:
                {
                    QMatrix4x4 m;
                    m.translate(0, 0, -m_distance);
                    m.rotate(m_rotation);
                    if (m_reverseHandedness)
                    {
                        m.scale(1,1,-1);
                    }
                    return qt2exr(m).translate(qt2exr(-m_center));
                }

                case NAVIGATION:
                {
                    QVector3D front;
                    front.setX(cos(qDegreesToRadians(m_yaw)) * cos(qDegreesToRadians(m_pitch)));
                    front.setY(sin(qDegreesToRadians(m_yaw)) * cos(qDegreesToRadians(m_pitch)));
                    front.setZ(sin(qDegreesToRadians(m_pitch)));
                    front = front.normalized();

                    QMatrix4x4 m;
                    m.lookAt(m_position, m_position + front, QVector3D(0, 0, 1));
                    return qt2exr(m);
                }
            }
        }

        /// Get transformation from screen coords to viewport coords
        ///
        /// The viewport coordinates are in pixels, with 0,0 at the top left
        /// and width,height at the bottom right.
        Imath::M44d viewportMatrix() const
        {
            QMatrix4x4 m;
            m.translate(m_viewport.x(), m_viewport.y(), 0);
            m.scale(0.5*m_viewport.width(), -0.5*m_viewport.height(), 1);
            m.translate(1, -1, 0);
            return qt2exr(m);
        }

        /// Get view rotation-only matrix
        Imath::M44d rotationMatrix() const
        {
            switch (m_mode)
            {
                case TRACKBALL:
                case TURNTABLE:
                default:
                {
                    QMatrix4x4 m;
                    m.rotate(m_rotation);
                    if (m_reverseHandedness)
                    {
                        m.scale(1,1,-1);
                    }
                    return qt2exr(m);
                }

                case NAVIGATION:
                {
                    QVector3D front;
                    front.setX(cos(qDegreesToRadians(m_yaw)) * cos(qDegreesToRadians(m_pitch)));
                    front.setY(sin(qDegreesToRadians(m_yaw)) * cos(qDegreesToRadians(m_pitch)));
                    front.setZ(sin(qDegreesToRadians(m_pitch)));
                    front = front.normalized();

                    QQuaternion q = QQuaternion::fromDirection(front, QVector3D(0, 0, 1));
                    return qt2exr(QMatrix4x4(q.toRotationMatrix()));
                }
            }
        }

        // Examiner parameters
        QVector3D   m_center;               ///< center of view for camera
        QQuaternion m_rotation;             ///< camera rotation about center
        float       m_distance = 5.0f;      ///< distance from center of view

        // Navigation parameter
        QVector3D   m_position;             ///< camera position
        float       m_yaw   = 0.0f;         ///< XY plane yaw angle (degrees)
        float       m_pitch = 0.0f;         ///< pitch angle (degrees) towards +Z or -Z

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
        Imath::V3d mouseMovePoint(Imath::V3d p, QPoint mouseMovement,
                                  bool zooming) const
        {
            const float dx = 2*static_cast<float>(mouseMovement.x()) / m_viewport.width();
            const float dy = 2*static_cast<float>(-mouseMovement.y()) / m_viewport.height();
            if (zooming)
            {
                Imath::M44d view = viewMatrix();
                return (p*view*std::exp(dy)) * view.inverse();
            }
            else
            {
                Imath::M44d proj = viewMatrix()*projectionMatrix();
                return (p*proj + Imath::V3d(dx, dy, 0)) * proj.inverse();
            }
        }

    public slots:
        void setViewport(QRect rect)
        {
            m_viewport = rect;
            emit viewChanged();
        }
        void setFieldOfView(float fov)
        {
            m_fieldOfView = fov;
            emit projectionChanged();
        }
        void setCenter(Imath::V3d center)
        {
            m_center = exr2qt(center);
            if (m_mode != NAVIGATION)
            {
                m_position = exr2qt(Imath::V3d(0,0,0)*viewMatrix().inverse());
            }
            emit viewChanged();
        }
        void setEyeToCenterDistance(float distance)
        {
            m_distance = distance;
            emit viewChanged();
        }
        void setRotation(QQuaternion rotation)
        {
            m_rotation = rotation;
            emit viewChanged();
        }
        void setRotation(QMatrix3x3 rot3x3)
        {
            m_rotation = QQuaternion::fromRotationMatrix(rot3x3);
            emit viewChanged();
        }

        void setTrackballInteraction(bool trackballInteraction)
        {
            m_mode = trackballInteraction ? TRACKBALL : TURNTABLE;
        }

        /// Move the camera using a drag of the mouse.
        ///
        /// The previous and current positions of the mouse during the move are
        /// given by prevPos and currPos.  By default this rotates the camera
        /// around the center, but if zoom is true, the camera position is
        /// zoomed in toward the center instead.
        void mouseDrag(QPoint prevPos, QPoint currPos, bool zoom = false)
        {
            if (zoom)
            {
                // exponential zooming gives scale-independent sensitivity
                float dy = float(currPos.y() - prevPos.y())/m_viewport.height();
                const float zoomSpeed = 3.0f;
                m_distance *= std::exp(zoomSpeed*dy);
            }
            else
            {
                if (m_mode == TRACKBALL)
                {
                    m_rotation = trackballRotation(prevPos, currPos) * m_rotation;
                }
                else
                {
                    // TODO: Not sure this is entirely consistent if the user
                    // switches between trackball and turntable modes...
                    m_rotation = turntableRotation(prevPos, currPos, m_rotation);
                }
                m_rotation.normalize();
            }
            emit viewChanged();
        }

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
                                      QQuaternion initialRot) const
        {
            float dx = 4*float(currPos.x() - prevPos.x())/m_viewport.width();
            float dy = 4*float(currPos.y() - prevPos.y())/m_viewport.height();
            QQuaternion r1 = QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), 180/M_PI*dy);
            QQuaternion r2 = QQuaternion::fromAxisAndAngle(QVector3D(0,0,1), 180/M_PI*dx);
            return r1 * initialRot * r2;
        }

        /// Get rotation of trackball.
        ///
        /// currPos is the new position of the mouse pointer; prevPos is the
        /// previous position.  For the parameters chosen here, moving the
        /// mouse around any closed curve will give a composite rotation of the
        /// identity.  This is rather important for the predictability of the
        /// user interface.
        QQuaternion trackballRotation(QPoint prevPos, QPoint currPos) const
        {
            // Compute the new and previous positions of the cursor on a 3D
            // virtual trackball.  Form a rotation around the axis which would
            // take the previous position to the new position.
            const float trackballRadius = 1.1; // as in blender
            QVector3D p1 = trackballVector(prevPos, trackballRadius);
            QVector3D p2 = trackballVector(currPos, trackballRadius);
            QVector3D axis = QVector3D::crossProduct(p1, p2);
            // The rotation angle between p1 and p2 in radians is
            //
            // std::asin(axis.length()/(p1.length()*p2.length()));
            //
            // However, it's preferable to use two times this angle for the
            // rotation instead: It's a remarkable fact that the total rotation
            // after moving the mouse through any closed path is then the
            // identity, which means the model returns exactly to its previous
            // orientation when you return the mouse to the starting position.
            float angle = 2*std::asin(axis.length()/(p1.length()*p2.length()));
            return QQuaternion::fromAxisAndAngle(axis, 180/M_PI*angle);
        }

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
        QVector3D trackballVector(QPoint pos, float r) const
        {
            // Map x & y mouse locations to the interval [-1,1]
            float x =  2.0*(pos.x() - m_viewport.center().x())/m_viewport.width();
            float y = -2.0*(pos.y() - m_viewport.center().y())/m_viewport.height();
            float d = sqrt(x*x + y*y);
            // get projected z coordinate -      sphere : cone
            float z = (d < r/M_SQRT2) ? sqrt(r*r - d*d) : r*M_SQRT2 - d;
            return QVector3D(x,y,z);
        }


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
