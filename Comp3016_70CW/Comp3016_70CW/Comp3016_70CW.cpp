#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>

#include <GLAD/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include "main.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <learnopengl/shader_m.h>
#include <learnopengl/model.h>

#include "glm/glm/ext/vector_float3.hpp"
#include <glm/glm/ext/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/noise.hpp>

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
        const float speed = CAMERA_SPEED * deltaTime;

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
    const mat4 GetView() { return lookAt(position, position + front, up); }
    const vec3 GetPos() { return position; }

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
        program(nullptr),
        waterProgram(nullptr),
        modelProgram(nullptr),
        treeModel(nullptr),
        signatureModel(nullptr),
        signatureModelMatrix(mat4(1.0f)),
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

        // Initialise GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            cerr << "Failed to initialise GLAD" << endl;
            glfwTerminate();
            return;
        }

        glEnable(GL_DEPTH_TEST);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);    // Use the FramebufferSizeCallback function when window is resized
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);        // Automatically binds cursor to window & hides pointer

        // Enable blending for transparent objects
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Load shaders
        program = new Shader("shaders/vertexShader.vert", "shaders/fragmentShader.frag");
        waterProgram = new Shader("shaders/waterVertexShader.vert", "shaders/waterFragmentShader.frag");
        modelProgram = new Shader("shaders/modelVertexShader.vert", "shaders/modelFragmentShader.frag");

        // Set sampler uniform
        program->setInt("textureSampler", 0);

        SetProjectionMatrix();

        // Load models
        treeModel = new Model("media/tree/tree.obj");
        signatureModel = new Model("media/signature/signature.obj");
        signatureModelMatrix = translate(signatureModelMatrix, vec3(80.0f, 25.0f, 58.0f));
        signatureModelMatrix = scale(signatureModelMatrix, vec3(0.75f));

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

        // Generate intial terrain chunks
        QueueNewChunks();
        while (!pendingChunks.empty()) {
            UpdateQueuedChunks();
        }

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
            cout << "Queueing new chunks..." << endl;
            QueueNewChunks();

            previousCameraChunk = currentCameraChunk;
        } else {
            UpdateQueuedChunks();
        }
    }

    void Render() {
        glClearColor(lightColour.r, lightColour.g, lightColour.b, 1.0f);    // RGBA Colour (normalised between 0.0f-1.0f instead of 0-255)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera view matrix sets position of the viewer, movement direction in relation to it & world up direction
        mat4 view = camera.GetView();

        // -=-=- Render Terrain -=-=-
        program->use();

        // Disable backwards face culling for terrain (it breaks LOD skirts)
        glDisable(GL_CULL_FACE);

        // Render each chunk
        for (auto& pair : terrainChunks) {
            RenderTerrainObject& chunkTerrain = pair.second.terrain;

            // Bind Textures
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.sandTexture);
            program->setInt("sandDiffuse", 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.grassTexture);
            program->setInt("grassDiffuse", 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.rockTexture);
            program->setInt("rockDiffuse", 2);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.snowTexture);
            program->setInt("snowDiffuse", 3);

            // Bind Normals
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.sandNormal);
            program->setInt("sandNormal", 4);

            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.grassNormal);
            program->setInt("grassNormal", 5);

            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.rockNormal);
            program->setInt("rockNormal", 6);

            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, chunkTerrain.snowNormal);
            program->setInt("snowNormal", 7);

            // Pass light intensity to shader
            program->setFloat("lightIntensity", lightIntensity);

            // Build transform
            mat4 terrainMvp = projection * view * chunkTerrain.modelMatrix;
            program->setMat4("mvpIn", terrainMvp);
            program->setMat4("model", chunkTerrain.modelMatrix);

            glBindVertexArray(chunkTerrain.VAO);
            glDrawElements(GL_TRIANGLES, chunkTerrain.indexCount, GL_UNSIGNED_INT, nullptr);
        }

        // -=-=- Render Water -=-=-
        float waterTimer = (float)glfwGetTime();
        waterProgram->use();
        glDepthMask(GL_FALSE);

        // Re-enable backwards face culling
        glEnable(GL_CULL_FACE);

        // Render each chunk
        for (auto& pair : terrainChunks) {
            RenderWaterObject& chunkWater = pair.second.water;

            // Bind Texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, chunkWater.texture);
            waterProgram->setInt("diffuseMap", 0);

            // Pass needed variables to shader
            waterProgram->setFloat("lightIntensity", lightIntensity);
            waterProgram->setFloat("timer", waterTimer);
            waterProgram->setFloat("waterAlpha", chunkWater.alpha);

            // Build transform
            mat4 waterMvp = projection * view * chunkWater.modelMatrix;
            waterProgram->setMat4("mvpIn", waterMvp);

            glBindVertexArray(chunkWater.VAO);
            glDrawElements(GL_TRIANGLES, chunkWater.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        glDepthMask(GL_TRUE);

        // -=-=- Render Models -=-=-
        modelProgram->use();

        // Pass light intensity to shader
        modelProgram->setFloat("lightIntensity", lightIntensity);

        // Build transform
        mat4 signatureMvp = projection * view * signatureModelMatrix;
        modelProgram->setMat4("mvpIn", signatureMvp);

        signatureModel->Draw(*modelProgram);

        // Render each chunk
        for (auto& pair : terrainChunks) {
            TerrainChunk& chunk = pair.second;

            // Only render trees in chunks with highest LOD
            if (chunk.LOD == 0) {
                for (auto& tree : chunk.trees) {
                    // Pass light intensity to shader
                    modelProgram->setFloat("lightIntensity", lightIntensity);

                    // Build transform
                    mat4 treeMvp = projection * view * tree;
                    modelProgram->setMat4("mvpIn", treeMvp);

                    treeModel->Draw(*modelProgram);
                }
            }
        }

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
        // Delete shader programs and models
        delete program;
        delete waterProgram;
        delete modelProgram;
        delete treeModel;
        delete signatureModel;

        // Delete OpenGL GPU resources
        glDeleteTextures(1, &waterTexture);
        glDeleteTextures(1, &sandTexture);
        glDeleteTextures(1, &grassTexture);
        glDeleteTextures(1, &rockTexture);
        glDeleteTextures(1, &snowTexture);

        glDeleteTextures(1, &sandNormal);
        glDeleteTextures(1, &grassNormal);
        glDeleteTextures(1, &rockNormal);
        glDeleteTextures(1, &snowNormal);

        for (auto& pair : terrainChunks) {
            glDeleteVertexArrays(1, &pair.second.terrain.VAO);
            glDeleteBuffers(1, &pair.second.terrain.VBO);
            glDeleteBuffers(1, &pair.second.terrain.EBO);

            glDeleteVertexArrays(1, &pair.second.water.VAO);
            glDeleteBuffers(1, &pair.second.water.VBO);
            glDeleteBuffers(1, &pair.second.water.EBO);
        }

        // Terminate GLFW
        glfwTerminate();
    }

    void UpdateQueuedChunks() {
        // Only generate one chunk per frame to reduce lag spikes
        if (!pendingChunks.empty()) {
            QueuedChunk queued = pendingChunks.front();
            pendingChunks.pop();

            ChunkKey key{ queued.chunkX, queued.chunkZ };

            // Create new chunk or override pre-existing chunk with same key (happens when LODed changes)
            TerrainChunk& chunk = terrainChunks[key];

            chunk.isQueued = false;
            chunk.LOD = queued.LOD;

            // If chunk did not already exist, create chunk
            if (queued.isNew) {
                chunk.chunkX = queued.chunkX;
                chunk.chunkZ = queued.chunkZ;

                chunk.terrain = CreateTerrain(
                    CHUNK_SIZE, TILE_SIZE, queued.chunkX, queued.chunkZ, queued.LOD, heightCache,
                    sandTexture, sandNormal,
                    grassTexture, grassNormal,
                    rockTexture, rockNormal,
                    snowTexture, snowNormal
                );

                chunk.water = CreateWater(
                    CHUNK_SIZE, TILE_SIZE, queued.chunkX, queued.chunkZ, 0.5f, waterTexture
                );

                chunk.trees = CreateTrees(
                    CHUNK_SIZE, TILE_SIZE, queued.chunkX, queued.chunkZ
                );
            }
            // Rebuild chunk if LOD has changed
            else {
                // Delete old GPU resources for chunk
                glDeleteVertexArrays(1, &chunk.terrain.VAO);
                glDeleteBuffers(1, &chunk.terrain.VBO);
                glDeleteBuffers(1, &chunk.terrain.EBO);

                chunk.terrain = CreateTerrain(
                    CHUNK_SIZE, TILE_SIZE, queued.chunkX, queued.chunkZ, queued.LOD, heightCache,
                    sandTexture, sandNormal,
                    grassTexture, grassNormal,
                    rockTexture, rockNormal,
                    snowTexture, snowNormal
                );
            }
        }
    }

    void QueueNewChunks() {
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

                int LOD = CalculateLOD(x, z);
                auto it = terrainChunks.find(key);

                // Queue new chunk
                if (it == terrainChunks.end()) {
                    pendingChunks.push({ currentChunkX, currentChunkZ, LOD, true });
                }
                // Queue updating pre-exising chunk
                else if (!it->second.isQueued && it->second.LOD != LOD) {
                    it->second.isQueued = true;
                    pendingChunks.push({ currentChunkX, currentChunkZ, LOD, false });
                }
            }
        }

        // Delete faraway chunks
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
        projection = perspective(radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 1000.0f);
        glViewport(0, 0, windowWidth, windowHeight);
    }

private:
    GLFWwindow* window;
    Shader* program;
    Shader* waterProgram;

    Shader* modelProgram;
    Model* treeModel;
    Model* signatureModel;
    mat4 signatureModelMatrix;

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

    unordered_map<int64_t, float> heightCache;
    unordered_map<ChunkKey, TerrainChunk, ChunkKeyHash> terrainChunks;
    queue<QueuedChunk> pendingChunks;

    mat4 projection;
    Camera camera;
};


void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        // Resizes window based on given width and height values
        game->SetWindowSize(width, height);
    }
}

RenderTerrainObject CreateTerrain(
    int gridSize, float tileSize, int chunkX, int chunkZ, int currentLOD, unordered_map<int64_t, float>& heightCache,
    GLuint sandTexture, GLuint sandNormal,
    GLuint grassTexture, GLuint grassNormal,
    GLuint rockTexture, GLuint rockNormal,
    GLuint snowTexture, GLuint snowNormal
)
{
    RenderTerrainObject object;

    vector<float> vertices;
    vector<unsigned int> indices;

    // LOD step
    int step = 1 << currentLOD;

    // LOD adjusted chunk resolution
    int effectiveGrid = gridSize / step;

    // Chunk offset
    float offsetX = chunkX * (gridSize * tileSize);
    float offsetZ = chunkZ * (gridSize * tileSize);

    // Generate vertices
    for (int z = 0; z <= gridSize; z += step) {
        for (int x = 0; x <= gridSize; x += step) {
            float worldX = offsetX + x * tileSize;
            float worldZ = offsetZ + z * tileSize;

            float height = GetCachedHeight(heightCache, worldX, worldZ);
            vec3 normal = GenerateNormal(heightCache, worldX, worldZ);

            vertices.insert(vertices.end(), {
                worldX, height, worldZ,         // positions
                normal.x, normal.y, normal.z,   // normals
                worldX, worldZ                  // textures
            });
        }
    }

    int rowSize = effectiveGrid + 1;

    // Generate indices
    for (int z = 0; z < effectiveGrid; z++) {
        for (int x = 0; x < effectiveGrid; x++) {
            unsigned int topLeft = z * rowSize + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * rowSize + x;
            unsigned int bottomRight = bottomLeft + 1;

            indices.insert(indices.end(), {
                topLeft, bottomLeft, topRight,      // first triangle
                topRight, bottomLeft, bottomRight   // second triangle
            });
        }
    }

    // -=-=- Edge skirts -=-=-
    // Chunks with full LOD dont need edge skirts
    if (currentLOD > 0) {
        vector<int> north, south, west, east;

        // Find cardinal edges
        for (int i = 0; i <= effectiveGrid; i++) {
            north.push_back(i);
            south.push_back(effectiveGrid * rowSize + i);
            west.push_back(i * rowSize);
            east.push_back(i * rowSize + effectiveGrid);
        }

        // Add skirt to each edge
        AddSkirtStrip(vertices, indices, north);
        AddSkirtStrip(vertices, indices, south);
        AddSkirtStrip(vertices, indices, west);
        AddSkirtStrip(vertices, indices, east);
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

RenderWaterObject CreateWater(int gridSize, float tileSize, int chunkX, int chunkZ, float alpha, GLuint waterTexture) {
    RenderWaterObject object;
    object.alpha = alpha;

    float sizeX = gridSize * tileSize;
    float sizeZ = gridSize * tileSize;

    float offsetX = chunkX * sizeX;
    float offsetZ = chunkZ * sizeZ;

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

vector<mat4> CreateTrees(int gridSize, float tileSize, int chunkX, int chunkZ) {
    vector<mat4> trees;

    // Chunk offset
    float offsetX = chunkX * (gridSize * tileSize);
    float offsetZ = chunkZ * (gridSize * tileSize);

    for (int i = 0; i < TREES_PER_CHUNK; i++) {
        float x = offsetX + (rand() / (float)RAND_MAX) * gridSize * tileSize;
        float z = offsetZ + (rand() / (float)RAND_MAX) * gridSize * tileSize;

        // Cached heights don't work for this due to tree placement needing deterministic sampling
        float y = GenerateHeight(x, z);

        // Stop trees from generating in water or on top of mountains
        if (y < WATER_LEVEL || y > ROCK_LEVEL) {
            i--;
            continue;
        }

        mat4 tree = mat4(1.0f);
        tree = translate(tree, vec3(x - 4.9f, y - 0.1f, z - 4.7f));  // 4.9f, 4.7f = model offset from world coordinates (adjusted for scale)
        tree = scale(tree, vec3(0.5f));

        trees.push_back(tree);
    }

    return trees;
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

float GetCachedHeight(unordered_map<int64_t, float>& heightCache, float x, float z) {
    int ix = (int)floor(x);
    int iz = (int)floor(z);

    // Construct height key
    int64_t key = (int64_t(ix) << 32) ^ int64_t(iz);

    // If key found in height cache return found value
    auto it = heightCache.find(key);
    if (it != heightCache.end()) { return it->second; }

    // If key not found, generate new height value
    float height = GenerateHeight((float)ix, (float)iz);
    heightCache[key] = height;

    return height;
}

vec3 GenerateNormal(unordered_map<int64_t, float>& heightCache, float x, float z) {
    // Sample surrounding heights
    float heightL = GetCachedHeight(heightCache, x - 1.0f, z);
    float heightR = GetCachedHeight(heightCache, x + 1.0f, z);
    float heightD = GetCachedHeight(heightCache, x, z - 1.0f);
    float heightU = GetCachedHeight(heightCache, x, z + 1.0f);

    // Calculate normal vector from height difference
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

static int CalculateLOD(int x, int z) {
    // Calculate distance from player
    int distance = std::max(abs(x), abs(z));

    // Lower LOD values = higher level of detail (0 = max detail)
    if (distance <= 1) { return 0; }
    if (distance <= 2) { return 1; }
    else { return 2; }
}

static int AddSkirtVertex(vector<float>& vertices, int baseIndex) {
    int v = baseIndex * 8;

    // Create new vertex below original edge vertex
    vertices.insert(vertices.end(), {
        vertices[v + 0], vertices[v + 1] - SKIRT_DEPTH, vertices[v + 2],  // positions
        vertices[v + 3], vertices[v + 4], vertices[v + 5],                // normals
        vertices[v + 6], vertices[v + 7]                                  // textures
    });

    return (int)(vertices.size() / 8) - 1;
}

static void AddSkirtStrip(vector<float>& vertices, vector<unsigned int>& indices, const vector<int>& edge) {
    for (size_t i = 0; i < edge.size() - 1; i++) {
        // Original chunk edge vertices
        unsigned int v0 = edge[i];
        unsigned int v1 = edge[i + 1];

        // Skirt vertices directly below edge
        unsigned int s0 = AddSkirtVertex(vertices, v0);
        unsigned int s1 = AddSkirtVertex(vertices, v1);

        // Connect edge and skirt with a quad
        indices.insert(indices.end(), {
            v0, s0, v1,     // first triangle
            v1, s0, s1      // second triangle
        });
    }
}


int main(int argc, char* argv[]) {
    Game game;
    game.Initialise();

    game.Run();

    game.CleanUp();
    return 0;
}
