#include <iostream>
#include <vector>
#include <math.h>
#include <queue>
#include <algorithm>
#include "bitmap_image.hpp"
#include "main.h"

using namespace std;

/**
 * Allocate and prepare the plane as a whiteboard, hence the color_t { 255, 255, 255, 1 }
 * everywhere.
 */
color_t** init_plane() {
  color_t** plane = new color_t*[PLANE_WIDTH * RESOLUTION_COEFF];
  for(int i = 0; i < PLANE_WIDTH * RESOLUTION_COEFF; ++i) {
    plane[i] = new color_t[PLANE_HEIGHT * RESOLUTION_COEFF];
    for(int j = 0; j < PLANE_HEIGHT * RESOLUTION_COEFF; ++j) {
      plane[i][j] = color_t { 255, 255, 255, 1 };
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
 * Write an annotation for a property of an object.
 */
void annot(string property, int no, string object) {
  cout << property << " for " << object << " No. " << no << ":" << endl;
}

/**
 * Write an annotation for position of an object.
 */
void pos_annot(string axis, int no, string object) {
  annot("Position (" + axis + ")", no, object);
}

/**
 * Write an annotation for color of an object.
 */
void color_annot(string color_component, int sphere_number, string object) {
  annot("Color (" + color_component + ")", sphere_number, object);
}

quadratic_result quadratic(double A, double B, double C, double *result1, double *result2) {
  double discr = B * B - 4 * A * C;
  if(discr < 0) {
    // No intersection
    return NO_ROOT;
  } else if (discr == 0) {
    *result1 = -B / (2 * A);
    return ONE_ROOT;
  } else {
    *result1 = (-B - sqrt(discr)) / (2 * A);
    *result2 = (-B + sqrt(discr)) / (2 * A);
    return TWO_ROOTS;
  }
}

vector<intersection_t> *ray_sphere_intersections(vector_t ray_vec, sphere_t sphere) {
  vector<intersection_t> *intersections = new vector<intersection_t>();
  double A = ray_vec.direction.dot(ray_vec.direction);
  double B = 2 * (ray_vec.direction.dot(ray_vec.origin - sphere.center));
  double C = pow((ray_vec.origin - sphere.center).length(), 2) - pow(sphere.radius, 2);
  double t1, t2;
  quadratic_result result = quadratic(A, B, C, &t1, &t2);

  if(result == NO_ROOT) {
    // No intersection
  } else if (result == ONE_ROOT) {
    // One intersection
    position_t point = ray_vec.origin + (ray_vec.direction * t1).approximate();
    intersections->push_back(intersection_t { sphere.color, point, sphere_normal_vector(sphere, point) });
  } else if (result == TWO_ROOTS) {
    // Two intersections
    position_t point1 = ray_vec.origin + (ray_vec.direction * t1).approximate();
    position_t point2 = ray_vec.origin + (ray_vec.direction * t2).approximate();

    if(t1 >= 0) intersections->push_back(intersection_t { sphere.color, point1, sphere_normal_vector(sphere, point1) });
    if(t2 >= 0) intersections->push_back(intersection_t { sphere.color, point2, sphere_normal_vector(sphere, point2) });
  }

  return intersections;
}

vector<intersection_t> *ray_plane_intersections(vector_t ray_vec, plane_t plane) {
  vector<intersection_t> *intersections = new vector<intersection_t>();
  position_t p0 = plane.point;
  position_t l0 = ray_vec.origin;
  direction_t n = plane.normal_vector;
  direction_t l = ray_vec.direction;
  if(n.dot(l) == 0) cout << "wow" << endl;
  double t = pos_to_dir(p0 - l0).dot(n) / l.dot(n);
  position_t intersection_point = ray_vec.origin + (ray_vec.direction * t).approximate();
  if(t >= 0)
    intersections->push_back(intersection_t { plane.color, intersection_point, plane.normal_vector });
  return intersections;
}

/**
 * This is the heart of the program. This function takes a ray vector and a list of
 * spheres, and returns a SORTED list of intersections, which if popped from back, returns
 * intersections that are closest first.
 */
vector<intersection_t> *ray_intersections(vector_t ray_vec, vector<sphere_t> spheres, plane_t ground_plane) {

  vector<intersection_t> *intersections = new vector<intersection_t>();

  /* Ray-Sphere Intersections */
  for(const sphere_t & sphere : spheres) {
    for(const intersection_t & intersection : *ray_sphere_intersections(ray_vec, sphere)) {
      intersections->push_back(intersection);
    }
  }
  for(const intersection_t & intersection : *ray_plane_intersections(ray_vec, ground_plane)) {
    intersections->push_back(intersection);
  }

  sort(intersections->begin(), intersections->end(), [&ray_vec](intersection_t l, intersection_t r) {
    return (l.point - ray_vec.origin).length() > (r.point - ray_vec.origin).length(); 
  });
  return intersections;
}

bool between(position_t point, position_t start, position_t end) {
  return (point - start).length() <= (end - start).length() &&
    (end - point).length() <= (end - start).length();
}
/**
 * Given an intersection and a list of spheres, shadows that point if necessary.
 */
void illuminate_point(intersection_t* focus_intersection, vector<sphere_t> spheres, plane_t ground_plane, position_t light_pos) {
  if(DEBUG) {
    cout << "-- Shadowing --" << endl;
    cout << "Focus Point: ";
    focus_intersection->point.print();
    cout << "Vector: ";
    (light_pos - focus_intersection->point).print();
  }
  vector_t shadow_vec = vector_t { focus_intersection->point, pos_to_dir(light_pos - focus_intersection->point) };
  vector<intersection_t> *intersections = ray_intersections(shadow_vec, spheres, ground_plane);
  if(!intersections->empty()) {
    while((between(light_pos, focus_intersection->point, intersections->back().point) || intersections->back().point.too_close(focus_intersection->point)) && !intersections->empty()) {
      intersections->pop_back();
    }
  }
  if(intersections->empty()) {
    double illumination = focus_intersection->normal_vector.angle_cos_with(pos_to_dir(light_pos - focus_intersection->point));
    focus_intersection->color.illuminate(illumination);
  }
  if(DEBUG) cout << "-----------------------------------------------" << endl;
}

/**
 * Shoots the given ray vector considering the spheres list.
 */
color_t shoot_ray(vector_t ray_vec, vector<sphere_t> spheres, vector<position_t> light_positions, plane_t ground_plane) {
  vector<intersection_t> *intersections = ray_intersections(ray_vec, spheres, ground_plane);
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
    for(const position_t& light_pos : light_positions) {
      illuminate_point(&closest_intersection, spheres, ground_plane, light_pos);
    }
    return closest_intersection.color;
  }
}

color_t apply_illumination(color_t color) {
  return color_t { (int) (color.R * color.lustre + 0.5)
                 , (int) (color.G * color.lustre + 0.5)
                 , (int) (color.B * color.lustre + 0.5)
                 , color.lustre
                 };
}

/**
 * Writes the given plane `plane` as a bmp image into a file named `filename`.
 */
void write_image(color_t **plane, string filename) {
  bitmap_image image(100 * RESOLUTION_COEFF, 100 * RESOLUTION_COEFF);
  forall_plane(plane, [plane, &image](double x, double y) {
    color_t color =
      apply_illumination(
        plane
          [(int) ((x - PLANE_START_X) * RESOLUTION_COEFF)]
          [(int) ((y - PLANE_START_Y) * RESOLUTION_COEFF)]
      );
    image.set_pixel((x-PLANE_START_X) * RESOLUTION_COEFF, (y-PLANE_START_Y) * RESOLUTION_COEFF, color.R, color.G, color.B);
    return color;
  });
  image.save_image(filename);
}

double read_double(string description) {
  cout << description << ": ";
  double value;
  cin >> value;
  return value;
}

int read_int(string description) {
  cout << description << ": ";
  int value;
  cin >> value;
  return value;
}

bool read_bool(string description) {
  cout << description << ": ";
  bool value;
  cin >> value;
  return value;
}

position_t read_position(string description) {
  cout << description << endl;
  double x = read_double("x");
  double y = read_double("y");
  double z = read_double("z");
  return position_t { x, y, z };
}

color_t read_color(string description) {
  cout << description << endl;
  int R, G, B;
  R = read_int("R");
  G = read_int("G");
  B = read_int("B");
  return color_t { R, G, B, AMBIENT_LIGHT };
}

/**
 * Reads all the information necessary for representing spheres
 */
void read_spheres(vector<sphere_t> *spheres) {
  string object = "Sphere";
  int N = read_int("Number of spheres");
  for(int i = 1; i <= N; i++) {
    color_t color = read_color("Sphere Color");
    position_t center = read_position("Sphere Center");
    int radius = read_int("Sphere Radius");
    spheres->push_back(sphere_t { color, center, radius });
  };
}

void read_light_positions(vector<position_t> *light_positions) {
  int N = read_int("Number of light sources");
  for(int i = 1; i <= N; i++) {
    light_positions->push_back(read_position("Light Source Position"));
  }
}

direction_t read_direction(string description) {
  return pos_to_dir(read_position(description));
}

void read_ground_plane(plane_t *ground_plane) {
  string object = "Ground Plane";
  *ground_plane = plane_t { read_position("Ground Plane Position: (0, 0, 700) for instance"), read_direction("Ground Plane Direction: (0, 0, -1) for instance"), PLANE_COLOR };
}

input_data_t read_input_data() {
  vector<sphere_t>* spheres = new vector<sphere_t>();
  vector<position_t>* light_positions = new vector<position_t>();
  plane_t ground_plane = plane_t { position_t { 0, 0, 700 }, direction_t { 0, 0, -1 }, PLANE_COLOR };

  bool use_test_data = read_bool("Use the test data only? (1 or 0 for yes or no)");

  if(use_test_data) {
    spheres->push_back(sphere_t { color_t { 255, 0, 0, AMBIENT_LIGHT }, position_t { 50.0, 50.0, 300.0 }, 20 });
    spheres->push_back(sphere_t { color_t { 0, 255, 0, AMBIENT_LIGHT }, position_t { 100.0, 100.0, 600.0 }, 60 });
    light_positions->push_back(position_t { 500, 500, 500 });
  } else {
    read_spheres(spheres);
    read_light_positions(light_positions);
    read_ground_plane(&ground_plane);
  }
  return input_data_t { *spheres, *light_positions, plane_t { position_t { 0, 0, 700 }, direction_t { 0, 0, -1 }, PLANE_COLOR } };
}

int main() 
{
  input_data_t input_data = read_input_data();

  cout << "Starting the rendering, this process can take a while..." << endl;
  /* Preparing the plane */
  color_t **plane = init_plane();

  forall_plane(plane, [input_data](double x, double y){
    return shoot_ray(vector_t { origin, direction_t { x, y, PLANE_Z } }, input_data.spheres, input_data.light_positions, input_data.ground_plane);
  });

  /* Drawing the image */
  write_image(plane, "screen.bmp");
  return 0;
}
