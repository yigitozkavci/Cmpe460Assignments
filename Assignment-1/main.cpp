#include <iostream>
#include <vector>

#define PLANE_WIDTH 100
#define PLANE_HEIGHT 100

using namespace std;

struct color_t {
  int R;
  int G;
  int B;
};

struct position_t {
  int x;
  int y;
  int z;
};

class Sphere {
  color_t color;
  position_t position;
  int radius;
public:
  Sphere(color_t, position_t, int);
};

Sphere::Sphere(color_t color, position_t position, int radius) {
  this->color = color;
  this->position = position;
  this->radius = radius;
}

// (not-so-smart) constructors. Better than putting constructors inside structs IMO.
color_t color(int R, int G, int B) {
  color_t color;
  color.R = R;
  color.G = G;
  color.B = B;
  return color;
}

position_t position(int x, int y, int z) {
  position_t position;
  position.x = x;
  position.y = y;
  position.z = z;
  return position;
}

color_t** init_plane() {
  color_t** plane = new color_t*[PLANE_WIDTH];
  for(int i = 0; i < PLANE_WIDTH; ++i)
    plane[i] = new color_t[PLANE_HEIGHT];
  return plane;
}

template <typename action>
void forall_plane(color_t** plane, action act) {
  for(int x = -50; x <= 50; x++) {
    for(int y = -50; y <= 50; y++) {
      plane[x][y] = act(x, y);
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

color_t red_color() {
  color_t color;
  color.R = 255;
  color.G = 0;
  color.B = 0;
  return color;
}

color_t shoot_ray(int x, int y, vector<Sphere*> spheres) {
  return red_color();
}

int main() 
{
    vector<Sphere*>* spheres = new vector<Sphere*>();
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

      int x, y, z;
      pos_annot("x", i);
      cin >> x;
      pos_annot("y", i);
      cin >> y;
      pos_annot("z", i);
      cin >> z;

      int radius;
      sphere_annot("Radius", i);
      cin >> radius;
      spheres->push_back(new Sphere(color(R, G, B), position(x, y, z), radius));
    };
    forall_plane(init_plane(), [spheres](int x, int y){ return shoot_ray(x, y, *spheres); });
    return 0;
}
