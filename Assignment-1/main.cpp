#include <iostream>
#include <vector>
#include <math.h>
#include <queue>
#include "bitmap_image.hpp"

#define PLANE_START_X -50
#define PLANE_END_X 50
#define PLANE_START_Y -50
#define PLANE_END_Y 50
#define PLANE_WIDTH PLANE_END_X - PLANE_START_X
#define PLANE_HEIGHT PLANE_END_Y - PLANE_START_Y
#define PLANE_Z 100 // Assuming this is constant since it eases up things
#define DEBUG 1
#define LIGHT_VEC { 400, 0, 600 }

using namespace std;

struct color_t {
  double R;
  double G;
  double B;
  bool operator==(color_t other) {
    return this->R == other.R && this->G == other.G && this->B == other.B;
  }
  void desaturate() {
    this->R = this->R * 0.1;
    this->R = this->R * 0.1;
    this->R = this->R * 0.1;
  }
  void print() const {
    cout << "(" << this->R << ", " << this->G << ", " << this->B << ")" << endl;
  }
};

struct position_t {
  int x;
  int y;
  int z;
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
};

struct direction_t {
  double x;
  double y;
  double z;
  position_t approximate() {
    return position_t { (int) round(this->x), (int) round(this->y), (int) round(this->z) };
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

direction_t pos_to_dir(position_t pos) {
  return direction_t { (double) pos.x, (double) pos.y, (double) pos.z };
}

struct vector_t {
  position_t origin;
  direction_t direction;
};

/* struct threeDvec_t { */
/*   int x; */
/*   int y; */
/*   int z; */
/*   threeDvec_t operator+(threeDvec_t other) { */
/*     return threeDvec_t { this->x + other.x, this->y + other.y, this->z + other.z }; */
/*   } */
/*   threeDvec_t operator-(threeDvec_t other) { */
/*     return threeDvec_t { this->x - other.x, this->y - other.y, this->z - other.z }; */
/*   } */
/*   threeDvec_t operator*(double coeff) { */
/*     return threeDvec_t { (int) round(coeff) * this->x, (int) round(coeff) * this->y, (int) round(coeff) * this->z }; */
/*   } */
/*   int dot(threeDvec_t other) { */
/*     return this->x * other.x + this->y * other.y + this->z * other.z; */
/*   } */
/*   double length() { */
/*     return sqrt(this->x * this->x + this->y * this->y + this->z * this->z); */ 
/*   } */
/*   void print() const { */
/*     cout << "(" << this->x << ", " << this->y << ", " << this->z << ")" << endl; */
/*   } */
/* }; */

struct Sphere {
  color_t color;
  position_t center;
  int radius;
};

color_t** init_plane() {
  color_t** plane = new color_t*[PLANE_WIDTH];
  for(int i = 0; i < PLANE_WIDTH; ++i) {
    plane[i] = new color_t[PLANE_HEIGHT];
    for(int j = 0; j < PLANE_HEIGHT; ++j) {
      plane[i][j] = color_t { 0, 0, 0 };
    }
  }
  return plane;
}

/* struct ray_vec_t { */
/*   threeDvec_t origin; */
/*   threeDvec_t direction; */
/*   void print() { */
/*     cout << origin.x << "x + " << origin.y << "y + " << origin.z << " + t(" << direction.x << "x + " << direction.y << "y + " << direction.z << "z)" << endl; */
/*   } */
/* }; */

template <typename action>
void forall_plane(color_t** plane, action act) {
  for(int x = PLANE_START_X; x < PLANE_END_X; x++) {
    for(int y = PLANE_START_Y; y < PLANE_END_Y; y++) {
      plane[x-PLANE_START_X][y-PLANE_START_Y] = act(x, y); // Shifting indexes of plane matrix accordingly
    }
  }
}

void sphere_annot(string component, int sphere_number) {
  cout << component << " for Sphere No. " << sphere_number << ":";
}

void pos_annot(string axis, int sphere_number) {
  sphere_annot("Position (" + axis + ")", sphere_number);
}

void color_annot(string color_component, int sphere_number) {
  sphere_annot("Color (" + color_component + ")", sphere_number);
}

struct intersection_t {
  color_t color;
  position_t point;
};

auto create_comparator(position_t vec) {
  return [&vec](intersection_t l, intersection_t r) {
    return (l.point - vec).length() > (r.point - vec).length(); 
  };
}

vector<intersection_t> *ray_sphere_intersection(vector_t ray_vec, vector<Sphere> spheres) {
  vector<intersection_t> *intersections = new vector<intersection_t>();
  for(vector<Sphere>::iterator it = spheres.begin(); it != spheres.end(); it++) {
    /* cout << "Sphere " << endl << "Pos: "; */
    /* it->center.print(); */
    /* cout << "Radius: " << it->radius << endl; */
    /* cout << endl << endl; */
    Sphere sphere = *it;
    double A = ray_vec.direction.dot(ray_vec.direction);
    double B = 2 * (ray_vec.direction.dot(ray_vec.origin - sphere.center));
    double C = pow((ray_vec.origin - sphere.center).length(), 2) - pow(sphere.radius, 2);
    double discr = B * B - 4 * A * C;
    if(DEBUG) cout << "Delta: " << discr << endl;
    if(discr < 0) {
      // No intersection
      if(DEBUG) cout << "No intersections!" << endl;
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
      if(DEBUG) cout << "Point 1:";
      point1.print();
      if(DEBUG) cout << "Point 2:";
      point2.print();
      intersections->push_back(intersection_t { sphere.color, point1 });
      intersections->push_back(intersection_t { sphere.color, point2 });
    }
    if(DEBUG) cout << "-----------------------------------------------" << endl;
  }
  sort(intersections->begin(), intersections->end(), [&ray_vec](intersection_t l, intersection_t r) {
    return (l.point - ray_vec.origin).length() > (r.point - ray_vec.origin).length(); 
  });
  return intersections;
}

color_t white_color = color_t { 255, 255, 255 };
position_t light_pos = position_t LIGHT_VEC;

void shadow_point(intersection_t* focus_intersection, vector<Sphere> spheres) {
  cout << "-- Shadowing" << endl;
  cout << "Focus Point: ";
  focus_intersection->point.print();
  cout << "Vector: ";
  (light_pos - focus_intersection->point).print();
  cout << "-----------------------------------------------" << endl;
  vector_t shadow_vec = vector_t { focus_intersection->point, pos_to_dir(light_pos - focus_intersection->point) };
  vector<intersection_t> *intersections = ray_sphere_intersection(shadow_vec, spheres);
  if(intersections->empty()) {
    // This intersection point is taking perfect direct light.
    return;
  } else {
    cout << "wow";
    cout << intersections->size();
    intersection_t elem = intersections->back();
    while(elem.point == focus_intersection->point && !intersections->empty()) {
      intersections->pop_back();
      cout << "wow" << endl;
      cout << intersections->size() << endl;
      intersection_t elem = intersections->back();
    }
    if(!intersections->empty()) {
      focus_intersection->color.desaturate();
    }
    /*   cout << "----------" << endl; */
    /*   while(!intersections->empty()) { */
    /*     intersections->top().point.print(); */
    /*     intersections->pop(); */
    /*   } */
    /* } */
  }
  cout << endl;
}

color_t shoot_ray(vector_t ray_vec, vector<Sphere> spheres) {
  vector<intersection_t> *intersections = ray_sphere_intersection(ray_vec, spheres);
  if(intersections->empty()) {
    return white_color;
  } else {
    intersection_t elem = intersections->back();

    position_t origin = position_t { 0, 0, 0 }; // TODO: Fix, duplication
    while(elem.point == origin && !intersections->empty()) {
      cout << "Clearing one (0, 0, 0)" << endl;
      intersections->pop_back();
      intersection_t elem = intersections->back();
    }
    if(intersections->empty()) return white_color;
    intersection_t closest_intersection = intersections->back();
    cout << "All intersections:" << endl;
    while(!intersections->empty()) {
      intersections->back().point.print();
      intersections->back().color.print();
      intersections->pop_back();
    }
    /* shadow_point(&closest_intersection, spheres); */
    return closest_intersection.color;
  }
}

int main() 
{
    vector<Sphere>* spheres = new vector<Sphere>();
    /* cout << "Number of spheres:"; */
    /* int N; */
    /* cin >> N; */
    /* for(int i = 1; i <= N; i++) { */
    /*   int R, G, B; */
    /*   color_annot("R", i); */
    /*   cin >> R; */
    /*   color_annot("G", i); */
    /*   cin >> G; */
    /*   color_annot("B", i); */
    /*   cin >> B; */

    /*   int x, y, z; */
    /*   pos_annot("x", i); */
    /*   cin >> x; */
    /*   pos_annot("y", i); */
    /*   cin >> y; */
    /*   pos_annot("z", i); */
    /*   cin >> z; */

    /*   int radius; */
    /*   sphere_annot("Radius", i); */
    /*   cin >> radius; */
    /*   spheres->push_back(Sphere { color_t { R, G, B }, position_t { x, y, z }, radius }); */
    /* }; */
    spheres->push_back(Sphere { color_t { 255, 0, 0 }, position_t { 0, 0, 200 }, 40 });
    spheres->push_back(Sphere { color_t { 0, 0, 255 }, position_t { 50, 0, 600 }, 100 });
    color_t **empty_plane = init_plane();
    position_t origin = position_t { 0, 0, 0 };
    forall_plane(empty_plane, [origin, spheres](int x, int y){ return shoot_ray(vector_t { origin, pos_to_dir(position_t { x, y, PLANE_Z }) }, *spheres); });

    /* Drawing the image */
    bitmap_image image(100, 100);
    image.set_all_channels(0, 0, 0);
    forall_plane(empty_plane, [empty_plane, &image](int x, int y) {
      color_t color = empty_plane[x-PLANE_START_X][y-PLANE_START_Y];
      image.set_pixel(x-PLANE_START_X, y-PLANE_START_Y, color.R, color.G, color.B);
      return color;
    });
    image.save_image("wow.bmp");
    return 0;
}
