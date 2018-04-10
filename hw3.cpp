/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Tianhang Liu
 * *************************
 */

#ifdef WIN32
#include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
#include <GL/gl.h>
#include <GL/glut.h>
#elif defined(__APPLE__)
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#define strcasecmp _stricmp
#endif

#include <imageIO.h>
#include <vector>
#include <cmath>
#include <iostream>

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char *filename = NULL;

// different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

// you may want to make these smaller for debugging purposes
#define WIDTH (640 / 4)
#define HEIGHT (480 / 4)

// the field of view of the camera
#define fov 60.0
#define depth 1.0

using namespace std;

unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
};
struct Point
{
  double x;
  double y;
  double z;
};

struct Light
{
  double position[3];
  double color[3];
};

struct Ray
{
  Point direction;
  Point origin;
  double distance;
  double color[3];
};

Point normalize(Point p)
{
  Point output;
  float length = sqrt(pow(p.x, 2) + pow(p.y, 2) + pow(p.z, 2));
  output.x = p.x / length;
  output.y = p.y / length;
  output.z = p.z / length;
  return output;
}

Point normalizedCrossProduct(Point p1, Point p2)
{
  Point output;
  output.x = p1.y * p2.z - p1.z * p2.y;
  output.y = p1.z * p2.x - p1.x * p2.z;
  output.z = p1.x * p2.y - p1.y * p2.x;
  return normalize(output);
}

Point crossProduct(Point p1, Point p2)
{
  Point output;
  output.x = p1.y * p2.z - p1.z * p2.y;
  output.y = p1.z * p2.x - p1.x * p2.z;
  output.z = p1.x * p2.y - p1.y * p2.x;
  return output;
}

double dotProduct(Point p1, Point p2)
{
  return p1.x * p2.x + p1.y * p2.y + p1.z * p2.z;
}

Point scalarMultiplication(Point p, float num)
{
  Point output;
  output.x = p.x * num;
  output.y = p.y * num;
  output.z = p.z * num;
  return output;
}

Point addTwoPoint(Point pointA, Point pointB)
{
  Point output;
  output.x = pointA.x + pointB.x;
  output.y = pointA.y + pointB.y;
  output.z = pointA.z + pointB.z;
  return output;
}

Point deductTwoPoint(Point pointA, Point pointB)
{
  Point output;
  output.x = pointA.x - pointB.x;
  output.y = pointA.y - pointB.y;
  output.z = pointA.z - pointB.z;
  return output;
}

Point reverse(Point p)
{
  Point output;
  output.x = -p.x;
  output.y = -p.y;
  output.z = -p.z;
  return output;
}

Point convertArrayToPoint(double loc[3])
{
  Point p;
  p.x = loc[0];
  p.y = loc[1];
  p.z = loc[2];
  return p;
}

vector<Ray> rays;

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;
const double pi = std::acos(-1);

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g,
                        unsigned char b);
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g,
                     unsigned char b);
void plot_pixel(int x, int y, unsigned char r, unsigned char g,
                unsigned char b);

double calculateDiffuseLight(Point l, Point n, double diffuse)
{
  double dotLN = dotProduct(l, n);
  dotLN = dotLN < 0 ? 0 : dotLN;
  return dotLN * diffuse;
}

double calculateReflective(Point r, Point v, double speculative, double shi)
{
  double dotRV = dotProduct(r, v);
  dotRV = dotRV < 0 ? 0 : dotRV;
  return pow(dotRV, shi) * speculative;
}

double calculateLight(Point l, Point n, Point r, Point v, double diffuse, double speculative, double shi, double color)
{
  double output = color * (calculateDiffuseLight(l, n, diffuse) + calculateReflective(r, v, speculative, shi));
  return output > 1 ? 1 : output;
}

vector<Ray> createRays()
{
  vector<Ray> rays;
  double a = (double)WIDTH / (double)HEIGHT;
  double xMax = a * tan(pi / 6);
  double yMax = tan(pi / 6);
  double midWidth = (double)WIDTH / 2.0;
  double midHeight = (double)HEIGHT / 2.0;
  for (unsigned int x = 0; x < WIDTH; x++)
  {
    for (unsigned int y = 0; y < HEIGHT; y++)
    {
      Ray r;
      // initialize origin
      r.origin.x = 0;
      r.origin.y = 0;
      r.origin.z = 0;
      // initialize direction
      r.direction.x = xMax * (x - midWidth) / midWidth;
      r.direction.y = yMax * (y - midHeight) / midHeight;
      r.direction.z = -depth;
      // normalize direction
      r.direction = normalize(r.direction);
      // set default color
      r.color[0] = 1;
      r.color[1] = 1;
      r.color[2] = 1;
      r.distance = -1;
      // cout << "Ray for " << x << " " << y << " is " << r.direction[0] << " " << r.direction[1] << " " << r.direction[2] << endl;
      rays.push_back(r);
    }
  }
  return rays;
}

// Calculate b2 – 4c, abort if negative
// – Compute normal only for closest intersection
// – Other similar optimizations
// x = x0 + xdt, y=y0+ydt, z=z0+zdt

void testSphereIntersection(Ray &ray)
{
  Point direction = ray.direction;
  Point origin = ray.origin;
  for (int i = 0; i < num_spheres; i++)
  {
    double xc = spheres[i].position[0];
    double yc = spheres[i].position[1];
    double zc = spheres[i].position[2];
    double r = spheres[i].radius;
    double b = 2 * (direction.x * (origin.x - xc) + direction.y * (origin.y - yc) + direction.z * (origin.z - zc));
    double c = pow(origin.x - xc, 2) + pow(origin.y - yc, 2) + pow(origin.z - zc, 2) - pow(r, 2);
    if (pow(b, 2) - 4 * c < 0)
    {
      continue;
    }
    else
    {
      double t0 = (-b + sqrt(pow(b, 2) - 4 * c)) / 2;
      double t1 = (-b - sqrt(pow(b, 2) - 4 * c)) / 2;
      // test if any of them is larger than 0
      t0 = t0 > 0 ? t0 : 0;
      t1 = t1 > 0 ? t0 : 0;
      double tFinal = t0 < t1 ? t0 : t1;
      if (tFinal != 0)
      {
        if (ray.distance == -1)
        {
          ray.distance = tFinal;
          ray.color[0] = spheres[i].color_diffuse[0];
          ray.color[1] = spheres[i].color_diffuse[1];
          ray.color[2] = spheres[i].color_diffuse[2];
        }
        else if (tFinal < ray.distance)
        {
          ray.distance = tFinal;
          ray.color[0] = spheres[i].color_diffuse[0];
          ray.color[1] = spheres[i].color_diffuse[1];
          ray.color[2] = spheres[i].color_diffuse[2];
        }
      }
    }
  }
}

void testTriangleIntersection(Ray &ray)
{
  Point direction = ray.direction;
  Point origin = ray.origin;
  for (int i = 0; i < num_triangles; i++)
  {
    Point v1 = convertArrayToPoint(triangles[i].v[0].position);
    Point v2 = convertArrayToPoint(triangles[i].v[1].position);
    Point v3 = convertArrayToPoint(triangles[i].v[2].position);
    Point edgeBA = deductTwoPoint(v2, v1);
    Point edgeCA = deductTwoPoint(v3, v1);
    Point normal = crossProduct(edgeBA, edgeCA);
    double d = dotProduct(normal, v1);
    double tp = -(dotProduct(normal, origin) + d) / dotProduct(direction, normal);
    // currently the point intersect with plane
    Point contactPoint = addTwoPoint(origin, scalarMultiplication(direction, tp));
    if (dotProduct(crossProduct(deductTwoPoint(v2, v1), deductTwoPoint(contactPoint, v1)), normal) >= 0 && dotProduct(crossProduct(deductTwoPoint(v3, v2), deductTwoPoint(contactPoint, v2)), normal) >= 0 && dotProduct(crossProduct(deductTwoPoint(v1, v3), deductTwoPoint(contactPoint, v3)), normal) >= 0)
    {
      if (ray.distance == -1)
      {
        ray.distance = tp;
        ray.color[0] = triangles[i].v[0].color_diffuse[0];
        ray.color[1] = triangles[i].v[0].color_diffuse[1];
        ray.color[2] = triangles[i].v[0].color_diffuse[2];
      }
      else if (tp < ray.distance)
      {
        ray.distance = tp;
        ray.color[0] = triangles[i].v[0].color_diffuse[0];
        ray.color[1] = triangles[i].v[0].color_diffuse[1];
        ray.color[2] = triangles[i].v[0].color_diffuse[2];
      }
    }
  }
}

// MODIFY THIS FUNCTION

void draw_scene()
{
  rays = createRays();
  // a simple test output
  for (int i = 0; i < rays.size(); i++)
  {
    testSphereIntersection(rays[i]);
    testTriangleIntersection(rays[i]);
  }
  int count = 0;
  for (unsigned int x = 0; x < WIDTH; x++)
  {
    glPointSize(2.0);
    glBegin(GL_POINTS);
    for (unsigned int y = 0; y < HEIGHT; y++)
    {
      plot_pixel(x, y, rays[count].color[0] * 256.0, rays[count].color[1] * 256.0, rays[count].color[2] * 256.0);
      count++;
    }
    glEnd();
    glFlush();
  }
  printf("Done!\n");
  fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g,
                        unsigned char b)
{
  glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  glVertex2i(x, y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g,
                     unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g,
                unsigned char b)
{
  plot_pixel_display(x, y, r, g, b);
  if (mode == MODE_JPEG)
    plot_pixel_jpeg(x, y, r, g, b);
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);

  ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in Saving\n");
  else
    printf("File saved Successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if (strcasecmp(expected, found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parse error, abnormal abortion\n");
    exit(0);
  }
}

void parse_doubles(FILE *file, const char *check, double p[3])
{
  char str[100];
  fscanf(file, "%s", str);
  parse_check(check, str);
  fscanf(file, "%lf %lf %lf", &p[0], &p[1], &p[2]);
  printf("%s %lf %lf %lf\n", check, p[0], p[1], p[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file, "%s", str);
  parse_check("rad:", str);
  fscanf(file, "%lf", r);
  printf("rad: %f\n", *r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file, "%s", s);
  parse_check("shi:", s);
  fscanf(file, "%lf", shi);
  printf("shi: %f\n", *shi);
}

int loadScene(char *argv)
{
  FILE *file = fopen(argv, "r");
  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file, "%i", &number_of_objects);

  printf("number of objects: %i\n", number_of_objects);

  parse_doubles(file, "amb:", ambient_light);

  for (int i = 0; i < number_of_objects; i++)
  {
    fscanf(file, "%s\n", type);
    printf("%s\n", type);
    if (strcasecmp(type, "triangle") == 0)
    {
      printf("found triangle\n");
      for (int j = 0; j < 3; j++)
      {
        parse_doubles(file, "pos:", t.v[j].position);
        parse_doubles(file, "nor:", t.v[j].normal);
        parse_doubles(file, "dif:", t.v[j].color_diffuse);
        parse_doubles(file, "spe:", t.v[j].color_specular);
        parse_shi(file, &t.v[j].shininess);
      }

      if (num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if (strcasecmp(type, "sphere") == 0)
    {
      printf("found sphere\n");

      parse_doubles(file, "pos:", s.position);
      parse_rad(file, &s.radius);
      parse_doubles(file, "dif:", s.color_diffuse);
      parse_doubles(file, "spe:", s.color_specular);
      parse_shi(file, &s.shininess);

      if (num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if (strcasecmp(type, "light") == 0)
    {
      printf("found light\n");
      parse_doubles(file, "pos:", l.position);
      parse_doubles(file, "col:", l.color);

      if (num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n", type);
      exit(0);
    }
  }
  return 0;
}

void display() {}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0, WIDTH, 0, HEIGHT, 1, -1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  // hack to make it only draw once
  static int once = 0;
  if (!once)
  {
    draw_scene();
    if (mode == MODE_JPEG)
      save_jpg();
  }
  once = 1;
}

int main(int argc, char **argv)
{
  if ((argc < 2) || (argc > 3))
  {
    printf("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if (argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if (argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc, argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(WIDTH, HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}
