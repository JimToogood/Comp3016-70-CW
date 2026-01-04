#pragma once
#include <GLAD/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm/ext/matrix_transform.hpp>
#include <string>
#include <vector>

using namespace std;
using namespace glm;


// Define global constants
const float CAMERA_SPEED = 15.0f;
const float WATER_LEVEL = -11.0f;
const float ROCK_LEVEL = 7.0f;
const int CHUNK_SIZE = 100;
const float TILE_SIZE = 2.0;
const float CHUNK_WORLD_SIZE = CHUNK_SIZE * TILE_SIZE;
const int RENDER_DISTANCE = 5;      // Number of chunks loaded in each direction from the camera
const float SKIRT_DEPTH = 15.0f;    // How far below the vertex edge the skirt extends
const int TREES_PER_CHUNK = 25;

class Game;


// All GPU data needed to render terrain
struct RenderTerrainObject {
    GLuint VAO;                 // Vertex array object
    GLuint VBO;                 // Vertex buffer object
    GLuint EBO;                 // Element buffer object

    GLuint sandTexture;
    GLuint sandNormal;

    GLuint grassTexture;
    GLuint grassNormal;

    GLuint rockTexture;
    GLuint rockNormal;

    GLuint snowTexture;
    GLuint snowNormal;

    unsigned int indexCount;    // Number of indices to draw
    mat4 modelMatrix;           // Model transformation

    RenderTerrainObject() : 
        VAO(0), VBO(0), EBO(0), 
        sandTexture(0), sandNormal(0),
        grassTexture(0), grassNormal(0),
        rockTexture(0), rockNormal(0),
        snowTexture(0), snowNormal(0),
        indexCount(0), modelMatrix(mat4(1.0f))
    {}

    void SetPosition(const vec3& pos) {
        modelMatrix = translate(modelMatrix, pos);
    }

    void Rotate(float degrees, const vec3& axis) {
        modelMatrix = rotate(modelMatrix, radians(degrees), axis);
    }
};

// All GPU data needed to render water
struct RenderWaterObject {
    GLuint VAO;                 // Vertex array object
    GLuint VBO;                 // Vertex buffer object
    GLuint EBO;                 // Element buffer object
    GLuint texture;             // Texture ID
    unsigned int indexCount;    // Number of indices to draw
    mat4 modelMatrix;           // Model transformation
    float alpha;                // Transparency alpha value

    RenderWaterObject() : VAO(0), VBO(0), EBO(0), texture(0), indexCount(0), modelMatrix(mat4(1.0f)), alpha(1.0f) {}

    void SetPosition(const vec3& pos) {
        modelMatrix = translate(modelMatrix, pos);
    }

    void Rotate(float degrees, const vec3& axis) {
        modelMatrix = rotate(modelMatrix, radians(degrees), axis);
    }
};

// Data needed for each terrain chunk
struct TerrainChunk {
    RenderTerrainObject terrain;
    RenderWaterObject water;
    vector<mat4> trees;
    int chunkX;
    int chunkZ;
    int LOD;        // Level of Detail
    bool isQueued;

    TerrainChunk() : chunkX(0), chunkZ(0), LOD(0), isQueued(false) {}
};

// Unique key used to identify each terrain chunk
struct ChunkKey {
    int x;
    int z;

    // Unordered map key comparison
    bool operator==(const ChunkKey& other) const {
        return (x == other.x) && (z == other.z);
    }
};

// Combines x and z coords into single hash value
struct ChunkKeyHash {
    size_t operator()(const ChunkKey& key) const {
        // Calculate hashes
        size_t hashX = hash<int>{}(key.x);
        size_t hashZ = hash<int>{}(key.z);

        // Combine hashes
        return hashX ^ (hashZ << 1);
    }
};

// Data needed in pending chunks queue
struct QueuedChunk {
    int chunkX;
    int chunkZ;
    int LOD;    // Level of Detail
    bool isNew;
};

// Window resize logic
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

// Function to create textured terrain chunk
RenderTerrainObject CreateTerrain(
    int gridSize, float tileSize, int chunkX, int chunkZ, int currentLOD, unordered_map<int64_t, float>& heightCache,
    GLuint sandTexture, GLuint sandNormal,
    GLuint grassTexture, GLuint grassNormal,
    GLuint rockTexture, GLuint rockNormal,
    GLuint snowTexture, GLuint snowNormal
);

// Function to create textured flat water
RenderWaterObject CreateWater(int gridSize, float tileSize, int chunkX, int chunkZ, float alpha, GLuint waterTexture);

// Function to randomly create trees based on terrain height
vector<mat4> CreateTrees(int gridSize, float tileSize, int chunkX, int chunkZ);

// Function generate y values for terrain mapping
float GenerateHeight(float x, float z);

// Retrieve cached height values, from generate height function
float GetCachedHeight(unordered_map<int64_t, float>& heightCache, float x, float z);

// Function generate normal values
vec3 GenerateNormal(unordered_map<int64_t, float>& heightCache, float x, float z);

// Load texture image from given file location
GLuint LoadTexture(const string& texturePath);

// Calculate LOD based on distance from player
static int CalculateLOD(int x, int z);

// Adds a single vertex beneath existing terrain edge vertex
static int AddSkirtVertex(vector<float>& vertices, int baseIndex);

// Creates a continuous vertex skirt along an edge of a chunk
static void AddSkirtStrip(vector<float>& vertices, vector<unsigned int>& indices, const vector<int>& edge);
