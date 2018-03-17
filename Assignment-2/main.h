#define PLANE_START_X -50
#define PLANE_END_X 50
#define PLANE_START_Y -50
#define PLANE_END_Y 50
#define PLANE_WIDTH (PLANE_END_X - PLANE_START_X)
#define PLANE_HEIGHT (PLANE_END_Y - PLANE_START_Y)
#define PLANE_Z 100.0 // Assuming this is constant since it eases up things
#define DEBUG 0
#define LIGHT_POS { 500, 500, 500 }
#define RESOLUTION_COEFF 10
#define AMBIENT_LIGHT 0.3
#define WHITE_COLOR color_t { 255, 255, 255, 0 }
#define PLANE_COLOR color_t { 255, 255, 255, AMBIENT_LIGHT }
#define EPSILON 0.00001

using namespace std;

/**
 * When we round the quadratic equation results to integers, there occurs
 * resulting points which are "too" close, but not equal. These points are
 * meant to be equal so we tolerate length between them.
 *
 * In other words, if two points have distance of <= CLOSENESS_TOLERANCE, we
 * treat them as same points
 */
#define CLOSENESS_TOLERANCE 10

/**
 * Representation of a position. Positions intentionally have
 * integer axes because it's very hard to identify point equality
 * with double axes.
 */
struct position_t {
  double x;
  double y;
  double z;
  position_t operator+(position_t other) {
    return position_t { this->x + other.x, this->y + other.y, this->z + other.z };
  }
  position_t operator-(position_t other) {
    return position_t { this->x - other.x, this->y - other.y, this->z - other.z };
  }
  double length() {
    return sqrt(this->x * this->x + this->y * this->y + this->z * this->z); 
  }
  void print() const {
    cout << "(" << this->x << ", " << this->y << ", " << this->z << ")" << endl;
  }
  bool operator==(position_t other) {
    return this->x == other.x && this->y == other.y && this->z == other.z;
  }
  bool too_close(position_t other) {
    return pow(this->x - other.x, 2) + pow(this->y - other.y, 2) + pow(this->z - other.z, 2) <= CLOSENESS_TOLERANCE;
  }
};

/**
 * Representation of a RGB color.
 */
struct color_t {
  int R;
  int G;
  int B;
  double lustre; // Fancy way of saying "the amount of light it has".
  bool operator==(color_t other) {
    return this->R == other.R && this->G == other.G && this->B == other.B && this->lustre == other.lustre;
  }
  void illuminate(double amount) {
    this->lustre = min(1.0, this->lustre + max(0.0, amount));
  }
  void print() const {
    cout << "(" << this->R << ", " << this->G << ", " << this->B << ", " << this->lustre << ")" << endl;
  }
};

/**
 * Helper for receiving white color.
 */
color_t white_color = color_t { 255, 255, 255, 1 };

/**
 * Origin point.
 */
position_t origin = position_t { 0.0, 0.0, 0.0 };

/**
 * Representation of a sphere.
 */
struct sphere_t {
  color_t color;
  position_t center;
  int radius;
};

/**
 * Representation of a direction vector. This is double because we often want
 * rays to be absolute to use in quadratic equations. We approximate this to be
 * a position if needed.
 */
struct direction_t {
  double x;
  double y;
  double z;
  position_t approximate() {
    return position_t { this->x, this->y, this->z };
  }
  double length() {
    return sqrt(this->x * this->x + this->y * this->y + this->z * this->z); 
  }
  int dot(direction_t other) {
    return this->x * other.x + this->y * other.y + this->z * other.z;
  }
  int dot(position_t other) {
    return this->x * other.x + this->y * other.y + this->z * other.z;
  }
  direction_t operator*(double coeff) {
    return direction_t { coeff * this->x, coeff * this->y, coeff * this->z };
  }
  double angle_cos_with(direction_t other) {
    return this->dot(other) / (this->length() * other.length());
  }
};

/**
 * Approximate a direction to be a position.
 */
direction_t pos_to_dir(position_t pos) {
  return direction_t { (double) pos.x, (double) pos.y, (double) pos.z };
}

struct plane_t {
  position_t point;
  direction_t normal_vector;
  color_t color;
};

struct input_data_t {
  vector<sphere_t> spheres;
  vector<position_t> light_positions;
  plane_t ground_plane;
};

/**
 * Vectors have origin and direction. This represents the formula (origin + t * direction).
 */
struct vector_t {
  position_t origin;
  direction_t direction;
};

class Shape {
  virtual direction_t calculateNormalVector(position_t) = 0;
  color_t color;
  Shape(color_t);
};

class Sphere : Shape {
  public:
    direction_t calculateNormalVector(position_t point) {
      return pos_to_dir(point - this->center);
    }
  private:
    position_t center;
    int radius;
};

class Plane : Shape {
  public:
    direction_t calculateNormalVector(position_t point) {
      return this->normal_vector; // Normal vector for planes are constant
    }
  private:
    direction_t normal_vector;
};

template<class T>
class Intersection {
  public:
    Intersection(T object, color_t color, position_t point);
    position_t getPoint() {
      return this->point;
    }
    direction_t getNormalVector() {
      return this->normal_vector;
    }
    color_t getColor() {
      return this->color;
    }
  private:
    direction_t normal_vector;
    position_t point;
    color_t color;
    T object;
};

template<typename T>
Intersection<T>::Intersection(T object, color_t color, position_t point) {
  this->object = object;
  this->color = color;
  this->point = point;
}
/**
 * Intersection is modeled as a color and a point. Color is the color of the object
 * which point is on.
 */
struct intersection_t {
  color_t color;
  position_t point;
  direction_t normal_vector;
};

direction_t sphere_normal_vector(sphere_t sphere, position_t pos) {
  return pos_to_dir(pos - sphere.center);
}

enum quadratic_result {
  NO_ROOT,
  ONE_ROOT,
  TWO_ROOTS
};

bool double_are_same(double v1, double v2) {
  return fabs(v1 - v2) < EPSILON;
}
