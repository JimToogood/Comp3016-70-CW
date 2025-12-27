#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/glm/ext/vector_float3.hpp"
#include <glm/glm/ext/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/noise.hpp>

#include "main.h"
#include "LoadShaders.h"

using namespace std;
using namespace glm;


class Camera {
public:
    Camera(int windowWidth, int windowHeight) :
        position(vec3((CHUNK_SIZE * TILE_SIZE)/2, 10.0f, (CHUNK_SIZE * TILE_SIZE)/2)),  // Spawn player at centre of starting chunk
        front(vec3(0.0f, 0.0f, -1.0f)),
        up(vec3(0.0f, 1.0f, 0.0f)),
        yaw(-90.0f),
        pitch(0.0f),
        lastXPos(windowWidth / 2.0f),
        lastYPos(windowHeight / 2.0f),
        mouseFirstEntry(true)
    {}

    void HandleKeyboard(GLFWwindow* window, float deltaTime) {
        const float speed = 25.0f * deltaTime;

        // Horizontal movement controls
        vec3 horizontalFront = normalize(vec3(front.x, 0.0f, front.z));
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            position += speed * horizontalFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            position -= speed * horizontalFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            position -= normalize(cross(horizontalFront, up)) * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            position += normalize(cross(horizontalFront, up)) * speed;

        // Vertical movement controls
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            position += speed * up;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            position -= speed * up;
    }

    void HandleMouse(double xpos, double ypos) {
        // Initialise last positions
        if (mouseFirstEntry) {
            lastXPos = (float)xpos;
            lastYPos = (float)ypos;
            mouseFirstEntry = false;
        }

        float xOffset = (float)xpos - lastXPos;
        float yOffset = lastYPos - (float)ypos;
        lastXPos = (float)xpos;
        lastYPos = (float)ypos;

        // Define mouse sensitivity
        const float sensitivity = 0.04f;
        xOffset *= sensitivity;
        yOffset *= sensitivity;

        yaw += xOffset;
        pitch += yOffset;

        // Clamp pitch to avoid turning up & down beyond 90 degrees
        pitch = clamp(pitch, -89.0f, 89.0f);

        // Calculate camera direction
        vec3 direction = vec3(
            cos(radians(yaw)) * cos(radians(pitch)),
            sin(radians(pitch)),
            sin(radians(yaw)) * cos(radians(pitch))
        );
        front = normalize(direction);
    }

    // Getters and Setters
    mat4 GetView() { return lookAt(position, position + front, up); }
    vec3 GetPos() { return position; }

private:
    vec3 position;
    vec3 front;
    vec3 up;

    float yaw;
    float pitch;

    float lastXPos;
    float lastYPos;
    bool mouseFirstEntry;
};

class Game {
public:
    Game() :
        window(nullptr),
        program(0),
        waterProgram(0),
        windowWidth(1280),
        windowHeight(720),
        deltaTime(0.0f),
        lastFrame(0.0f),
        timeOfDay(0.5f),
        dayLength(120.0f),          // in seconds, 120.0f = 2 minutes
        lightColour(vec3(0.0f)),
        lightIntensity(0.0f),
        previousCameraChunk(vec3(0.0f)),
        waterTexture(0),
        sandTexture(0),
        grassTexture(0),
        rockTexture(0),
        snowTexture(0),
        sandNormal(0),
        grassNormal(0),
        rockNormal(0),
        snowNormal(0),
        projection(mat4(1.0f)),
        camera(windowWidth, windowHeight)
    {}

    void Initialise() {
        // Initialise GLFW
        glfwInit();

        // Create window
        window = glfwCreateWindow(windowWidth, windowHeight, "LOADING...      PLEASE WAIT.", nullptr, nullptr);
        if (!window) {
            cerr << "Failed to initialise GLFW Window" << endl;
            glfwTerminate();
            return;
        }
        glfwMakeContextCurrent(window);
        glewInit();

        glEnable(GL_DEPTH_TEST);
        glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);    // Use the FramebufferSizeCallback function when window is resized
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);        // Automatically binds cursor to window & hides pointer

        // Enable blending for transparent objects
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // -=-=- Load shaders -=-=-
        ShaderInfo shaders[] = {
            { GL_VERTEX_SHADER, "shaders/vertexShader.vert" },
            { GL_FRAGMENT_SHADER, "shaders/fragmentShader.frag" },
            { GL_NONE, nullptr }
        };
        program = LoadShaders(shaders);

        ShaderInfo waterShaders[] = {
            { GL_VERTEX_SHADER, "shaders/waterVertexShader.vert" },
            { GL_FRAGMENT_SHADER, "shaders/waterFragmentShader.frag" },
            { GL_NONE, nullptr }
        };
        waterProgram = LoadShaders(waterShaders);

        // Set sampler uniform
        int texLoc = glGetUniformLocation(program, "textureSampler");
        glUniform1i(texLoc, 0);

        SetProjectionMatrix();

        // -=-=- Terrain -=-=-
        // Load water texture
        waterTexture = LoadTexture("media/water.jpg");

        // Load terrain textures
        sandTexture = LoadTexture("media/sand.jpg");
        grassTexture = LoadTexture("media/grass.jpg");
        rockTexture = LoadTexture("media/rock.jpg");
        snowTexture = LoadTexture("media/snow.jpg");

        // Load terrain normals
        sandNormal = LoadTexture("media/sand_normal.jpg");
        grassNormal = LoadTexture("media/grass_normal.jpg");
        rockNormal = LoadTexture("media/rock_normal.jpg");
        snowNormal = LoadTexture("media/snow_normal.jpg");

        UpdateTerrainChunks();

        // Remove loading title
        glfwSetWindowTitle(window, "window");
    }

    void HandleInput() {
        // Close window on escape pressed
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        camera.HandleKeyboard(window, deltaTime);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        camera.HandleMouse(x, y);
    }

    void Update() {
        // Calculate delta time
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Advance day/night cycle
        timeOfDay += deltaTime / dayLength;
        if (timeOfDay > 1.0f) {
            timeOfDay = 0.0f;
        }

        // Define skybox colours
        const vec3 dayColour = vec3(0.56f, 0.78f, 0.92f);
        const vec3 twilightColour = vec3(1.0f, 0.6f, 0.2f);
        const vec3 nightColour = vec3(0.04f, 0.02f, 0.08f);

        // Night
        if (timeOfDay < 0.4f) {
            lightColour = nightColour;
            lightIntensity = 0.3f;
        }
        // Sunrise
        else if (timeOfDay < 0.45f) {
            float t = (timeOfDay - 0.4f) / 0.05f;
            lightColour = mix(nightColour, twilightColour, t);
            lightIntensity = mix(0.3f, 0.65f, t);
        }
        else if (timeOfDay < 0.5f) {
            float t = (timeOfDay - 0.45f) / 0.05f;
            lightColour = mix(twilightColour, dayColour, t);
            lightIntensity = mix(0.65f, 1.0f, t);
        }
        // Day
        else if (timeOfDay < 0.9f) {
            lightColour = dayColour;
            lightIntensity = 1.0f;
        }
        // Sunset
        else if (timeOfDay < 0.95f) {
            float t = (timeOfDay - 0.9f) / 0.05f;
            lightColour = mix(dayColour, twilightColour, t);
            lightIntensity = mix(1.0f, 0.65f, t);
        }
        else {
            float t = (timeOfDay - 0.95f) / 0.05f;
            lightColour = mix(twilightColour, nightColour, t);
            lightIntensity = mix(0.65f, 0.3f, t);
        }

        vec3 cameraPosition = camera.GetPos();
        vec3 currentCameraChunk = vec3(
            floor(cameraPosition.x / CHUNK_WORLD_SIZE),
            0.0f,
            floor(cameraPosition.z / CHUNK_WORLD_SIZE)
        );

        // Update chunks when camera enters a new chunk
        if (currentCameraChunk != previousCameraChunk) {
            cout << "Updating chunks..." << endl;
            UpdateTerrainChunks();

            previousCameraChunk = currentCameraChunk;
        }
    }

    void Render() {
        glClearColor(lightColour.r, lightColour.g, lightColour.b, 1.0f);    // RGBA Colour (normalised between 0.0f-1.0f instead of 0-255)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera view matrix sets position of the viewer, movement direction in relation to it & world up direction
        mat4 view = camera.GetView();

        // -=-=- Render Terrain -=-=-
        glUseProgram(program);

        // Render each chunk
        for (auto& pair : terrainChunks) {
            RenderTerrainObject& chunkTerrain = pair.second.terrain;

            // Bind Textures
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.sandTexture);
            glUniform1i(glGetUniformLocation(program, "sandDiffuse"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.grassTexture);
            glUniform1i(glGetUniformLocation(program, "grassDiffuse"), 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.rockTexture);
            glUniform1i(glGetUniformLocation(program, "rockDiffuse"), 2);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.snowTexture);
            glUniform1i(glGetUniformLocation(program, "snowDiffuse"), 3);

            // Bind Normals
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.sandNormal);
            glUniform1i(glGetUniformLocation(program, "sandNormal"), 4);

            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.grassNormal);
            glUniform1i(glGetUniformLocation(program, "grassNormal"), 5);

            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.rockNormal);
            glUniform1i(glGetUniformLocation(program, "rockNormal"), 6);

            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.snowNormal);
            glUniform1i(glGetUniformLocation(program, "snowNormal"), 7);

            // Pass light intensity to shader
            glUniform1f(glGetUniformLocation(program, "lightIntensity"), lightIntensity);

            // Build transform
            mat4 terrainMvp = projection * view * chunkTerrain.modelMatrix;
            glUniformMatrix4fv(glGetUniformLocation(program, "mvpIn"), 1, GL_FALSE, value_ptr(terrainMvp));
            glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, value_ptr(chunkTerrain.modelMatrix));

            glBindVertexArray(chunkTerrain.VAO);
            glDrawElements(GL_TRIANGLES, chunkTerrain.indexCount, GL_UNSIGNED_INT, nullptr);
        }

        // -=-=- Render Water -=-=-
        float waterTimer = (float)glfwGetTime();
        glUseProgram(waterProgram);
        glDepthMask(GL_FALSE);

        // Render each chunk
        for (auto& pair : terrainChunks) {
            RenderWaterObject& chunkWater = pair.second.water;

            // Bind Texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, chunkWater.texture);
            glUniform1i(glGetUniformLocation(waterProgram, "diffuseMap"), 0);

            // Pass needed variables to shader
            glUniform1f(glGetUniformLocation(waterProgram, "lightIntensity"), lightIntensity);
            glUniform1f(glGetUniformLocation(waterProgram, "timer"), waterTimer);
            glUniform1f(glGetUniformLocation(waterProgram, "waterAlpha"), chunkWater.alpha);

            // Build transform
            mat4 waterMvp = projection * view * chunkWater.modelMatrix;
            glUniformMatrix4fv(glGetUniformLocation(waterProgram, "mvpIn"), 1, GL_FALSE, value_ptr(waterMvp));

            glBindVertexArray(chunkWater.VAO);
            glDrawElements(GL_TRIANGLES, chunkWater.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        glDepthMask(GL_TRUE);

        // Refreshing
        glfwSwapBuffers(window);    // Swaps the colour buffer
        glfwPollEvents();           // Queries all GLFW events
    }

    void Run() {
        while (!glfwWindowShouldClose(window)) {
            HandleInput();
            Update();
            Render();
        }
    }

    void CleanUp() {
        glfwTerminate();
    }

    void UpdateTerrainChunks() {
        // Find which chunk the camera is in
        vec3 cameraPosition = camera.GetPos();
        int cameraChunkX = (int)floor(cameraPosition.x / CHUNK_WORLD_SIZE);
        int cameraChunkZ = (int)floor(cameraPosition.z / CHUNK_WORLD_SIZE);

        // Load nearby chunks
        for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; z++) {
            for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++) {
                int currentChunkX = cameraChunkX + x;
                int currentChunkZ = cameraChunkZ + z;

                // Create unique key for current chunk
                ChunkKey key{ currentChunkX, currentChunkZ };

                // If chunk did not already exist, create it
                if (terrainChunks.find(key) == terrainChunks.end()) {
                    TerrainChunk chunk;
                    chunk.chunkX = currentChunkX;
                    chunk.chunkZ = currentChunkZ;

                    chunk.terrain = CreateTerrain(
                        CHUNK_SIZE, CHUNK_SIZE, TILE_SIZE, currentChunkX, currentChunkZ,
                        sandTexture, sandNormal,
                        grassTexture, grassNormal,
                        rockTexture, rockNormal,
                        snowTexture, snowNormal
                    );

                    chunk.water = CreateWater(
                        CHUNK_SIZE, CHUNK_SIZE, TILE_SIZE, currentChunkX, currentChunkZ, 0.5f, waterTexture
                    );

                    // Add current chunk to chunk map
                    terrainChunks[key] = chunk;
                }
            }
        }

        // Unload faraway chunks
        for (auto it = terrainChunks.begin(); it != terrainChunks.end();) {
            int dx = it->second.chunkX - cameraChunkX;
            int dz = it->second.chunkZ - cameraChunkZ;

            // If chunk outside of render distance
            if (abs(dx) > RENDER_DISTANCE || abs(dz) > RENDER_DISTANCE) {
                // Delete GPU resources for chunk
                glDeleteVertexArrays(1, &it->second.terrain.VAO);
                glDeleteBuffers(1, &it->second.terrain.VBO);
                glDeleteBuffers(1, &it->second.terrain.EBO);
                glDeleteVertexArrays(1, &it->second.water.VAO);
                glDeleteBuffers(1, &it->second.water.VBO);
                glDeleteBuffers(1, &it->second.water.EBO);

                // Remove chunk from chunk map
                it = terrainChunks.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Getters and Setters
    void SetWindowSize(int width, int height) {
        windowWidth = width;
        windowHeight = height;
        SetProjectionMatrix();
    }
    void SetProjectionMatrix() {
        projection = perspective(radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 500.0f);
        glViewport(0, 0, windowWidth, windowHeight);
    }

private:
    GLFWwindow* window;
    GLuint program;
    GLuint waterProgram;

    int windowWidth;
    int windowHeight;
    float deltaTime;
    float lastFrame;
    float timeOfDay;
    float dayLength;
    vec3 lightColour;
    float lightIntensity;
    vec3 previousCameraChunk;

    GLuint waterTexture;
    GLuint sandTexture;
    GLuint grassTexture;
    GLuint rockTexture;
    GLuint snowTexture;
    GLuint sandNormal;
    GLuint grassNormal;
    GLuint rockNormal;
    GLuint snowNormal;

    unordered_map<ChunkKey, TerrainChunk, ChunkKeyHash> terrainChunks;
    RenderWaterObject Water;

    mat4 projection;
    Camera camera;
};


// TODO: Fix issue where enlargening window does not render objects in the newly created screen space
void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        // Resizes window based on given width and height values
        game->SetWindowSize(width, height);
    }
}

RenderTerrainObject CreateTerrain(
    int gridWidth, int gridDepth, float tileSize, int chunkX, int chunkZ,
    GLuint sandTexture, GLuint sandNormal,
    GLuint grassTexture, GLuint grassNormal,
    GLuint rockTexture, GLuint rockNormal,
    GLuint snowTexture, GLuint snowNormal
)
{
    RenderTerrainObject object;

    vector<float> vertices;
    vector<unsigned int> indices;

    float offsetX = chunkX * (gridWidth * tileSize + 5.0f);
    float offsetZ = chunkZ * (gridDepth * tileSize + 5.0f);

    // Generate vertices
    for (int z = 0; z <= gridDepth; z++) {
        for (int x = 0; x <= gridWidth; x++) {
            float worldX = offsetX + x * tileSize;
            float worldZ = offsetZ + z * tileSize;

            vec3 normal = GenerateNormal(worldX, worldZ);

            // positions
            vertices.push_back(worldX);
            vertices.push_back(GenerateHeight(worldX, worldZ));
            vertices.push_back(worldZ);

            // normals
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);

            // textures
            vertices.push_back(worldX);
            vertices.push_back(worldZ);
        }
    }

    // Generate indices
    for (int z = 0; z < gridDepth; z++) {
        for (int x = 0; x < gridWidth; x++) {
            int topLeft = z * (gridWidth + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (gridWidth + 1) + x;
            int bottomRight = bottomLeft + 1;

            // first triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    object.indexCount = (unsigned int)indices.size();

    // Generate VAO/VBO/EBO
    glGenVertexArrays(1, &object.VAO);
    glGenBuffers(1, &object.VBO);
    glGenBuffers(1, &object.EBO);
    glBindVertexArray(object.VAO);

    // Vertex data
    glBindBuffer(GL_ARRAY_BUFFER, object.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal data
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture data
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Assign textures
    object.sandTexture = sandTexture;
    object.grassTexture = grassTexture;
    object.rockTexture = rockTexture;
    object.snowTexture = snowTexture;

    // Assign normals
    object.sandNormal = sandNormal;
    object.grassNormal = grassNormal;
    object.rockNormal = rockNormal;
    object.snowNormal = snowNormal;

    glBindVertexArray(0);
    return object;
}

RenderWaterObject CreateWater(int gridWidth, int gridDepth, float tileSize, int chunkX, int chunkZ, float alpha, GLuint waterTexture) {
    RenderWaterObject object;
    object.alpha = alpha;

    float sizeX = gridWidth * tileSize;
    float sizeZ = gridDepth * tileSize;

    float offsetX = chunkX * (sizeX + 5.0f);
    float offsetZ = chunkZ * (sizeZ + 5.0f);

    float vertices[] = {
        // positions                                    // textures
        offsetX + sizeX, WATER_LEVEL, offsetZ + sizeZ,  sizeX, sizeZ,   // top right
        offsetX + sizeX, WATER_LEVEL, offsetZ,          sizeX, 0.0f,    // bottom right
        offsetX,         WATER_LEVEL, offsetZ,          0.0f,  0.0f,    // bottom left
        offsetX,         WATER_LEVEL, offsetZ + sizeZ,  0.0f,  sizeZ    // top left
    };

    unsigned int indices[] = {
        0, 1, 3,    // first triangle
        1, 2, 3     // second triangle
    };
    object.indexCount = 6;

    // Generate VAO/VBO/EBO
    glGenVertexArrays(1, &object.VAO);
    glGenBuffers(1, &object.VBO);
    glGenBuffers(1, &object.EBO);
    glBindVertexArray(object.VAO);

    // Vertex data
    glBindBuffer(GL_ARRAY_BUFFER, object.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture data
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Assign texture
    object.texture = waterTexture;

    glBindVertexArray(0);
    return object;
}

float GenerateHeight(float x, float z) {
    float baseFrequency = 0.02f;    // Higher value = More hills
    float baseAmplitude = 25.0f;    // Higher value = Bigger hills

    float height = 0.0f;        // Accumlated height
    float persistence = 0.35f;  // Amplitude scaling for each octave
    int octaves = 6;            // Higher value = more terrain detail

    for (int i = 0; i < octaves; i++) {
        // Frequency increases per octave
        float frequency = baseFrequency * (float)pow(2.0f, i);

        // Amplitude decreases per octave
        float amplitude = baseAmplitude * (float)pow(persistence, i);

        // Use glm to generate perlin noise and multiply by amplitude
        height += perlin(vec2(x * frequency, z * frequency)) * amplitude;
    }

    return height;
}

vec3 GenerateNormal(float x, float z) {
    float heightL = GenerateHeight(x - 1.0f, z);
    float heightR = GenerateHeight(x + 1.0f, z);
    float heightD = GenerateHeight(x, z - 1.0f);
    float heightU = GenerateHeight(x, z + 1.0f);

    vec3 normal = vec3(heightL - heightR, 2.0f, heightD - heightU);

    return normalize(normal);
}

GLuint LoadTexture(const string& texturePath) {
    GLuint textureID;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Enable texture wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int imageWidth, imageHeight, colourChannels;
    unsigned char* data = stbi_load(texturePath.c_str(), &imageWidth, &imageHeight, &colourChannels, 0);

    // If retrieval successful
    if (data) {
        // Upload texture to GPU
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        cerr << "Failed to load texture: " << texturePath << endl;
    }

    stbi_image_free(data);
    return textureID;
}


int main(int argc, char* argv[]) {
    Game game;
    game.Initialise();

    game.Run();

    game.CleanUp();
    return 0;
}
