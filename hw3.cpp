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
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <cmath>
#include <iostream>

#ifdef WIN32
#define strcasecmp _stricmp
#endif

#include <imageIO.h>

using namespace std;
#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char *filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH (640 / 4)
#define HEIGHT (480 / 4)

//the field of view of the camera
#define fov M_PI / 3

int TYPE_EMPTY = 0;
int TYPE_SPHERE = 1;
int TYPE_TRIANGLE = 2;

unsigned char buffer[HEIGHT][WIDTH][3];

// Data Structures

struct Point
{
    double x;
    double y;
    double z;
};

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
    int id;
};

struct Sphere
{
    double position[3];
    double color_diffuse[3];
    double color_specular[3];
    double shininess;
    double radius;
    int id;
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
    int intersectionType;
    int intersectionItemId;
};

// Point related functionPoint normalize(Point p)
Point normalize(Point p)
{
    Point output;
    float length = sqrt(pow(p.x, 2) + pow(p.y, 2) + pow(p.z, 2));
    output.x = p.x / length;
    output.y = p.y / length;
    output.z = p.z / length;
    return output;
}

double pointLength(Point p)
{
    return sqrt(pow(p.x, 2) + pow(p.y, 2) + pow(p.z, 2));
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

Point convertArrayToPoint(double loc[3])
{
    Point p;
    p.x = loc[0];
    p.y = loc[1];
    p.z = loc[2];
    return p;
}

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
vector<Ray> rays;
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b);

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b);

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);

bool debug = true;

Ray createRay(Point from, Point to)
{
    Ray r;
    r.origin = from;
    r.direction = normalize(deductTwoPoint(to, from));
    r.intersectionType = TYPE_EMPTY;
    r.color[0] = 0.99;
    r.color[1] = 0.99;
    r.color[2] = 0.99;
    r.distance = -1;
    return r;
}

vector<Ray> initRays()
{
    double a = (double)WIDTH / HEIGHT;
    double xLeft = -a * tan(fov / 2);
    double xRight = a * tan(fov / 2);
    double yBottom = -tan(fov / 2);
    double yTop = tan(fov / 2);

    vector<Ray> outputRays;
    for (int i = 0; i < WIDTH; i++)
    {
        for (int j = 0; j < HEIGHT; j++)
        {
            //            Starts at camera position
            Point from;
            from.x = 0;
            from.y = 0;
            from.z = 0;
            //            Ends at a point on image plane at -1
            Point to;
            to.x = xLeft + (i / (double)WIDTH) * 2 * xRight;
            to.y = yBottom + (j / (double)HEIGHT) * 2 * yTop;
            to.z = -1;
            outputRays.push_back(createRay(from, to));
        }
    }
    return outputRays;
}

void calculateSphereIllumination(Ray &r)
{
    for (int i = 0; i < num_lights; i++)
    {
        Point l = convertArrayToPoint(lights[i].position);
        Point intersectionPoint = addTwoPoint(r.origin, scalarMultiplication(r.direction, r.distance));
        for (int j = 0; j < num_spheres; j++)
        {
            if (spheres[j].id == r.intersectionItemId)
            {
                Point sc = convertArrayToPoint(spheres[j].position);
                Point n = normalize(deductTwoPoint(intersectionPoint, sc));
                double ln = dotProduct(l, n);
                ln = ln > 0 ? ln : 0;
                r.color[0] += lights[i].color[0] * spheres[j].color_diffuse[0] * ln;
                r.color[1] += lights[i].color[1] * spheres[j].color_diffuse[1] * ln;
                r.color[2] += lights[i].color[2] * spheres[j].color_diffuse[2] * ln;
            }
        }
    }
}
bool testShadowRaySphereIntersection(Ray r, Sphere s)
{
    Point o = r.origin;
    Point d = r.direction;
    //  center of sphere
    Point sc = convertArrayToPoint(s.position);
    double radius = s.radius;
    double b = 2 * (d.x * (o.x - sc.x) + d.y * (o.y - sc.y) + d.z * (o.z - sc.z));
    double c = pow((o.x - sc.x), 2) + pow((o.y - sc.y), 2) + pow((o.z - sc.z), 2) - pow(radius, 2);
    double mid = pow(b, 2) - 4 * c;
    if (mid < 0)
    {
        return false;
    }
    double t0 = (-b + sqrt(mid)) / 2;
    double t1 = (-b - sqrt(mid)) / 2;
    t0 = t0 < 0 ? 0 : t0;
    t1 = t1 < 0 ? 0 : t1;
    if (debug)
    {
        printf("For ray to point (%f, %f, %f ) \n", d.x, d.y, d.z);
        printf("t0 is %f, t1 is %f  \n", t0, t1);
    }
    double finalT = t0 < t1 ? t0 : t1;
    return finalT > 0;
}

bool testShadowRayTriangleIntersection(Ray r, Triangle t)
{
    Point direction = r.direction;
    Point origin = r.origin;
    Point a = convertArrayToPoint(t.v[0].position);
    Point b = convertArrayToPoint(t.v[1].position);
    Point c = convertArrayToPoint(t.v[2].position);
    Point edgeBA = deductTwoPoint(b, a);
    Point edgeCA = deductTwoPoint(c, a);
    Point normal = normalize(crossProduct(edgeBA, edgeCA));
    double d = dotProduct(normal, a);
    double tp = (d - dotProduct(normal, origin)) / dotProduct(direction, normal);
    if (tp <= 0)
    {
        return false;
    }
    // currently the point intersect with plane
    Point contactPoint = addTwoPoint(origin, scalarMultiplication(direction, tp));
    bool testBA = dotProduct(crossProduct(deductTwoPoint(b, a), deductTwoPoint(contactPoint, a)), normal) >= 0;
    bool testCB = dotProduct(crossProduct(deductTwoPoint(c, b), deductTwoPoint(contactPoint, b)), normal) >= 0;
    bool testAC = dotProduct(crossProduct(deductTwoPoint(a, c), deductTwoPoint(contactPoint, c)), normal) >= 0;

    if (testBA && testCB && testAC)
    {
        return true;
    }
    return false;
}

int testRaySphereIntersection(Ray &r, Sphere s)
{
    Point o = r.origin;
    Point d = r.direction;
    //  center of sphere
    Point sc = convertArrayToPoint(s.position);
    double radius = s.radius;
    double b = 2 * (d.x * (o.x - sc.x) + d.y * (o.y - sc.y) + d.z * (o.z - sc.z));
    double c = pow((o.x - sc.x), 2) + pow((o.y - sc.y), 2) + pow((o.z - sc.z), 2) - pow(radius, 2);
    double mid = pow(b, 2) - 4 * c;
    if (mid < 0)
    {
        return -1;
    }
    double t0 = (-b + sqrt(mid)) / 2;
    double t1 = (-b - sqrt(mid)) / 2;
    t0 = t0 < 0 ? 0 : t0;
    t1 = t1 < 0 ? 0 : t1;
    if (debug)
    {
        printf("For ray to point (%f, %f, %f ) \n", d.x, d.y, d.z);
        printf("t0 is %f, t1 is %f  \n", t0, t1);
    }
    double finalT = t0 < t1 ? t0 : t1;
    if (r.distance == -1 || r.distance > finalT)
    {
        r.color[0] = ambient_light[0];
        r.color[1] = ambient_light[1];
        r.color[2] = ambient_light[2];
        r.intersectionType = TYPE_SPHERE;
        r.intersectionItemId = s.id;
        r.distance = finalT;
        for (int i = 0; i < num_lights; i++)
        {
            Point intersectionPoint = addTwoPoint(r.origin, scalarMultiplication(r.direction, r.distance));
            Point l = normalize(deductTwoPoint(convertArrayToPoint(lights[i].position), intersectionPoint));
            Ray shadowRay = createRay(intersectionPoint, convertArrayToPoint(lights[i].position));
            bool isBlocked = false;
            for (int j = 0; j < num_spheres; j++)
            {
                if (spheres[j].id != s.id)
                {
                    isBlocked = testShadowRaySphereIntersection(shadowRay, spheres[j]);
                    if (isBlocked)
                    {
                        break;
                    }
                }
            }
            if (!isBlocked)
            {
                for (int j = 0; j < num_triangles; j++)
                {
                    if (triangles[j].id != s.id)
                    {
                        isBlocked = testShadowRayTriangleIntersection(shadowRay, triangles[j]);
                        if (isBlocked)
                        {
                            break;
                        }
                    }
                }
            }
            if (!isBlocked)
            {
                Point n = normalize(deductTwoPoint(intersectionPoint, sc));
                double ln = dotProduct(l, n);
                ln = ln > 0 ? ln : 0;
                r.color[0] += lights[i].color[0] * s.color_diffuse[0] * ln;
                r.color[1] += lights[i].color[1] * s.color_diffuse[1] * ln;
                r.color[2] += lights[i].color[2] * s.color_diffuse[2] * ln;
                Point v = normalize(deductTwoPoint(r.origin, intersectionPoint));
                Point reflection = deductTwoPoint(scalarMultiplication(n, 2 * dotProduct(l, n)), l);
                double rv = dotProduct(reflection, v);
                rv = rv > 0 ? rv : 0;
                r.color[0] += lights[i].color[0] * s.color_specular[0] * pow(rv, s.shininess);
                r.color[1] += lights[i].color[1] * s.color_specular[1] * pow(rv, s.shininess);
                r.color[2] += lights[i].color[2] * s.color_specular[2] * pow(rv, s.shininess);
            }
        }
        r.color[0] = r.color[0] > 1 ? 0.99 : r.color[0];
        r.color[1] = r.color[1] > 1 ? 0.99 : r.color[1];
        r.color[2] = r.color[2] > 1 ? 0.99 : r.color[2];
    }
    return -1;
}

int testRayTriangleIntersection(Ray &r, Triangle t)
{

    Point direction = r.direction;
    Point origin = r.origin;
    Point a = convertArrayToPoint(t.v[0].position);
    Point b = convertArrayToPoint(t.v[1].position);
    Point c = convertArrayToPoint(t.v[2].position);
    Point edgeBA = deductTwoPoint(b, a);
    Point edgeCA = deductTwoPoint(c, a);
    Point normal = normalize(crossProduct(edgeBA, edgeCA));
    double d = dotProduct(normal, a);
    double tp = (d - dotProduct(normal, origin)) / dotProduct(direction, normal);
    // currently the point intersect with plane
    Point contactPoint = addTwoPoint(origin, scalarMultiplication(direction, tp));
    bool testBA = dotProduct(crossProduct(deductTwoPoint(b, a), deductTwoPoint(contactPoint, a)), normal) >= 0;
    bool testCB = dotProduct(crossProduct(deductTwoPoint(c, b), deductTwoPoint(contactPoint, b)), normal) >= 0;
    bool testAC = dotProduct(crossProduct(deductTwoPoint(a, c), deductTwoPoint(contactPoint, c)), normal) >= 0;

    if (testBA && testCB && testAC)
    {
        if (r.distance == -1 || tp < r.distance)
        {
            r.distance = tp;
            r.color[0] = ambient_light[0];
            r.color[1] = ambient_light[1];
            r.color[2] = ambient_light[2];
            double areaABC = 0.5 * pointLength(crossProduct(edgeBA, edgeCA));
            double areaPBC = dotProduct(crossProduct(deductTwoPoint(c, b), deductTwoPoint(contactPoint, b)), normal);
            double areaAPC = dotProduct(crossProduct(deductTwoPoint(a, c), deductTwoPoint(contactPoint, c)), normal);
            double areaABP = dotProduct(crossProduct(deductTwoPoint(b, a), deductTwoPoint(contactPoint, a)), normal);
            double alpha = areaPBC / areaABC;
            double beta = areaAPC / areaABC;
            double y = 1 - alpha - beta;
            Point na = convertArrayToPoint(t.v[0].normal);
            Point nb = convertArrayToPoint(t.v[1].normal);
            Point nc = convertArrayToPoint(t.v[2].normal);
            Point n = normalize(addTwoPoint(addTwoPoint(scalarMultiplication(na, alpha), scalarMultiplication(nb, beta)), scalarMultiplication(nc, y)));
            double diffuseColor[3];
            double specularColor[3];
            diffuseColor[0] = alpha * t.v[0].color_diffuse[0] + beta * t.v[1].color_diffuse[0] + y * t.v[2].color_diffuse[0];
            diffuseColor[1] = alpha * t.v[0].color_diffuse[1] + beta * t.v[1].color_diffuse[1] + y * t.v[2].color_diffuse[1];
            diffuseColor[2] = alpha * t.v[0].color_diffuse[2] + beta * t.v[1].color_diffuse[2] + y * t.v[2].color_diffuse[2];

            specularColor[0] = alpha * t.v[0].color_specular[0] + beta * t.v[1].color_specular[0] + y * t.v[2].color_specular[0];
            specularColor[1] = alpha * t.v[0].color_specular[1] + beta * t.v[1].color_specular[1] + y * t.v[2].color_specular[1];
            specularColor[2] = alpha * t.v[0].color_specular[2] + beta * t.v[1].color_specular[2] + y * t.v[2].color_specular[2];

            for (int i = 0; i < num_lights; i++)
            {
                Point intersectionPoint = contactPoint;
                Point l = normalize(deductTwoPoint(convertArrayToPoint(lights[i].position), intersectionPoint));
                Ray shadowRay = createRay(intersectionPoint, convertArrayToPoint(lights[i].position));
                bool isBlocked = false;
                for (int j = 0; j < num_spheres; j++)
                {
                    if (spheres[j].id != t.id)
                    {
                        isBlocked = testShadowRaySphereIntersection(shadowRay, spheres[j]);
                        if (isBlocked)
                        {
                            break;
                        }
                    }
                }
                if (!isBlocked)
                {
                    for (int j = 0; j < num_triangles; j++)
                    {
                        if (triangles[j].id != t.id)
                        {
                            isBlocked = testShadowRayTriangleIntersection(shadowRay, triangles[j]);
                            if (isBlocked)
                            {
                                break;
                            }
                        }
                    }
                }
                if (!isBlocked)
                {
                    double ln = dotProduct(l, n);
                    ln = ln > 0 ? ln : 0;
                    r.color[0] += lights[i].color[0] * diffuseColor[0] * ln;
                    r.color[1] += lights[i].color[1] * diffuseColor[1] * ln;
                    r.color[2] += lights[i].color[2] * diffuseColor[2] * ln;
                    Point v = normalize(deductTwoPoint(r.origin, intersectionPoint));
                    Point reflection = deductTwoPoint(scalarMultiplication(n, 2 * dotProduct(l, n)), l);
                    double rv = dotProduct(reflection, v);
                    rv = rv > 0 ? rv : 0;
                    r.color[0] += lights[i].color[0] * specularColor[0] * pow(rv, t.v[0].shininess);
                    r.color[1] += lights[i].color[1] * specularColor[1] * pow(rv, t.v[0].shininess);
                    r.color[2] += lights[i].color[2] * specularColor[2] * pow(rv, t.v[0].shininess);
                }
            }
            r.color[0] = r.color[0] > 1 ? 0.99 : r.color[0];
            r.color[1] = r.color[1] > 1 ? 0.99 : r.color[1];
            r.color[2] = r.color[2] > 1 ? 0.99 : r.color[2];
        }
    }
    return -1;
}

void calculateIllumination(Ray &r)
{
    if (debug)
    {
        printf("Type of intersection for r is %d", r.intersectionType);
    }
    if (r.intersectionType == TYPE_SPHERE)
    {
    }
}

//MODIFY THIS FUNCTION
void draw_scene()
{
    rays = initRays();
    for (int i = 0; i < rays.size(); i++)
    {
        for (int j = 0; j < num_spheres; j++)
        {
            testRaySphereIntersection(rays[i], spheres[j]);
            //      calculateIllumination(rays[i]);
        }
        for (int j = 0; j < num_triangles; j++)
        {
            testRayTriangleIntersection(rays[i], triangles[j]);
            //      calculateIllumination(rays[i]);
        }
    }
    //a simple test output
    int count = 0;
    for (unsigned int x = 0; x < WIDTH; x++)
    {
        glPointSize(2.0);
        glBegin(GL_POINTS);
        for (unsigned int y = 0; y < HEIGHT; y++)
        {
            plot_pixel(x, y, rays[count].color[0] * 256, rays[count].color[1] * 256, rays[count].color[2] * 256);
            count++;
        }
        glEnd();
        glFlush();
    }
    printf("Done!\n");
    fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
    glVertex2i(x, y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    buffer[y][x][0] = r;
    buffer[y][x][1] = g;
    buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
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
            t.id = i;

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
            s.id = i;

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

void display()
{
}

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
    //hack to make it only draw once
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
