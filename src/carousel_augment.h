#pragma once
#include <GL/glew.h>
#include <iostream>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "common/renderable.h"
#include "common/carousel/carousel.h"
#include "common/carousel/carousel_to_renderable.h"

#define N_GROUND_TILES 20.0   // the terrain is covered with 20^2 tiles

inline void pushVec3ToBuffer(std::vector<float>& buffer, glm::vec3 v) {
    buffer.push_back(v.x);
    buffer.push_back(v.y);
    buffer.push_back(v.z);
}

// N = length of the curb in vertices
inline std::vector<GLuint> generateTrackTriangles(int N) {
    std::vector<GLuint> v;
    N = 2 * N;       // the track vertex buffer is twice as long as the curb
    v.resize(3 * N); // each triangle takes up 3 slots
    int slot = 0;
    for (int vertex = 0; vertex < N; vertex += 2) {  // indices specified in right-hand winding order (normal = up)
        v[slot] = vertex % N;
        v[slot + 1] = (vertex + 1) % N;
        v[slot + 2] = (vertex + 3) % N;
        v[slot + 3] = vertex % N;
        v[slot + 4] = (vertex + 3) % N;
        v[slot + 5] = (vertex + 2) % N;
        slot += 6;
    }

    return v;
}

inline std::vector<GLfloat> generateTrackTextureCoords(track t) {
    std::vector<GLfloat> v;

    std::vector<glm::vec3> left = t.curbs[1];
    std::vector<glm::vec3> right = t.curbs[0];

    unsigned int N = left.size();
    v.resize(4 * N);         // one vertex for each side, each 2D vertex takes up 2 slots

    float step = glm::length(left[100] - left[0]);   // we take the length of the first 100 curb segments as a unit; this is arbitrary

    float d_left = 0;
    float d_right = 0;
    unsigned int slot = 0;
    for (unsigned int i = 0; i < N; i++) {
        v[slot] = d_left / step;
        v[slot + 1] = 0.f;
        v[slot + 2] = d_left / step;
        v[slot + 3] = 1.f;

        d_left += glm::length(left[(i + 1) % N] - left[i]);
        d_right += glm::length(right[(i + 1) % N] - right[i]);

        slot += 4;
    }

    return v;
}

inline std::vector<GLfloat> generateTerrainTextureCoords(terrain t) {
    std::vector<GLfloat> v;
    const unsigned int Z = (t.size_pix[1]);
    const unsigned int X = (t.size_pix[0]);
    v.resize(2 * Z * X);

    unsigned int slot = 0;
    for (unsigned int iz = 0; iz < Z; ++iz) {
        for (unsigned int ix = 0; ix < X; ++ix) {
            v[slot] = N_GROUND_TILES * ix / float(X);
            v[slot + 1] = N_GROUND_TILES * iz / float(Z);
            slot += 2;
        }
    }

    return v;
}

inline float getTerrainHeight(terrain t, const unsigned int i, const unsigned int j) {
    const unsigned int& Z = static_cast<unsigned int>(t.size_pix[1]);
    const unsigned int& X = static_cast<unsigned int>(t.size_pix[0]);

    float x = t.rect_xz[0] + (i / float(X)) * t.rect_xz[2];
    float z = t.rect_xz[1] + (j / float(Z)) * t.rect_xz[3];

    return t.hf((i >= X) ? (X - 1) : (i), (j >= Z) ? (Z - 1) : (j));
}

inline glm::vec3 computeVertexNormal(terrain t, unsigned int ix, unsigned int iz) {
   float hL = getTerrainHeight(t, ix - 1, iz);
   float hR = getTerrainHeight(t, ix + 1, iz);
   float hD = getTerrainHeight(t, ix, iz - 1);
   float hU = getTerrainHeight(t, ix, iz + 1);

   glm::vec3 n;
   n.x = (hL - hR) / 2.f;
   n.z = (hD - hU) / 2.f;
   n.y = 1.0f;
   return n;
}

inline void generateTerrainVertexNormals(terrain t, renderable& r) {
    const unsigned int& Z = static_cast<unsigned int>(t.size_pix[1]);
    const unsigned int& X = static_cast<unsigned int>(t.size_pix[0]);

    std::vector<float> normals;
    for (unsigned int iz = 0; iz < Z; ++iz) {
        for (unsigned int ix = 0; ix < X; ++ix) {
           pushVec3ToBuffer(normals, computeVertexNormal(t, ix, iz));
        }
    }

    r.add_vertex_attribute<float>(&normals[0], X * Z * 3, 2, 3);
}

inline void generateTrackVertexNormals(track t, renderable& r) {
    unsigned int size = t.curbs[0].size();

    std::vector<float> normals;
    for (unsigned int i = 0; i < size; ++i) {
        glm::vec3 V0 = t.curbs[0][(i + 1) % size] - t.curbs[0][i];
        glm::vec3 V1 = t.curbs[1][(i + 1) % size] - t.curbs[0][i];
        glm::vec3 U = t.curbs[1][i] - t.curbs[0][i];

        pushVec3ToBuffer(normals, glm::normalize(glm::cross(V0, U)));
        pushVec3ToBuffer(normals, glm::normalize(glm::cross(V1, U)));
    }

    r.add_vertex_attribute<float>(&normals[0], 2 * 3 * size, 2, 3);
}

void inline prepareTrack(race r, renderable& r_track) {
   std::cout << "Generating track... ";

   r_track.create();
   game_to_renderable::to_track(r, r_track);

   std::vector<GLuint> trackTriangles = generateTrackTriangles(r.t().curbs[0].size());
   r_track.add_indices<GLuint>(&trackTriangles[0], (unsigned int)trackTriangles.size(), GL_TRIANGLES);

   std::vector<GLfloat> trackTextureCoords = generateTrackTextureCoords(r.t());
   r_track.add_vertex_attribute<GLfloat>(&trackTextureCoords[0], trackTextureCoords.size(), 4, 2);

   generateTrackVertexNormals(r.t(), r_track);
   std::cout << "done" << std::endl;
}

void inline prepareTerrain(race r, renderable& r_terrain) {
   std::cout << "Generating terrain... ";

   r_terrain.create();
   game_to_renderable::to_heightfield(r, r_terrain);

   std::vector<GLfloat> terrainTextureCoords = generateTerrainTextureCoords(r.ter());
   r_terrain.add_vertex_attribute<GLfloat>(&terrainTextureCoords[0], terrainTextureCoords.size(), 4, 2);

   generateTerrainVertexNormals(r.ter(), r_terrain);
   std::cout << "done" << std::endl;
}