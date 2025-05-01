// Copyright 2015, Christopher J. Foster and the other displaz contributors.
// Use of this code is governed by the BSD-style license found in LICENSE.txt

#include "Camera.h"

Imath::M44d Camera::projectionMatrix() const
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

Imath::M44d Camera::viewMatrix() const
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

Imath::M44d Camera::viewportMatrix() const
{
    QMatrix4x4 m;
    m.translate(m_viewport.x(), m_viewport.y(), 0);
    m.scale(0.5*m_viewport.width(), -0.5*m_viewport.height(), 1);
    m.translate(1, -1, 0);
    return qt2exr(m);
}

Imath::M44d Camera::rotationMatrix() const
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

Imath::V3d Camera::mouseMovePoint(Imath::V3d p, QPoint mouseMovement,
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

void Camera::setViewport(QRect rect)
{
    m_viewport = rect;
    emit viewChanged();
}

void Camera::setFieldOfView(float fov)
{
    m_fieldOfView = fov;
    emit projectionChanged();
}

void Camera::setCenter(Imath::V3d center)
{
    m_center = exr2qt(center);
    if (m_mode != NAVIGATION)
    {
        m_position = exr2qt(Imath::V3d(0,0,0)*viewMatrix().inverse());
    }
    emit viewChanged();
}

void Camera::setEyeToCenterDistance(float distance)
{
    m_distance = distance;
    emit viewChanged();
}

void Camera::setRotation(QQuaternion rotation)
{
    m_rotation = rotation;
    emit viewChanged();
}

void Camera::setRotation(QMatrix3x3 rot3x3)
{
    m_rotation = QQuaternion::fromRotationMatrix(rot3x3);
    emit viewChanged();
}

void Camera::setTrackballInteraction(bool trackballInteraction)
{
    m_mode = trackballInteraction ? TRACKBALL : TURNTABLE;
}

void Camera::mouseDrag(QPoint prevPos, QPoint currPos, bool zoom)
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

QQuaternion Camera::turntableRotation(QPoint prevPos, QPoint currPos, QQuaternion initialRot) const
{
    float dx = 4*float(currPos.x() - prevPos.x())/m_viewport.width();
    float dy = 4*float(currPos.y() - prevPos.y())/m_viewport.height();
    QQuaternion r1 = QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), 180/M_PI*dy);
    QQuaternion r2 = QQuaternion::fromAxisAndAngle(QVector3D(0,0,1), 180/M_PI*dx);
    return r1 * initialRot * r2;
}

QQuaternion Camera::trackballRotation(QPoint prevPos, QPoint currPos) const
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

QVector3D Camera::trackballVector(QPoint pos, float r) const
{
    // Map x & y mouse locations to the interval [-1,1]
    float x =  2.0*(pos.x() - m_viewport.center().x())/m_viewport.width();
    float y = -2.0*(pos.y() - m_viewport.center().y())/m_viewport.height();
    float d = sqrt(x*x + y*y);
    // get projected z coordinate -      sphere : cone
    float z = (d < r/M_SQRT2) ? sqrt(r*r - d*d) : r*M_SQRT2 - d;
    return QVector3D(x,y,z);
}

void Camera::navigateSlower()
{
    if (m_speedMode > 0)
    {
        m_speedMode--;
    }
}

void Camera::navigateFaster()
{
    if (m_speedMode < m_speed.size() - 2)
    {
        m_speedMode++;
    }
}

void Camera::updateNavigation(const QSet<int>& keyboard)
{
    const bool keyUp   = keyboard.contains(Qt::Key_E);
    const bool keyDown = keyboard.contains(Qt::Key_Q);

    QVector3D dir;
    dir.setX(cos(qDegreesToRadians(m_yaw)) * cos(qDegreesToRadians(m_pitch)));
    dir.setY(sin(qDegreesToRadians(m_yaw)) * cos(qDegreesToRadians(m_pitch)));
    dir.setZ(sin(qDegreesToRadians(m_pitch)));
    dir.normalize();

    QVector3D front(dir);
    if (keyUp || keyDown)
    {
        front.setZ(0.0f);
    }

    const QVector3D up = QVector3D(0, 0, 1);
    const QVector3D right = QVector3D::crossProduct(front, up).normalized();
    const float speed = m_speed[m_speedMode];

    // Elapsed time between updates
    const auto then = m_navigationTime;
    const auto now = std::chrono::steady_clock::now();
    const float duration = std::min(0.1f, std::chrono::duration<float>(now - then).count());
    m_navigationTime = now;

    // Update camera position

    if (keyboard.contains(Qt::Key_W))     m_position += front * duration * speed;
    if (keyboard.contains(Qt::Key_S))     m_position -= front * duration * speed;
    if (keyboard.contains(Qt::Key_A))     m_position -= right * duration * speed;
    if (keyboard.contains(Qt::Key_D))     m_position += right * duration * speed;
    if (keyUp)                            m_position +=    up * duration * speed * 0.5;
    if (keyDown)                          m_position -=    up * duration * speed * 0.5;

    // Update camera examiner to match
    m_center = m_position + dir;
    m_distance = 1.0f;
    m_rotation = QQuaternion::fromDirection(dir, QVector3D(0, 0, 1));    
}
