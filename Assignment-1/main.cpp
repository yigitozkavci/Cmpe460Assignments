#include <iostream>
#include <vector>
#include <math.h>
#include <queue>
#include "bitmap_image.hpp"

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

/**
 * When we round the quadratic equation results to integers, there occurs
 * resulting points which are "too" close, but not equal. These points are
 * meant to be equal so we tolerate length between them.
 *
 * In other words, if two points have distance of <= CLOSENESS_TOLERANCE, we
 * treat them as same points
 */
#define CLOSENESS_TOLERANCE 10

using namespace std;

/**
 * Representation of a RGB color.
 */
struct color_t {
  int R;
  int G;
  int B;
  bool operator==(color_t other) {
    return this->R == other.R && this->G == other.G && this->B == other.B;
  }
  void desaturate() {
    this->R = this->R * 0.1;
    this->G = this->G * 0.1;
    this->B = this->B * 0.1;
  }
  void print() const {
    cout << "(" << this->R << ", " << this->G << ", " << this->B << ")" << endl;
  }
};

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
 * Helper for receiving white color.
 */
color_t white_color = color_t { 255, 255, 255 };

/**
 * Helper for receiving light position.
 */
position_t light_pos = position_t LIGHT_POS;

/**
 * Origin point.
 */
position_t origin = position_t { 0.0, 0.0, 0.0 };

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
};

/**
 * Approximate a direction to be a position.
 */
direction_t pos_to_dir(position_t pos) {
  return direction_t { (double) pos.x, (double) pos.y, (double) pos.z };
}

/**
 * Vectors have origin and direction. This represents the formula (origin + t * direction).
 */
struct vector_t {
  position_t origin;
  direction_t direction;
};

/**
 * Representation of a sphere.
 */
struct Sphere {
  color_t color;
  position_t center;
  int radius;
};

/**
 * Allocate and prepare the plane as a whiteboard, hence the color_t { 0, 0, 0 }
 * everywhere.
 */
color_t** init_plane() {
  color_t** plane = new color_t*[PLANE_WIDTH * RESOLUTION_COEFF];
  for(int i = 0; i < PLANE_WIDTH * RESOLUTION_COEFF; ++i) {
    plane[i] = new color_t[PLANE_HEIGHT * RESOLUTION_COEFF];
    for(int j = 0; j < PLANE_HEIGHT * RESOLUTION_COEFF; ++j) {
      plane[i][j] = color_t { 0, 0, 0 };
    }
  }
  return plane;
}

/**
 * Invoke the action for all plane. Action is essentially a lambda
 * that will be run with the positions on the plane. It's practically for
 * shooting rays conventionally.
 */
template <typename action>
void forall_plane(color_t** plane, action act) {
  for(int x = 0; x < PLANE_WIDTH * RESOLUTION_COEFF; x++) {
    for(int y = 0; y < PLANE_HEIGHT * RESOLUTION_COEFF; y++) {
      plane[x][y] = act(
        ((double) x) / RESOLUTION_COEFF + PLANE_START_X,
        ((double) y) / RESOLUTION_COEFF + PLANE_START_Y
      ); // Shifting indexes of plane matrix accordingly
    }
  }
}

/**
 * Write an annotation for a component of a sphere.
 */
void sphere_annot(string component, int sphere_number) {
  cout << component << " for Sphere No. " << sphere_number << ":" << endl;
}

/**
 * Write an annotation for position of a sphere.
 */
void pos_annot(string axis, int sphere_number) {
  sphere_annot("Position (" + axis + ")", sphere_number);
}

/**
 * Write an annotation for color of a sphere.
 */
void color_annot(string color_component, int sphere_number) {
  sphere_annot("Color (" + color_component + ")", sphere_number);
}

/**
 * Intersection is modeled as a color and a point. Color is the color of the object
 * which point is on.
 */
struct intersection_t {
  color_t color;
  position_t point;
};

/**
 * This is the heart of the program. This function takes a ray vector and a list of
 * spheres, and returns a SORTED list of intersections, which if popped from back, returns
 * intersections that are closest first.
 */
vector<intersection_t> *ray_sphere_intersection(vector_t ray_vec, vector<Sphere> spheres) {
  vector<intersection_t> *intersections = new vector<intersection_t>();
  for(vector<Sphere>::iterator it = spheres.begin(); it != spheres.end(); it++) {
    if(DEBUG) cout << "-- Ray-Sphere Intersection --" << endl;
    Sphere sphere = *it;
    double A = ray_vec.direction.dot(ray_vec.direction);
    double B = 2 * (ray_vec.direction.dot(ray_vec.origin - sphere.center));
    double C = pow((ray_vec.origin - sphere.center).length(), 2) - pow(sphere.radius, 2);
    double discr = B * B - 4 * A * C;
    if(discr < 0) {
      // No intersection
      if(DEBUG) cout << "No intersection" << endl;
    } else if (discr == 0) {
      // One intersection
      double t = -B / (2 * A);
      if(DEBUG) cout << "One intersection: " << t << endl;
      position_t point = ray_vec.origin + (ray_vec.direction * t).approximate();
      intersections->push_back(intersection_t { sphere.color, point });
    } else {
      // Two intersections
      double t1 = (-B - sqrt(discr)) / (2 * A);
      double t2 = (-B + sqrt(discr)) / (2 * A);
      if(DEBUG) cout << "Two intersections: " << t1 << " and " << t2 << endl;
      position_t point1 = ray_vec.origin + (ray_vec.direction * t1).approximate();
      position_t point2 = ray_vec.origin + (ray_vec.direction * t2).approximate();
      if(DEBUG) {
        cout << "Point 1:";
        point1.print();
        cout << "Point 2:";
        point2.print();
      }
      if(t1 >= 0) intersections->push_back(intersection_t { sphere.color, point1 });
      if(t2 >= 0) intersections->push_back(intersection_t { sphere.color, point2 });
    }
    if(DEBUG) cout << "-----------------------------------------------" << endl;
  }
  sort(intersections->begin(), intersections->end(), [&ray_vec](intersection_t l, intersection_t r) {
    return (l.point - ray_vec.origin).length() > (r.point - ray_vec.origin).length(); 
  });
  return intersections;
}

/**
 * Given an intersection and a list of spheres, shadows that point if necessary.
 */
void shadow_point(intersection_t* focus_intersection, vector<Sphere> spheres) {
  if(DEBUG) {
    cout << "-- Shadowing --" << endl;
    cout << "Focus Point: ";
    focus_intersection->point.print();
    cout << "Vector: ";
    (light_pos - focus_intersection->point).print();
  }
  vector_t shadow_vec = vector_t { focus_intersection->point, pos_to_dir(light_pos - focus_intersection->point) };
  vector<intersection_t> *intersections = ray_sphere_intersection(shadow_vec, spheres);
  if(!intersections->empty()) {
    while(intersections->back().point.too_close(focus_intersection->point) && !intersections->empty()) {
      intersections->pop_back();
    }
    if(!intersections->empty()) {
      focus_intersection->color.desaturate();
    }
  }
  if(DEBUG) cout << "-----------------------------------------------" << endl;
}

/**
 * Shoots the given ray vector considering the spheres list.
 */
color_t shoot_ray(vector_t ray_vec, vector<Sphere> spheres) {
  vector<intersection_t> *intersections = ray_sphere_intersection(ray_vec, spheres);
  if(intersections->empty()) {
    return white_color;
  } else {
    intersection_t elem = intersections->back();

    while(elem.point == origin && !intersections->empty()) {
      intersections->pop_back();
      intersection_t elem = intersections->back();
    }
    if(intersections->empty()) return white_color;
    intersection_t closest_intersection = intersections->back();
    shadow_point(&closest_intersection, spheres);
    return closest_intersection.color;
  }
}

/**
 * Writes the given plane `plane` as a bmp image into a file named `filename`.
 */
void write_image(color_t **plane, string filename) {
  bitmap_image image(100 * RESOLUTION_COEFF, 100 * RESOLUTION_COEFF);
  forall_plane(plane, [plane, &image](double x, double y) {
    color_t color = plane[(int) ((x - PLANE_START_X) * RESOLUTION_COEFF)][(int) ((y - PLANE_START_Y) * RESOLUTION_COEFF) ];
    image.set_pixel((x-PLANE_START_X) * RESOLUTION_COEFF, (y-PLANE_START_Y) * RESOLUTION_COEFF, color.R, color.G, color.B);
    return color;
  });
  image.save_image(filename);
}

/**
 * Reads all the information necessary for representing spheres
 */
vector<Sphere>* read_spheres() {
  vector<Sphere>* spheres = new vector<Sphere>();
  bool use_test_data;
  cout << "Use the test data only? (1 or 0 for yes or no)" << endl;
  cin >> use_test_data;
  if(use_test_data) {
    spheres->push_back(Sphere { color_t { 255, 0, 0 }, position_t { 50.0, 50.0, 300.0 }, 20 });
    spheres->push_back(Sphere { color_t { 0, 255, 0 }, position_t { 100.0, 100.0, 600.0 }, 60 });
    return spheres;
  }
  cout << "Number of spheres:";
  int N;
  cin >> N;
  for(int i = 1; i <= N; i++) {
    int R, G, B;
    color_annot("R", i);
    cin >> R;
    color_annot("G", i);
    cin >> G;
    color_annot("B", i);
    cin >> B;

    double x, y, z;
    pos_annot("x", i);
    cin >> x;
    pos_annot("y", i);
    cin >> y;
    pos_annot("z", i);
    cin >> z;

    int radius;
    sphere_annot("Radius", i);
    cin >> radius;
    spheres->push_back(Sphere { color_t { R, G, B }, position_t { x, y, z }, radius });
  };
  return spheres;
}

int main() 
{
  vector<Sphere>* spheres = read_spheres();

  /* Preparing the plane */
  color_t **plane = init_plane();
  forall_plane(plane, [spheres](double x, double y){
    return shoot_ray(vector_t { origin, direction_t { x, y, PLANE_Z } }, *spheres);
  });

  /* Drawing the image */
  write_image(plane, "screen.bmp");
  return 0;
}
