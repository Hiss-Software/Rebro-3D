#pragma once

#include <QPointF>
#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cmath>

enum CommandKey { CmdNone = 0, CmdG = 1, CmdR = 2, CmdDelete = 3, CmdEscape = 4, CmdV = 5 };

inline glm::mat4 rotAxis(float deg, const glm::vec3& ax) {
    return glm::rotate(glm::mat4(1.0f), glm::radians(deg), ax);
}

inline glm::mat4 mapXToAxis(int a) {
    switch (a) {
    case 1: return rotAxis(90.0f,  glm::vec3(0,0,1));
    case 2: return rotAxis(-90.0f, glm::vec3(0,1,0));
    default: return glm::mat4(1.0f);
    }
}

inline glm::mat4 mapYToAxis(int a) {
    switch (a) {
    case 0: return rotAxis(-90.0f, glm::vec3(0,0,1));
    case 2: return rotAxis(90.0f,  glm::vec3(1,0,0));
    default: return glm::mat4(1.0f);
    }
}

inline glm::mat4 mapZToAxis(int a) {
    switch (a) {
    case 0: return rotAxis(90.0f,  glm::vec3(0,1,0));
    case 1: return rotAxis(-90.0f, glm::vec3(1,0,0));
    default: return glm::mat4(1.0f);
    }
}

inline bool rayAABB(const glm::vec3& orig, const glm::vec3& dir,
                    const glm::vec3& bmin, const glm::vec3& bmax, float& tOut)
{
    float tmin = -1e30f, tmax = 1e30f;
    for (int i = 0; i < 3; ++i) {
        if (std::abs(dir[i]) < 1e-8f) {
            if (orig[i] < bmin[i] || orig[i] > bmax[i]) return false;
        } else {
            float t1 = (bmin[i] - orig[i]) / dir[i];
            float t2 = (bmax[i] - orig[i]) / dir[i];
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return false;
        }
    }
    tOut = tmin >= 0.0f ? tmin : tmax;
    return tOut >= 0.0f;
}

inline float distPointSegment(const QPointF& p, const QPointF& a, const QPointF& b) {
    QPointF ab = b - a;
    float len2 = float(QPointF::dotProduct(ab, ab));
    if (len2 <= 1e-9f)
        return std::hypot(p.x() - a.x(), p.y() - a.y());
    float t = std::clamp(float(QPointF::dotProduct(p - a, ab)) / len2, 0.0f, 1.0f);
    QPointF c = a + ab * t;
    return std::hypot(p.x() - c.x(), p.y() - c.y());
}
