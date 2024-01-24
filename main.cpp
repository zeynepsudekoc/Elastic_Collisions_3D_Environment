#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <vector>


// K�re vertex pozisyonlar�n� hesaplayan fonksiyon
std::vector<glm::vec3> calculateSphereVertices(float radius, int segments) {
    std::vector<glm::vec3> vertices;

    for (int i = 0; i <= segments; ++i) {
        float theta = static_cast<float>(i) / static_cast<float>(segments) * glm::pi<float>();
        for (int j = 0; j <= segments; ++j) {
            float phi = static_cast<float>(j) / static_cast<float>(segments) * glm::two_pi<float>();

            float x = radius * sin(theta) * cos(phi);
            float y = radius * sin(theta) * sin(phi);
            float z = radius * cos(theta);

            vertices.push_back(glm::vec3(x, y, z));
        }
    }

    return vertices;
}



struct Sphere {
    glm::vec3 position;
    float radius;
    glm::vec3 color;
    glm::vec3 velocity; // H�z vekt�r� eklendi
};

// K�reler aras� �arp��may� kontrol et
void checkCollisions(std::vector<Sphere>& spheres) {
    for (size_t i = 0; i < spheres.size(); ++i) {
        for (size_t j = i + 1; j < spheres.size(); ++j) {
            glm::vec3 diff = spheres[i].position - spheres[j].position;
            float distance = glm::length(diff);
            if (distance < (spheres[i].radius + spheres[j].radius)) {
                // Basit �arp��ma tepkisi: h�z vekt�rlerini takas et
                std::swap(spheres[i].velocity, spheres[j].velocity);
            }
        }
    }
}

// K�relerin ve k�p s�n�rlar�n�n �arp��mas�n� kontrol et
void checkCubeCollisions(std::vector<Sphere>& spheres, float cubeSize) {
    float halfCubeSize = cubeSize / 2.0f;
    for (auto& sphere : spheres) {
        for (int i = 0; i < 3; ++i) {
            if (sphere.position[i] + sphere.radius > halfCubeSize || sphere.position[i] - sphere.radius < -halfCubeSize) {
                sphere.velocity[i] *= -1; // �arp��ma duvar� ile ters y�nde h�z
            }
        }
    }
}

// K�relerin pozisyonunu g�ncelle
void updateSpherePositions(std::vector<Sphere>& spheres, float deltaTime) {
    for (auto& sphere : spheres) {
        // K�renin pozisyonunu h�z�na g�re g�ncelle
        sphere.position += sphere.velocity * deltaTime;
    }
}



void updateSimulation(std::vector<Sphere>& spheres, float cubeSize, float deltaTime) {
    // Pozisyonlar� g�ncelle
    updateSpherePositions(spheres, deltaTime);

    // �arp��malar� kontrol et
    checkCollisions(spheres);

    // K�p s�n�rlar� ile �arp��may� kontrol et
    checkCubeCollisions(spheres, cubeSize);
}



// Her k�re i�in model matrisi olu�tur
std::vector<glm::mat4> createModelMatrices(const std::vector<Sphere>& spheres) {
    std::vector<glm::mat4> modelMatrices;
    for (const Sphere& sphere : spheres) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, sphere.position);
        model = glm::scale(model, glm::vec3(sphere.radius));
        modelMatrices.push_back(model);
    }
    return modelMatrices;
}

// K�re i�in VAO ve VBO olu�tur
void createSphereVAO(GLuint& sphereVAO, GLuint& sphereVBO, const std::vector<glm::vec3>& sphereVertices) {
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(glm::vec3), &sphereVertices[0], GL_STATIC_DRAW);

    // Vertex pozisyonlar�n� belirt
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}


// Shader kaynak kodlar�
const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 FragPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
in vec3 FragPos;
out vec4 FragColor;
void main() {
    vec3 color = vec3(1.0, 0.0, 0.0); // Bu sabit de�er t�m k�releri k�rm�z� yapar
    FragColor = vec4(color, 1.0);
}

)glsl";


void drawSphere(const Sphere& sphere, const glm::mat4& view, const glm::mat4& projection, GLuint shaderProgram, GLuint sphereVAO, const std::vector<glm::vec3>& sphereVertices, const glm::mat4& modelMatrix) {
    // Shader program�n� kullan
    glUseProgram(shaderProgram);

    // Model, g�r�� ve projeksiyon matrislerini shader'a g�nder
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // K�renin rengini shader'a g�nder
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "sphereColor");
    glUniform3fv(colorLoc, 1, &sphere.color[0]);

    // K�re �izimi
    glBindVertexArray(sphereVAO);
    glDrawArrays(GL_TRIANGLES, 0, sphereVertices.size());
    glBindVertexArray(0);
}


// Kamera parametreleri
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);


// Global De�i�kenler
glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f); // Ba�lang�� pozisyonu
glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f); // Ba�lang�� h�z�
float gravity = -9.81f; // Yer�ekimi ivmesi


// Fizik G�ncellemesi Fonksiyonu
void updatePhysics(float deltaTime) {
    velocity.y += gravity * deltaTime; // Dikey h�z� g�ncelle
    position.y += velocity.y * deltaTime; // Dikey pozisyonu g�ncelle

    // �arp��ma Alg�lama ve Tepkisi
    float groundLevel = -5.0f; // Zemin seviyesi
    if (position.y < groundLevel) {
        velocity.y = -velocity.y; // Yans�ma
        position.y = groundLevel; // Zemin seviyesine ayarla
    }
}

void processInput(GLFWwindow* window) {
    const float cameraSpeed = 0.001f; // H�z sabiti, daha yava� bir h�z i�in 0.05'ten 0.02'ye d���r�ld�
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

void drawCube(GLuint shaderProgram, GLuint cubeVAO) {
    // Shader program�n� kullan
    glUseProgram(shaderProgram);

    // �effafl�k i�in blending ayarlar�
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // �effaf k�p �izimi
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Blending'i devre d��� b�rak
    glDisable(GL_BLEND);
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

int main() {
    // GLFW ba�latma ve pencere olu�turma i�lemleri
    if (!glfwInit()) {
        std::cerr << "GLFW ba�lat�lamad�!" << std::endl;
        return -1;
    }

    // GLFW i�in pencere ayarlar�n� yap
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // OpenGL s�r�m� 3.x
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // OpenGL s�r�m� x.3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Core Profile kullan
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // MacOS uyumlulu�u i�in

    // Pencereyi olu�tur
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Penceresi", nullptr, nullptr);
    if (!window) {
        std::cerr << "GLFW penceresi olu�turulamad�!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Olu�turulan pencereye OpenGL context'ini ata
    glfwMakeContextCurrent(window);

    // GLEW'i ba�lat
    glewExperimental = GL_TRUE; // GLEW'in modern OpenGL i�levlerini kullanmas�na izin ver
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW ba�lat�lamad�!" << std::endl;
        return -1;
    }

    const int sphereCount = 15;

    const float cubeSize = 2.0f; // K�p�n kenar uzunlu�u
    const float maxSphereRadius = 0.2f; // K�reler i�in maksimum yar��ap
// K�relerin konumland�r�laca�� maksimum uzakl�k, k�p�n kenar�n�n yar�s� k�re yar��ap� ��kar�lm�� olmal�
    const float maxPositionOffset = (cubeSize / 2.0f) - maxSphereRadius;

    // K�relerin olu�turuldu�u d�ng� i�inde
    std::vector<Sphere> spheres;
    for (int i = 0; i < sphereCount; ++i) {
        // K�p�n i�inde yer alacak �ekilde k�reler i�in rastgele pozisyonlar
        float xPos = ((float)rand() / RAND_MAX) * (maxPositionOffset * 2) - maxPositionOffset;
        float yPos = ((float)rand() / RAND_MAX) * (maxPositionOffset * 2) - maxPositionOffset;
        float zPos = ((float)rand() / RAND_MAX) * (maxPositionOffset * 2) - maxPositionOffset;

        // Rastgele renkler i�in
        glm::vec3 color = glm::vec3(static_cast<float>(rand()) / static_cast<float>(RAND_MAX),
            static_cast<float>(rand()) / static_cast<float>(RAND_MAX),
            static_cast<float>(rand()) / static_cast<float>(RAND_MAX));

        // K�releri olu�tur ve h�z vekt�rleri ile birlikte kaydet
        float radius = maxSphereRadius; // Bu �rnekte t�m k�reler ayn� yar��apa sahip
        float randVelocityX = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; // -1.0 ile 1.0 aras�nda
        float randVelocityY = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float randVelocityZ = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

        Sphere sphere = { glm::vec3(xPos, yPos, zPos), radius, color, glm::vec3(randVelocityX, randVelocityY, randVelocityZ) };
        spheres.push_back(sphere);
    }





    glfwSetKeyCallback(window, key_callback); // Klavye callback fonksiyonunu kaydet

    // Shader program� olu�turma ve yap�land�rma i�lemleri
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success;
    GLchar infoLog[512];

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // K�re i�in vertex pozisyonlar� hesapla
    float sphereRadius = 1.0f;
    int sphereSegments = 30;
    std::vector<glm::vec3> sphereVertices = calculateSphereVertices(sphereRadius, sphereSegments);

    // K�re i�in VAO ve VBO olu�turma
    GLuint sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(glm::vec3), &sphereVertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);

    // Vertex pozisyonlar�n� belirt
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Model, G�r�� ve Projeksiyon matrislerini shader'a g�nder
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");

    // Zaman �l��m� i�in de�i�kenler
    float lastFrameTime = glfwGetTime();
    float deltaTime;



    // K�p�n 8 k��e noktas�
    GLfloat cubeVertices[] = {
        -0.5f, -0.5f, -0.5f, // 0. k��e
         0.5f, -0.5f, -0.5f, // 1. k��e
         0.5f,  0.5f, -0.5f, // 2. k��e
        -0.5f,  0.5f, -0.5f, // 3. k��e
        -0.5f, -0.5f,  0.5f, // 4. k��e
         0.5f, -0.5f,  0.5f, // 5. k��e
         0.5f,  0.5f,  0.5f, // 6. k��e
        -0.5f,  0.5f,  0.5f  // 7. k��e
    };


    // K�p�n y�zeylerini olu�turan index verileri
    GLuint cubeIndices[] = {
        0, 1, 2, 2, 3, 0, // Arka y�z
        4, 5, 6, 6, 7, 4, // �n y�z
        4, 5, 1, 1, 0, 4, // Alt y�z
        7, 6, 2, 2, 3, 7, // �st y�z
        4, 0, 3, 3, 7, 4, // Sol y�z
        5, 1, 2, 2, 6, 5  // Sa� y�z
    };


    GLuint cubeVAO, cubeVBO, cubeEBO;

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);

    glBindVertexArray(cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    // Vertex pozisyonlar�n� belirt
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);





    // Render d�ng�s�
    while (!glfwWindowShouldClose(window)) {
        processInput(window);


        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Derinlik testini etkinle�tir
        glEnable(GL_DEPTH_TEST);

        float currentFrameTime = glfwGetTime();
        deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        updatePhysics(deltaTime);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));


        for (size_t i = 0; i < spheres.size(); ++i) {
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, spheres[i].position);

            // D�nme i�lemi eklenebilir
            float rotationSpeed = 1.0f; // D�nme h�z�
            modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime() * rotationSpeed, glm::vec3(0.5f, 1.0f, 0.0f));

            // K�renin boyutunu k���ltmek i�in radius de�erini d���r�n
            float scaleFactor = 0.25f; // K���ltme fakt�r�
            modelMatrix = glm::scale(modelMatrix, glm::vec3(spheres[i].radius * scaleFactor));

            drawSphere(spheres[i], view, projection, shaderProgram, sphereVAO, sphereVertices, modelMatrix);
        }

        // K�relerin ve �arp��malar�n sim�lasyonunu g�ncelle
        updateSimulation(spheres, cubeSize, deltaTime);


        glUseProgram(shaderProgram);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(sphereVAO);
        glDrawArrays(GL_TRIANGLES, 0, sphereVertices.size());
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}