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
    if(DEBUG) cout << "No intersection" << endl;
    return NO_ROOT;
  } else if (discr == 0) {
    *result1 = -B / (2 * A);
    if(DEBUG) cout << "One intersection: " << *result1 << endl;
    return ONE_ROOT;
  } else {
    *result1 = (-B - sqrt(discr)) / (2 * A);
    *result2 = (-B + sqrt(discr)) / (2 * A);
    if(DEBUG) cout << "Two intersections: " << *result1 << " and " << *result2 << endl;
    return TWO_ROOTS;
  }
}

/**
 * This is the heart of the program. This function takes a ray vector and a list of
 * spheres, and returns a SORTED list of intersections, which if popped from back, returns
 * intersections that are closest first.
 */
vector<sphere_intersection_t> *ray_sphere_intersection(vector_t ray_vec, vector<sphere_t> spheres, plane_t ground_plane) {
  vector<sphere_intersection_t> *intersections = new vector<sphere_intersection_t>();
  for(vector<sphere_t>::iterator it = spheres.begin(); it != spheres.end(); it++) {
    if(DEBUG) cout << "-- Ray-Sphere Intersection --" << endl;
    sphere_t sphere = *it;
    double A = ray_vec.direction.dot(ray_vec.direction);
    double B = 2 * (ray_vec.direction.dot(ray_vec.origin - sphere.center));
    double C = pow((ray_vec.origin - sphere.center).length(), 2) - pow(sphere.radius, 2);
    double t1, t2;
    quadratic_result result = quadratic(A, B, C, &t1, &t2);
    if(result == NO_ROOT) {
      // No intersection
      if(DEBUG) cout << "No intersection" << endl;
    } else if (result == ONE_ROOT) {
      position_t point = ray_vec.origin + (ray_vec.direction * t1).approximate();
      intersections->push_back(sphere_intersection_t { sphere, sphere.color, point });
    } else if (result == TWO_ROOTS) {
      position_t point1 = ray_vec.origin + (ray_vec.direction * t1).approximate();
      position_t point2 = ray_vec.origin + (ray_vec.direction * t2).approximate();
      if(DEBUG) {
        cout << "Point 1:";
        point1.print();
        cout << "Point 2:";
        point2.print();
      }
      if(t1 >= 0) intersections->push_back(sphere_intersection_t { sphere, sphere.color, point1 });
      if(t2 >= 0) intersections->push_back(sphere_intersection_t { sphere, sphere.color, point2 });
    }
    if(DEBUG) cout << "-----------------------------------------------" << endl;
  }
  sort(intersections->begin(), intersections->end(), [&ray_vec](sphere_intersection_t l, sphere_intersection_t r) {
    return (l.point - ray_vec.origin).length() > (r.point - ray_vec.origin).length(); 
  });
  return intersections;
}

/**
 * Given an intersection and a list of spheres, shadows that point if necessary.
 */
void illuminate_point(sphere_intersection_t* focus_intersection, vector<sphere_t> spheres, plane_t ground_plane, position_t light_pos) {
  if(DEBUG) {
    cout << "-- Shadowing --" << endl;
    cout << "Focus Point: ";
    focus_intersection->point.print();
    cout << "Vector: ";
    (light_pos - focus_intersection->point).print();
  }
  vector_t shadow_vec = vector_t { focus_intersection->point, pos_to_dir(light_pos - focus_intersection->point) };
  vector<sphere_intersection_t> *intersections = ray_sphere_intersection(shadow_vec, spheres, ground_plane);
  if(!intersections->empty()) {
    while(intersections->back().point.too_close(focus_intersection->point) && !intersections->empty()) {
      intersections->pop_back();
    }
  }
  if(intersections->empty()) {
    double illumination = focus_intersection->normal_vector().angle_cos_with(pos_to_dir(light_pos - focus_intersection->point));
    focus_intersection->color.illuminate(illumination);
  }
  if(DEBUG) cout << "-----------------------------------------------" << endl;
}

/**
 * Shoots the given ray vector considering the spheres list.
 */
color_t shoot_ray(vector_t ray_vec, vector<sphere_t> spheres, vector<position_t> light_positions, plane_t ground_plane) {
  vector<sphere_intersection_t> *intersections = ray_sphere_intersection(ray_vec, spheres, ground_plane);
  if(intersections->empty()) {
    return white_color;
  } else {
    sphere_intersection_t elem = intersections->back();

    while(elem.point == origin && !intersections->empty()) {
      intersections->pop_back();
      sphere_intersection_t elem = intersections->back();
    }
    if(intersections->empty()) return white_color;
    sphere_intersection_t closest_intersection = intersections->back();
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

/**
 * Reads all the information necessary for representing spheres
 */
void read_spheres(vector<sphere_t> *spheres) {
  string object = "Sphere";
  cout << "Number of spheres:";
  int N;
  cin >> N;
  for(int i = 1; i <= N; i++) {
    int R, G, B;
    color_annot("R", i, object);
    cin >> R;
    color_annot("G", i, object);
    cin >> G;
    color_annot("B", i, object);
    cin >> B;

    double x, y, z;
    pos_annot("x", i, object);
    cin >> x;
    pos_annot("y", i, object);
    cin >> y;
    pos_annot("z", i, object);
    cin >> z;

    int radius;
    annot("Radius", i, object);
    cin >> radius;
    spheres->push_back(sphere_t { color_t { R, G, B, AMBIENT_LIGHT }, position_t { x, y, z }, radius });
  };
}

void read_light_positions(vector<position_t> *light_positions) {
  string object = "Light Source";
  cout << "Number of light sources:";
  int N;
  cin >> N;
  for(int i = 1; i <= N; i++) {
    double x, y, z;
    pos_annot("x", i, object);
    cin >> x;
    pos_annot("y", i, object);
    cin >> y;
    pos_annot("z", i, object);
    cin >> z;
    light_positions->push_back(position_t { x, y, z });
  }
}

input_data_t read_input_data() {
  vector<sphere_t>* spheres = new vector<sphere_t>();
  vector<position_t>* light_positions = new vector<position_t>();

  bool use_test_data;
  cout << "Use the test data only? (1 or 0 for yes or no)" << endl;
  cin >> use_test_data;

  if(use_test_data) {
    spheres->push_back(sphere_t { color_t { 255, 0, 0, AMBIENT_LIGHT }, position_t { 50.0, 50.0, 300.0 }, 20 });
    spheres->push_back(sphere_t { color_t { 0, 255, 0, AMBIENT_LIGHT }, position_t { 100.0, 100.0, 600.0 }, 60 });
    light_positions->push_back(position_t { 500, 500, 500 });
    light_positions->push_back(position_t { -500, -500, 500 });
  } else {
    read_spheres(spheres);
    read_light_positions(light_positions);
  }
  plane_t ground_plane = plane_t { position_t { 0, 0, 800 }, direction_t { 0, 0, -800 } };
  return input_data_t { *spheres, *light_positions, ground_plane };
}

int main() 
{
  input_data_t input_data = read_input_data();

  /* Preparing the plane */
  color_t **plane = init_plane();

  forall_plane(plane, [input_data](double x, double y){
    return shoot_ray(vector_t { origin, direction_t { x, y, PLANE_Z } }, input_data.spheres, input_data.light_positions, input_data.ground_plane);
  });

  /* Drawing the image */
  write_image(plane, "screen.bmp");
  return 0;
}
