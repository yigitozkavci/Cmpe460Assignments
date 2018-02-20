#include <iostream>
#include <vector>

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

// (not-so-smart) constructors. Better than putting constructors inside structs imo.
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

enum ColorComponent { R, G, B };
enum PositionComponent { X, Y, Z };

void sphere_annot(string component, int sphere_number) {
  cout << component << " for Sphere No. " << sphere_number << ":";
}

void pos_annot(string axis, int sphere_number) {
  sphere_annot("Position (" + axis + ")", sphere_number);
}

void color_annot(string color_component, int sphere_number) {
  sphere_annot("Color (" + color_component + ")", sphere_number);
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
    return 0;
}
