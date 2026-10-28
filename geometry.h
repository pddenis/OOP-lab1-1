#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <cmath>
#include <stdexcept>

const double kEpsilon = 1e-9;

inline bool nearlyEqual(double a, double b) {
    return std::abs(a - b) < kEpsilon;
}

class Vector2D {
private:
    double x_;
    double y_;

public:
    Vector2D() : x_(0.0), y_(0.0) {}
    Vector2D(double x, double y) : x_(x), y_(y) {}

    double getX() const { return x_; }
    double getY() const { return y_; }

    Vector2D operator-() const { return Vector2D(-x_, -y_); }
    Vector2D operator+(const Vector2D& o) const { return Vector2D(x_ + o.x_, y_ + o.y_); }
    Vector2D operator-(const Vector2D& o) const { return Vector2D(x_ - o.x_, y_ - o.y_); }
    Vector2D operator*(double k) const { return Vector2D(x_ * k, y_ * k); }

    Vector2D operator/(double k) const {
        if (k == 0.0) throw std::invalid_argument("Vector2D: division by zero");
        return Vector2D(x_ / k, y_ / k);
    }

    bool operator==(const Vector2D& o) const { return nearlyEqual(x_, o.x_) && nearlyEqual(y_, o.y_); }
    bool operator!=(const Vector2D& o) const { return !(*this == o); }
};

inline Vector2D operator*(double k, const Vector2D& v) { return v * k; }

class Point2D {
private:
    double x_;
    double y_;

public:
    Point2D() : x_(0.0), y_(0.0) {}
    Point2D(double x, double y) : x_(x), y_(y) {}

    double getX() const { return x_; }
    double getY() const { return y_; }

    Point2D operator+(const Vector2D& v) const { return Point2D(x_ + v.getX(), y_ + v.getY()); }
    Point2D operator-(const Vector2D& v) const { return Point2D(x_ - v.getX(), y_ - v.getY()); }
    Vector2D operator-(const Point2D& o) const { return Vector2D(x_ - o.x_, y_ - o.y_); }

    bool operator==(const Point2D& o) const { return nearlyEqual(x_, o.x_) && nearlyEqual(y_, o.y_); }
    bool operator!=(const Point2D& o) const { return !(*this == o); }
};

class Rect {
private:
    Point2D top_left_;
    Vector2D size_;

public:
    Rect(const Point2D& top_left, const Vector2D& size) : top_left_(top_left), size_(size) {
        if (size_.getX() < 0.0 || size_.getY() < 0.0) {
            throw std::invalid_argument("Rect: size must be non-negative");
        }
    }

    Point2D getTopLeft() const { return top_left_; }
    Vector2D getSize() const { return size_; }
    double getLeft() const { return top_left_.getX(); }
    double getTop() const { return top_left_.getY(); }
    double getRight() const { return top_left_.getX() + size_.getX(); }
    double getBottom() const { return top_left_.getY() + size_.getY(); }

    bool contains(const Point2D& p) const {
        return p.getX() >= getLeft() && p.getX() <= getRight() &&
            p.getY() >= getTop() && p.getY() <= getBottom();
    }

    bool intersects(const Rect& o) const {
        return !(getRight() < o.getLeft() || getLeft() > o.getRight() ||
            getBottom() < o.getTop() || getTop() > o.getBottom());
    }

    bool operator==(const Rect& o) const { return top_left_ == o.top_left_ && size_ == o.size_; }
    bool operator!=(const Rect& o) const { return !(*this == o); }
};

#endif // GEOMETRY_H