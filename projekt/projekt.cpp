#include <array>
#include <fstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <vector>

using namespace std;

// Kör pontjai és színei
std::vector<glm::vec3> circlePoints;
std::vector<glm::vec3> circleColors;
// Vonal pontjai és színei
std::vector<glm::vec3> linePoints;
std::vector<glm::vec3> lineColors;

// Ablak mérete
const int window_width = 600;
const int window_height = 600;
const float circle_radius = 50.0f / 300.0f; // 50 pixel sugarú kör normált koordinátákban

GLuint VBO[4];  // Most 4 VBO-t használunk
GLuint VAO[2];  // Most 2 VAO-t használunk
GLuint shaderProgram;
float lineY = 0.0f;  // A vonal Y pozíciója fix

// Kör mozgásának változói
float circleX = 0.0f;
float circleY = 0.0f;
float direction = 1.0f;
float speed = 0.01f;
bool isMoving = false;

// Alapértelmezett vízszintes mozgás sebessége
float defaultSpeed = 0.005f;

// Képernyő széleire pattanás logika változatlan marad
const float angle = 25.0f;  // Az irányvektor szöge (25°)
const float directionLength = 0.02f;  // 10 pixel normált koordinátákban

// Irányvektor kiszámítása
float directionX = directionLength * cos(glm::radians(angle));  // X komponens
float directionY = directionLength * sin(glm::radians(angle));  // Y komponens

// Ellenőrzi, hogy a kör metszi-e a vonalszakaszt
bool checkIntersection(float cx, float cy, float radius, float lineY, float lineX1, float lineX2) {
    if (cy - radius > lineY || cy + radius < lineY) {
        return false;  // A kör túl messze van a vonaltól
    }

    if (cx + radius >= lineX1 && cx - radius <= lineX2) {
        return true; // A kör metszi a vonalat az X tartományban
    }

    return false; // Nincs metszés
}

// Shader fordításának ellenőrzése
void checkShaderCompilation(GLuint shader) {
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        cerr << "Hiba a shader fordításánál: " << infoLog << endl;
    }
}

// Shader program linkelésének ellenőrzése
void checkProgramLinking(GLuint program) {
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        cerr << "Hiba a shader program linkelésénél: " << infoLog << endl;
    }
}

// Shader fájl beolvasása
std::string readShaderFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Nem sikerült megnyitni a shader fájlt: " << filePath << std::endl;
        return "";
    }
    std::stringstream buffer{};
    buffer << file.rdbuf();
    return buffer.str();
}

// Shader program létrehozása
GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode = readShaderFile(vertexPath);
    std::string fragmentCode = readShaderFile(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        return 0;
    }

    const char* vShaderSource = vertexCode.c_str();
    const char* fShaderSource = fragmentCode.c_str();

    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vShaderSource, NULL);
    glCompileShader(vShader);
    checkShaderCompilation(vShader);

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fShaderSource, NULL);
    glCompileShader(fShader);
    checkShaderCompilation(fShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);
    checkProgramLinking(program);

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    return program;
}

void generateCirclePoints(glm::vec2 O, GLfloat r, GLint num_segment) {
    GLfloat x, y;
    GLfloat alpha = 0.0f;
    GLfloat full_circle = 2.0f * M_PI;

    circlePoints.push_back(glm::vec3(O.x, O.y, 0.0f));
    circleColors.push_back(glm::vec3(0.0f, 0.7f, 0.3f));

    for (int i = 0; i <= num_segment; i++) {
        x = O.x + r * cos(alpha);
        y = O.y + r * sin(alpha);

        circlePoints.push_back(glm::vec3(x, y, 0.0f));
        circleColors.push_back(glm::vec3(1.0f, 0.0f, 0.0f));

        alpha += full_circle / num_segment;
    }
}

void generateLinePoints() {
    float lineLength = 2.0f / 3.0f;  // Harmad oldalhosszúságú vonal
    float yPos = 0.0f;  // Középen elhelyezve

    // A vonal két végpontja
    linePoints.push_back(glm::vec3(-lineLength / 2.0f, yPos, 0.0f));  // Bal végpont
    linePoints.push_back(glm::vec3(lineLength / 2.0f, yPos, 0.0f));   // Jobb végpont

    // Vonal színének beállítása (kék)
    lineColors.push_back(glm::vec3(0.0f, 0.0f, 1.0f));  // Első pont színe
    lineColors.push_back(glm::vec3(0.0f, 0.0f, 1.0f));  // Második pont színe
}

// 'S' billentyű lenyomása
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_UP) {
            lineY += 0.05f;  // Felfelé mozgás
        }
        if (key == GLFW_KEY_DOWN) {
            lineY -= 0.05f;  // Lefelé mozgás
        }
        if (key == GLFW_KEY_S) {
            isMoving = true;  // Az 'S' lenyomásával indítjuk el az irányvektorral való mozgást
        }
    }
}

void init(GLFWwindow* window) {
    shaderProgram = createShaderProgram("vertexShader.glsl", "fragmentShader.glsl");

    generateCirclePoints(glm::vec2(0.0f, 0.0f), circle_radius, 64);
    generateLinePoints();

    glGenBuffers(4, VBO);
    glGenVertexArrays(2, VAO);

    // Kör inicializálása
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, circlePoints.size() * sizeof(glm::vec3), circlePoints.data(), GL_STATIC_DRAW);

    glBindVertexArray(VAO[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, circleColors.size() * sizeof(glm::vec3), circleColors.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    // Vízszintes vonal inicializálása
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, linePoints.size() * sizeof(glm::vec3), linePoints.data(), GL_STATIC_DRAW);

    glBindVertexArray(VAO[1]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
    glBufferData(GL_ARRAY_BUFFER, lineColors.size() * sizeof(glm::vec3), lineColors.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glClearColor(1.0f, 0.8f, 0.3f, 1.0f);  // Háttér szín beállítása
}

void display(GLFWwindow* window, double currentTime) {
    glClear(GL_COLOR_BUFFER_BIT);

    if (isMoving) {
        circleX += directionX;
        circleY += directionY;
        if (circleX + circle_radius >= 1.0f || circleX - circle_radius <= -1.0f) directionX *= -1.0f;
        if (circleY + circle_radius >= 1.0f || circleY - circle_radius <= -1.0f) directionY *= -1.0f;
    }
    else {
        circleX += defaultSpeed;
        if (circleX + circle_radius >= 1.0f || circleX - circle_radius <= -1.0f) defaultSpeed *= -1.0f;
    }

    glUseProgram(shaderProgram);
    GLuint objectTypeLoc = glGetUniformLocation(shaderProgram, "objectType");
    if (objectTypeLoc != -1) {
        glUniform1i(objectTypeLoc, 0);  // 0 = kör
    }

    GLuint circleCenterLoc = glGetUniformLocation(shaderProgram, "circleCenter");
    if (circleCenterLoc != -1) {
        glUniform2f(circleCenterLoc, circleX, circleY);  // Kör pozíciója frissítve
    }

    bool isIntersecting = checkIntersection(circleX, circleY, circle_radius, lineY, linePoints[0].x, linePoints[1].x);
    GLuint colorSwapLoc = glGetUniformLocation(shaderProgram, "colorSwap");
    if (colorSwapLoc != -1) {
        glUniform1i(colorSwapLoc, isIntersecting);  // Ha metszi a vonalat, színváltozás
    }

    glBindVertexArray(VAO[0]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, circlePoints.size());
    glBindVertexArray(0);

    // Vonal renderelése
    glUseProgram(shaderProgram);
    if (objectTypeLoc != -1) {
        glUniform1i(objectTypeLoc, 1);  // 1 = vonal
    }

    GLuint lineYLoc = glGetUniformLocation(shaderProgram, "lineY");
    if (lineYLoc != -1) {
        glUniform1f(lineYLoc, lineY);  // Vonal Y pozíciója fix
    }

    glLineWidth(3.0f);

    glBindVertexArray(VAO[1]);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

int main(void) {
    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Jakab Aron 1.beadando", NULL, NULL);
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) exit(EXIT_FAILURE);
    glfwSwapInterval(1);

    glfwSetKeyCallback(window, key_callback);

    init(window);

    while (!glfwWindowShouldClose(window)) {
        display(window, glfwGetTime());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(2, VAO);
    glDeleteBuffers(4, VBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
}
