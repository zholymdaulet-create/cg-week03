// 3-апта: шейдерлер, GLSL, интерполяция
// Базалық: makeShader() + қате тексеру, позиция + түс атрибуты (6 float)
// Тапсырмаларды төмендегі ауыстырғыштармен қосып/өшіресің:

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

const int WINDOW_WIDTH  = 1280;
const int WINDOW_HEIGHT = 720;

// ===== Тапсырма ауыстырғыштары =====
const bool TASK1_INVERT_COLORS = true;   // 1: түстерді төңкеру (1.0 - vColor)
const bool TASK2_SAME_COLOR    = false;  // 2: үш вершинаға бірдей түс (градиент жоғалады)
const bool TASK3_FLIP_Y        = true;   // 3: gl_Position.y * -1 (үшбұрыш төңкеріледі)
const bool TASK4_BROKEN_SHADER = false;  // 4: әдейі синтаксис қатесі (тексеріп болған соң false қалдыр)

// ===== Шейдерлер =====
const char* VERTEX_SHADER_SRC = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
void main()
{
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}
)";

// 3-тапсырма: y компонентін -1-ге көбейту
const char* VERTEX_SHADER_FLIP_SRC = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
void main()
{
    gl_Position = vec4(aPos.x, aPos.y * -1.0, aPos.z, 1.0);
    vColor = aColor;
}
)";

const char* FRAGMENT_SHADER_SRC = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main()
{
    FragColor = vec4(vColor, 1.0);
}
)";

// 1-тапсырма: GLSL амалдары покомпонентті, цикл керек емес
const char* FRAGMENT_SHADER_INVERT_SRC = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main()
{
    FragColor = vec4(1.0 - vColor, 1.0);
}
)";

// 4-тапсырма: vec5 деген тип жоқ + ; жоқ
const char* FRAGMENT_SHADER_BROKEN_SRC = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main()
{
    vec5 c = vec5(vColor, 1.0, 1.0)
    FragColor = vec4(vColor, 1.0);
}
)";

void onResize(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Бір шейдерді компиляциялап, қатесін тексереді (GL_COMPILE_STATUS)
GLuint compileShader(GLenum type, const char* source)
{
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    int ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);   // компиляция үшін — glGetShaderiv
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        std::cerr << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
                  << " shader compile error:\n" << log << std::endl;
    }
    return id;
}

// Екі шейдерден бағдарлама жасайды, линковкасын тексереді (GL_LINK_STATUS)
GLuint makeShader(const char* vertexSrc, const char* fragmentSrc)
{
    GLuint vertexShader   = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vertexShader);
    glAttachShader(prog, fragmentShader);
    glLinkProgram(prog);

    int ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);   // линковка үшін — glGetProgramiv
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "Program link error:\n" << log << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return prog;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "cg", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n";
        return -1;
    }
    std::cout << "OpenGL:   " << glGetString(GL_VERSION)  << "\n";
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";

    // Ауыстырғыштарға қарай шейдер таңдау
    const char* vsSrc = TASK3_FLIP_Y ? VERTEX_SHADER_FLIP_SRC : VERTEX_SHADER_SRC;
    const char* fsSrc = TASK4_BROKEN_SHADER ? FRAGMENT_SHADER_BROKEN_SRC
                      : TASK1_INVERT_COLORS ? FRAGMENT_SHADER_INVERT_SRC
                      : FRAGMENT_SHADER_SRC;
    GLuint shaderProgram = makeShader(vsSrc, fsSrc);

    // Әр вершина: позиция (x, y, z) + түс (r, g, b) = 6 float
    float gradientVertices[] = {
        // позиция             // түс
        -0.5f, -0.5f, 0.0f,    1.0f, 0.0f, 0.0f,   // сол жақ төмен — қызыл
         0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,   // оң жақ төмен — жасыл
         0.0f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,   // жоғары — көк
    };
    // 2-тапсырма: үшеуіне бірдей түс (қызғылт сары)
    float sameColorVertices[] = {
        -0.5f, -0.5f, 0.0f,    1.0f, 0.5f, 0.2f,
         0.5f, -0.5f, 0.0f,    1.0f, 0.5f, 0.2f,
         0.0f,  0.5f, 0.0f,    1.0f, 0.5f, 0.2f,
    };
    float* vertices = TASK2_SAME_COLOR ? sameColorVertices : gradientVertices;

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gradientVertices), vertices, GL_STATIC_DRAW);

    const int STRIDE = 6 * sizeof(float);   // екі атрибутта да stride = 24 байт
    // 0-атрибут: позиция, offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    // 1-атрибут: түс, offset 3 float (байтпен, (void*)3 емес!)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
