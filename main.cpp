#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
 
#if __GNUG__
#   include <GLFW/glfw3.h>
#else
#   include <GL/glfw3.h>
#endif

#include "mat.h"
#include "shaders.h"

static double g_framesPerSec = 60.0f;
static double g_distancePerSec = 3.0f;
static double g_timeBetweenFrames = 1.0 / g_framesPerSec;
static double g_distancePerFrame = g_distancePerSec / g_framesPerSec;

GLFWwindow *window;

GLuint vao, vbo;
GLuint shaderProgram;

Vec4 sphere2Pos;

struct Material
{
    float ka;                // Ambient coefficient
    float kd;                // Diffuse coefficient
    float ks;                // Specular/Reflective coefficient
    float kt;                // Transmission coefficient
    float ior;               // Index of refraction
    Vec3 color;              
    int matType;             // Material type: 0 = Opaque non-reflective, 1 = reflective, 2 = transmissive
};

struct Sphere
{
    Vec3 center;             
    float radius;
    Material mat;
};

struct Plane
{
    Vec3 point;
    Vec3 normal;
    Material mat;
};

// Compile the shaders and link the program
void readAndCompileShaders(const char *vs, const char *fs, GLuint *shaderProgram)
{
    // Compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vs, NULL);
    glCompileShader(vertexShader);

    GLint status;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);

    if(status != GL_TRUE)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        fprintf(stderr, "Vertex shader compiled incorrectly.\n");
        fprintf(stderr, "%s\n", infoLog);
    }
        
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fs, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);

    if(status != GL_TRUE)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        fprintf(stderr, "Fragment shader compiled incorrectly.\n");
        fprintf(stderr, "%s\n", infoLog);
    }

    // Link the vertex and fragment shader into the shader program
    *shaderProgram = glCreateProgram();
    glAttachShader(*shaderProgram, vertexShader);
    glAttachShader(*shaderProgram, fragmentShader);
    glBindFragDataLocation(*shaderProgram, 0, "outColor");
    glLinkProgram(*shaderProgram);

    glGetProgramiv(*shaderProgram, GL_LINK_STATUS, &status);
    if(status != GL_TRUE)
    {
        glGetProgramInfoLog(fragmentShader, 512, NULL, infoLog);
        fprintf(stderr, "Shaders linked incorrectly.\n");
        fprintf(stderr, "%s\n", infoLog);
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void setLight(GLuint shaderProgram, const char *lightName, Vec3 position, Vec3 color, float intensity)
{
    char lightMember[30];
    strcpy(lightMember, lightName);
    strcat(lightMember, ".position");
    glUniform3f(glGetUniformLocation(shaderProgram, lightMember), position[0], position[1], position[2]);

    strcpy(lightMember, lightName);
    strcat(lightMember, ".color");
    glUniform3f(glGetUniformLocation(shaderProgram, lightMember), color[0], color[1], color[2]);

    strcpy(lightMember, lightName);
    strcat(lightMember, ".intensity");
    glUniform1f(glGetUniformLocation(shaderProgram, lightMember), intensity);
}

void setMaterial(GLuint shaderProgram, const char *objectName, Material mat)
{
    char objMatMember[30];
    strcpy(objMatMember, objectName);
    strcat(objMatMember, ".mat.ka");    
    glUniform1f(glGetUniformLocation(shaderProgram, objMatMember), mat.ka);

    strcpy(objMatMember, objectName);
    strcat(objMatMember, ".mat.kd");    
    glUniform1f(glGetUniformLocation(shaderProgram, objMatMember), mat.kd);
    
    strcpy(objMatMember, objectName);
    strcat(objMatMember, ".mat.ks");    
    glUniform1f(glGetUniformLocation(shaderProgram, objMatMember), mat.ks);

    strcpy(objMatMember, objectName);
    strcat(objMatMember, ".mat.color");    
    glUniform3f(glGetUniformLocation(shaderProgram, objMatMember), mat.color[0], mat.color[1], mat.color[2]);

    strcpy(objMatMember, objectName);    
    strcat(objMatMember, ".mat.matType");
    glUniform1i(glGetUniformLocation(shaderProgram, objMatMember), mat.matType);

    strcpy(objMatMember, objectName);
    strcat(objMatMember, ".mat.ior");
    glUniform1f(glGetUniformLocation(shaderProgram, objMatMember), mat.ior);

    strcpy(objMatMember, objectName);
    strcat(objMatMember, ".mat.kt");
    glUniform1f(glGetUniformLocation(shaderProgram, objMatMember), mat.kt);    
}

void setPlane(GLuint shaderProgram, const char *planeName, Vec3 point, Vec3 normal, Material mat, bool checkered)
{
    char planeMember[30];
    strcpy(planeMember, planeName);
    strcat(planeMember, ".point");
    glUniform3f(glGetUniformLocation(shaderProgram, planeMember), point[0], point[1], point[2]);
    
    strcpy(planeMember, planeName);
    strcat(planeMember, ".normal");
    glUniform3f(glGetUniformLocation(shaderProgram, planeMember), normal[0], normal[1], normal[2]);


    strcpy(planeMember, planeName);
    strcat(planeMember, ".checkered");
    glUniform1i(glGetUniformLocation(shaderProgram, planeMember), checkered);

    setMaterial(shaderProgram, planeName, mat);
}

void setSphere(GLuint shaderProgram, const char *sphereName, Vec3 point, float radius, Material mat)
{
    char sphereMember[30];
    strcpy(sphereMember, sphereName);
    strcat(sphereMember, ".center");    
    glUniform3f(glGetUniformLocation(shaderProgram, sphereMember), point[0], point[1], point[2]);
    
    strcpy(sphereMember, sphereName);
    strcat(sphereMember, ".radius");
    glUniform1f(glGetUniformLocation(shaderProgram, sphereMember), radius);

    setMaterial(shaderProgram, sphereName, mat);
}

void initScene()
{
    glUniform1i(glGetUniformLocation(shaderProgram, "MAX_BOUNCE"), 4);
    
    //--------------------- Lights
    // Light1
    Vec3 point(-1.0f, 1.0f, 1.0f);
    Vec3 color(1.0f, 1.0f, 1.0f);
    float intensity = 2.0f;
    setLight(shaderProgram, "uLight1", point, color, intensity);

    // Light2
    point = Vec3(1.0f, 2.0f, 0.0f);
    color = Vec3(1.0f, 1.0f, 1.0f);
    intensity = 0.5f;
    setLight(shaderProgram, "uLight2", point, color, intensity);
    
    //--------------------- Spheres
    // Sphere1
    point = Vec3(0.0f, 0.0f, 0.0f);
    //color = Vec3(0.5f, 0.0f, 0.0f);
    color = Vec3(0.0f, 0.0f, 0.0f);
    float radius = 0.3f;
    Material mat;
    mat.ka = 0.0f;
    mat.kd = 0.0f;
    //mat.ks = 1.0f;
    mat.kt = 0.9f;
    mat.color = color;
    mat.matType = 2;
    mat.ior = 1.5;
    setSphere(shaderProgram, "uSphere1", point, radius, mat);

    // Sphere2
    point = Vec3(0.6f, 0.0f, 0.0f);
    sphere2Pos = Vec4(point[0], point[1], point[2], 1.0f);
    color = Vec3(0.35f, 0.3f, 0.2f);
    radius = 0.2f;
    mat.ka = 0.1f;
    mat.kd = 0.8f;
    mat.ks = 0.2f;
    mat.color = color;
    mat.matType = 1;
    setSphere(shaderProgram, "uSphere2", point, radius, mat);
    
    // Sphere3
    point = Vec3(0.0f, 0.61f, 0.0f);
    //color = Vec3(0.5f, 0.0f, 0.0f);
    color = Vec3(0.0f, 0.0f, 0.0f);
    radius = 0.3f;
    mat.ka = 0.0f;
    mat.kd = 0.0f;
    mat.ks = 0.9f;
    mat.color = color;
    mat.matType = 1;
    setSphere(shaderProgram, "uSphere3", point, radius, mat);

    //--------------------- Planes
    // Plane1
    point = Vec3(0.0f, 0.0f, -10.0f);
    Vec3 normal(0.0f, 0.0f, 1.0f);
    color = Vec3(1.0f, 1.0f, 1.0f);
    mat.ka = 0.1f;
    mat.kd = 0.6f;
    mat.ks = 0.4f;
    mat.color = color;
    mat.matType = 0;
    bool checkered = false;
    setPlane(shaderProgram, "uPlane1", point, normal, mat, checkered);

    // Plane2 left
    point = Vec3(-4.0f, 0.0f, 0.0f);
    normal = Vec3(1.0f, 0.0f, 0.0f);
    color = Vec3(0.4f, 0.4f, 0.0f);
    mat.ka = 0.1f;
    mat.kd = 0.6f;
    mat.ks = 0.4f;
    mat.color = color;
    mat.matType = 0;
    checkered = false;            
    setPlane(shaderProgram, "uPlane2", point, normal, mat, checkered);

    // Plane3 right
    point = Vec3(4.0f, 0.0f, 0.0f);
    normal = Vec3(-1.0f, 0.0f, 0.0f);
    color = Vec3(0.0f, 0.7f, 0.0f);
    mat.ka = 0.1f;
    mat.kd = 0.6f;
    mat.ks = 0.4f;
    mat.color = color;
    mat.matType = 0;
    checkered = false;            
    setPlane(shaderProgram, "uPlane3", point, normal, mat, checkered);    
    

    // Plane4 back
    point = Vec3(0.0f, 0.0f, 10.0f);
    normal = Vec3(0.0f, 0.0f, -1.0f);
    color = Vec3(0.0f, 0.0f, 0.4f);
    mat.ka = 0.1f;
    mat.kd = 0.6f;
    mat.ks = 0.4f;
    mat.color = color;
    mat.matType = 0;
    checkered = false;            
    setPlane(shaderProgram, "uPlane4", point, normal, mat, checkered);    
    
    // Plane4 top
    point = Vec3(0.0f, 4.0f, 0.0f);
    normal = Vec3(0.0f, -1.0f, 0.0f);
    color = Vec3(0.4f, 0.0f, 0.0f);
    mat.ka = 0.1f;
    mat.kd = 0.6f;
    mat.ks = 0.4f;
    mat.color = color;
    mat.matType = 0;
    checkered = false;            
    setPlane(shaderProgram, "uPlane5", point, normal, mat, checkered);    
    
    // Plane4 bottom
    point = Vec3(0.0f, -3.0f, 0.0f);
    normal = Vec3(0.0f, 1.0f, 0.0f);
    color = Vec3(1.0f, 1.0f, 1.0f);
    mat.ka = 0.1f;
    mat.kd = 0.6f;
    mat.ks = 0.4f;
    mat.color = color;
    mat.matType = 1;
    checkered = true;            
    setPlane(shaderProgram, "uPlane6", point, normal, mat, checkered);    
}

void draw_scene()
{
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSwapBuffers(window);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        Vec3 point(-0.5f, 0.0f, -1.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "uSphere1.center"), point[0], point[1], point[2]);        
    }
}

int main()
{
    // -------------------------------- INIT ------------------------------- //

    // Init GLFW
    if (glfwInit() != GL_TRUE) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    
    // Create a rendering window with OpenGL 3.2 context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(1280, 960, "OpenGL", NULL, NULL);
    //window = glfwCreateWindow((int)g_windowWidth, (int)g_windowHeight, "OpenGL", NULL, NULL);
    glfwMakeContextCurrent(window);

    // Init GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    //glfwSetMouseButtonCallback(window, mouseButtonCallback);
    //glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);    

    // Setup shaders
    readAndCompileShaders(basicVertSrc, basicFragSrc, &shaderProgram);
    glUseProgram(shaderProgram);

    GLfloat vertices[] = {
        -1.0f,  -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0, -1.0,
        -1.0, 1.0,
        1.0, 1.0
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //sizeof works because vertices is an array, not a pointer.
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    // Setup vertex attributes
    GLint posAttrib = glGetAttribLocation(shaderProgram, "aPosition");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

    initScene();

    Mat4 rotation = Mat4::makeYRotation(3);

    GLuint uSphere2Pos = glGetUniformLocation(shaderProgram, "uSphere2.center");
    double currentTime, timeLastRender = 0;
    while(!glfwWindowShouldClose(window))
    {
        // Poll window events
        currentTime = glfwGetTime();
        if((currentTime - timeLastRender) >= g_timeBetweenFrames)
        {
            timeLastRender = currentTime;
            sphere2Pos = rotation * sphere2Pos;
            glUniform3f(uSphere2Pos, sphere2Pos[0], sphere2Pos[1], sphere2Pos[2]);
            draw_scene();
        }
        glfwPollEvents();

    }
}
