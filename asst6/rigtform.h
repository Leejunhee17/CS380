#ifndef RIGTFORM_H
#define RIGTFORM_H

#include <iostream>
#include <cassert>

#include "matrix4.h"
#include "quat.h"

class RigTForm {
  Cvec3 t_; // translation component
  Quat r_;  // rotation component represented as a quaternion

public:
  RigTForm() : t_(0) {
    assert(norm2(Quat(1,0,0,0) - r_) < CS175_EPS2);
  }

  RigTForm(const Cvec3& t, const Quat& r) {
    //TODO
      t_ = t;
      r_ = r;
  }

  explicit RigTForm(const Cvec3& t) {
    // TODO
      t_ = t;
  }

  explicit RigTForm(const Quat& r) {
    // TODO
      r_ = r;
  }

  Cvec3 getTranslation() const {
    return t_;
  }

  Quat getRotation() const {
    return r_;
  }

  RigTForm& setTranslation(const Cvec3& t) {
    t_ = t;
    return *this;
  }

  RigTForm& setRotation(const Quat& r) {
    r_ = r;
    return *this;
  }

  Cvec4 operator * (const Cvec4& a) const {
      // TODO
      const Cvec4 t = Cvec4(a[0] + t_[0], a[1] + t_[1], a[2] + t_[2], a[3]); // transform
      Cvec4 r = r_ * t; // rotation
      return r;
  }

  RigTForm operator * (const RigTForm& a) const {
    // TODO
      RigTForm RT;
      Cvec4 rt = r_ * Cvec4(a.t_[0], a.t_[1], a.t_[2], 0); // r1 * t2
      RT.t_ = t_ + Cvec3(rt[0], rt[1], rt[2]); // t1 + r1 * t2
      RT.r_ = r_ * a.r_; // r1 * r2
      return RT;
  }
};

inline RigTForm inv(const RigTForm& tform) {
  // TODO
    RigTForm RT;
    RT.setRotation(inv(tform.getRotation())); // Rotation is jus inv of quat
    Cvec3 t = tform.getTranslation(); // t2
    Cvec4 rt = RT.getRotation() * Cvec4(t[0], t[1], t[2], 0); // r1 * t2
    RT.setTranslation(Cvec3(-rt[0], -rt[1], -rt[2])); // Transformation make t1 + r1 * t2 = 0
    return RT;
}

inline RigTForm transFact(const RigTForm& tform) {
  return RigTForm(tform.getTranslation());
}

inline RigTForm linFact(const RigTForm& tform) {
  return RigTForm(tform.getRotation());
}

inline Matrix4 rigTFormToMatrix(const RigTForm& tform) {
  // TODO
    Matrix4 T, R; // create I matrix
    Cvec3 t = tform.getTranslation();
    const Quat q = tform.getRotation();
    T(0, 3) = t[0];
    T(1, 3) = t[1];
    T(2, 3) = t[2];
    R = quatToMatrix(q);
    return T * R;
}

inline RigTForm linInterpolation(const RigTForm& tform1, const RigTForm& tform2, float a) {
    Cvec3 t1 = tform1.getTranslation(), t2 = tform2.getTranslation(), t;
    Quat q1 = tform1.getRotation(), q2 = tform2.getRotation(), q;
    Quat v = q2 * inv(q1);
    if (v[0] < 0) {
        v *= -1;
    }
    Cvec3 k = Cvec3(v[1], v[2], v[3]);
    double rad = atan2(norm(k), v[0]);
    //std::cout << "interpolate rad: " << rad << std::endl;
    if (rad != 0) {
        k /= sin(rad);
        k *= sin(a * rad);
    }
    q = Quat(cos(a * rad), k) * q1;
    const RigTForm tform(t1 * (1 - a) + t2 * a, q);
    return tform;
}

#endif
