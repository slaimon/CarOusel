#define NANOSVG_IMPLEMENTATION   // Expands implementation
#include "../3dparty/nanosvg/src/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../3dparty/nanosvg/src/nanosvgrast.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL

#include "../common/debugging.h"
#include "../common/renderable.h"
#include "../common/shaders.h"
#include "../common/simple_shapes.h"
#include "../common/matrix_stack.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "../common/gltf_loader.h"
#include "../common/texture.h"

#include "../common/carousel/carousel.h"
#include "../common/carousel/carousel_to_renderable.h"
#include "../common/carousel/carousel_loader.h"

#include "carousel_augment.h"
#include "camera_controls.h"
#include "transformations.h"
#include "projector.h"

#include <algorithm>
#include <glm/glm.hpp>

#define BASE_DIR std::string("C:/Users/Slaimon/Desktop/unipi/CG/CarOusel/")

std::string shaders_path = BASE_DIR + "src/shaders/";
std::string textures_path = BASE_DIR + "src/assets/textures/";
std::string models_path = BASE_DIR + "src/assets/models/";


template <typename T>
void printVector (std::vector<T> v) {
   std::stringstream s;
   
   std::cout << "ARRAY SIZE: " << v.size() << std::endl;
   for (int i=0; i<v.size(); i++) {
      s << v[i] << " ";
   }
   std::cout << s.str() << std::endl;
}

void printVec3 (glm::vec3 v) {
   std::cout << "< " << v.x << " , " << v.y << " , " << v.z << " >" << std::endl;
}


int width = 1440;
int height = 900;

unsigned int shadowmapSize = 2048;

#define COLOR_RED    glm::vec3(1.f,0.f,0.f)
#define COLOR_GREEN  glm::vec3(0.f,1.f,0.f)
#define COLOR_BLUE   glm::vec3(0.f,0.f,1.f)
#define COLOR_BLACK  glm::vec3(0.f,0.f,0.f)
#define COLOR_WHITE  glm::vec3(1.f,1.f,1.f)
#define COLOR_YELLOW glm::vec3(1.f,1.f,0.f)

// textures and shading
typedef enum shadingMode {
   SHADING_TEXTURED_FLAT,     // 0
   SHADING_NORMALMAP,         // 1
   SHADING_MONOCHROME_FLAT,   // 2
   SHADING_TEXTURED_PHONG,    // 3
   SHADING_MONOCHROME_PHONG   // 4
} shadingMode_t;

typedef enum textureSlot {
   TEXTURE_DEFAULT,
   TEXTURE_GRASS,
   TEXTURE_ROAD,
   TEXTURE_DIFFUSE,
   TEXTURE_NORMAL,
   TEXTURE_SHADOWMAP
} textureSlot_t;

texture texture_grass_diffuse, texture_track_diffuse;
void load_textures() {
   texture_grass_diffuse.load(textures_path + "grass_diff.png", TEXTURE_GRASS);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);  // enable mipmap for minification
   texture_track_diffuse.load(textures_path + "road_diff.png", TEXTURE_ROAD);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
}

// bounding boxes and renderables for the loaded .glb models
box3 bbox_car, bbox_camera, bbox_lamp, bbox_tree;
std::vector <renderable> model_car, model_camera, model_lamp, model_tree;
void load_models() {
   gltf_loader gltfLoader;
   gltfLoader.load_to_renderable(models_path + "car1.glb", model_car, bbox_car);
   gltfLoader.load_to_renderable(models_path + "camera4.glb", model_camera, bbox_camera);
   gltfLoader.load_to_renderable(models_path + "lamp2.glb", model_lamp, bbox_lamp);
   gltfLoader.load_to_renderable(models_path + "styl-pine.glb", model_tree, bbox_tree);
}

// the passed shader MUST be already active!
void drawLoadedModel(matrix_stack stack, std::vector<renderable> obj, box3 bbox, shader s) {
   float scale = 1.f / bbox.diagonal();
   stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(scale)));
   stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(-bbox.center())));
   
   // render each renderable
   for (unsigned int i = 0; i < obj.size(); ++i) {
      obj[i].bind();
      stack.push();
      // each object had its own transformation that was read in the gltf file
      stack.mult(obj[i].transform);
      
      if (obj[i].mater.base_color_texture != -1) {
         glActiveTexture(GL_TEXTURE0 + TEXTURE_DIFFUSE);
         glBindTexture(GL_TEXTURE_2D, obj[i].mater.base_color_texture);
      }
      
      if (obj[i].mater.normal_texture != -1) {
         glActiveTexture(GL_TEXTURE0 + TEXTURE_NORMAL);
         glBindTexture(GL_TEXTURE_2D, obj[i].mater.normal_texture);
      }
      
      glUniform1i(s["uColorImage"], TEXTURE_DIFFUSE);
      glUniform1i(s["uNormalmapImage"], TEXTURE_NORMAL);
      glUniformMatrix4fv(s["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
      glDrawElements(obj[i]().mode, obj[i]().count, obj[i]().itype, 0);   
      stack.pop();
   }
}

// set up camera controls

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void updateDelta() {
   float currentFrame = glfwGetTime();
   deltaTime = currentFrame - lastFrame;
   lastFrame = currentFrame;
}

#define CAMERA_FAST 0.250f
#define CAMERA_SLOW 0.025f

CameraControls camera(glm::vec3(0.0f, 0.5f, 1.0f), glm::vec3(0.0f), CAMERA_FAST);
unsigned int POVselected = 0; // will keep track of how many times the user has requested a POV switch
bool fineMovement = false;
bool debugView = false;
bool timeStep = true;
bool lampState = true;
bool drawShadows = true;
bool sunState = true;
float playerMinHeight = 0.01;

float scale;
glm::vec3 center;

void processInput(GLFWwindow* window, terrain t) {
   if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

   if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.step(CAMERA_AHEAD, deltaTime);
   if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.step(CAMERA_BACK, deltaTime);
   if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.step(CAMERA_LEFT, deltaTime);
   if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.step(CAMERA_RIGHT, deltaTime);
   if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
      camera.step(CAMERA_UP, deltaTime);
   if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      glm::vec3 pos = camera.getPosition();
      //if (pos.y > t.y(pos.x, pos.z))
      if (pos.y > playerMinHeight)
         camera.step(CAMERA_DOWN, deltaTime);
   }
}

void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
   if (action == GLFW_PRESS && ((mods & GLFW_MOD_CONTROL)==0)) {
      switch (key) {
         // switch between fast and slow movement speed of the camera
         case GLFW_KEY_E:
            camera.setSpeed( fineMovement ? (CAMERA_FAST) : (CAMERA_SLOW) );
            fineMovement = !fineMovement;
            break;
         
         // switch to the next point of view
         case GLFW_KEY_C:
            POVselected++;
            break;
         
         case GLFW_KEY_V:
            debugView = !debugView;
            break;
         
         case GLFW_KEY_T:
            timeStep = !timeStep;
            break;
         
         case GLFW_KEY_L:
            lampState = !lampState;
            break;
         
         case GLFW_KEY_K:
            sunState = !sunState;
            break;

         case GLFW_KEY_Q:
            drawShadows = !drawShadows;
            break;
      }
   }  
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
   camera.mouseLook(xpos/width, ypos/height);
}


race r;
shader shader_basic, shader_world, shader_depth;
renderable fram;
void draw_frame() {
   glUseProgram(shader_basic.program);
   glUniformMatrix4fv(shader_basic["uModel"], 1, GL_FALSE, &glm::mat4(1.f)[0][0]);
   glUniform3f(shader_basic["uColor"], -1.f, 0.6f, 0.f);
   fram.bind();
   glDrawArrays(GL_LINES, 0, 6);
   glUseProgram(0);
}

void draw_sunDir(glm::vec3 sundir) {
   glUseProgram(shader_basic.program);
   glUniform3f(shader_basic["uColor"], 0.f, 0.f, 0.f);
   glUniformMatrix4fv(shader_basic["uModel"], 1, GL_FALSE, &glm::mat4(1.f)[0][0]);
   glColor3f(0, 0, 1);
   glBegin(GL_LINES);
   glVertex3f(0, 0, 0);
   glVertex3f(sundir.x, sundir.y, sundir.z);
   glEnd();
   glUseProgram(0);
}

renderable r_terrain;
void draw_terrain(shader sh, matrix_stack stack) {
   glUseProgram(sh.program);
   r_terrain.bind();
   
   glActiveTexture(GL_TEXTURE0 + TEXTURE_GRASS);
   glBindTexture(GL_TEXTURE_2D, texture_grass_diffuse.id);
   glUniform1i(sh["uMode"], SHADING_TEXTURED_PHONG);
   glUniform1i(sh["uColorImage"], TEXTURE_GRASS);
   glUniformMatrix4fv(sh["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
   glDrawElements(r_terrain().mode, r_terrain().count, r_terrain().itype, 0);
   glUseProgram(0);
}

renderable r_track;
void draw_track(shader sh, matrix_stack stack) {
   glUseProgram(sh.program);
   glEnable(GL_POLYGON_OFFSET_FILL);
   glPolygonOffset(0.1, 0.1);
   r_track.bind();

   // we bump the track upwards to prevent it from falling under the terrain
   stack.push();
   stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.15f, 0.f)));
   
   glActiveTexture(GL_TEXTURE0 + TEXTURE_ROAD);
   glBindTexture(GL_TEXTURE_2D, texture_track_diffuse.id);
   glUniform1i(sh["uMode"], SHADING_TEXTURED_FLAT);
   glUniform1i(sh["uColorImage"], TEXTURE_ROAD);
   glUniformMatrix4fv(sh["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
   glDrawElements(r_track().mode, r_track().count, r_track().itype, 0);
   glDisable(GL_POLYGON_OFFSET_FILL);
   
   stack.pop();
   glUseProgram(0);
}

void draw_cars(shader sh, matrix_stack stack) {
   glUseProgram(sh.program);
   //glDisable(GL_CULL_FACE);
   //glEnable(GL_BLEND);
   //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   for (unsigned int ic = 0; ic < r.cars().size(); ++ic) {
      stack.push();
      stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(0.f,.5f,0.f))); // bump the car upwards so the wheels don't clip into the terrain
      stack.mult(r.cars()[ic].frame);
      stack.mult(glm::scale(glm::mat4(1.), glm::vec3(3.5f)));
      stack.mult(glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0.f,1.f,0.f)));  // make it face the direction it's going
      
      glUniform1i(sh["uMode"], SHADING_NORMALMAP);
      drawLoadedModel(stack, model_car, bbox_car, sh);
      stack.pop();
   }
   glUseProgram(0);
}

std::vector<bool> draw_cameraman;
void draw_cameramen(shader sh, matrix_stack stack) {
   // draw each cameraman
   glUseProgram(sh.program);
   for (unsigned int ic = 0; ic < r.cameramen().size(); ++ic) {
      if (!draw_cameraman[ic])
         continue;
      stack.push();
      stack.mult(r.cameramen()[ic].frame);
      stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(2.5f)));
      stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(0.f,0.25f,0.f)));
      stack.mult(glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0.f,1.f,0.f)));
      
      glUniform3f(sh["uColor"], 0.2f, 0.2f, 0.2f);
      glUniform1i(sh["uMode"], SHADING_MONOCHROME_PHONG);
      
      drawLoadedModel(stack, model_camera, bbox_camera, sh);
      stack.pop();
   }
   glUseProgram(0);
}

renderable r_sphere;
std::vector<glm::mat4> lampT;
std::vector<glm::vec3> lampLightPos;
void draw_lightBulbs(shader sh) {
   glUseProgram(sh.program);
   for (unsigned int i = 0; i < lampT.size(); ++i) {
      r_sphere.bind();
      glm::mat4 model = glm::translate(glm::mat4(1.f), lampLightPos[i]);
                model = glm::scale(model, glm::vec3(0.00075f));
      glUniformMatrix4fv(sh["uModel"], 1, GL_FALSE, &model[0][0]);
      glUniform3f(sh["uColor"], 1.f, 0.63f, 0.08f);
      glUniform1i(sh["uMode"], SHADING_MONOCHROME_FLAT);
      glDrawElements(r_sphere().mode, r_sphere().count, r_sphere().itype, 0);
   }
   glUseProgram(0);
}
void draw_lamps(shader sh, matrix_stack stack) {
   glUseProgram(sh.program);
   for (unsigned int i = 0; i < lampT.size(); ++i) {
      stack.push();
      stack.load(lampT[i]);

      glUniform1i(sh["uMode"], SHADING_TEXTURED_PHONG);
      glUniform3f(sh["uColor"], 0.2f, 0.2f, 0.2f);
      drawLoadedModel(stack, model_lamp, bbox_lamp, sh);
      
      stack.pop();
   }
   glUseProgram(0);
}

std::vector<glm::mat4> treeT;
void draw_trees(shader sh, matrix_stack stack) {
   glUseProgram(sh.program);
   glDisable(GL_CULL_FACE);
   for (unsigned int i = 0; i < treeT.size(); i++) {
      stack.push();
      stack.load(treeT[i]);
      
      glUniform1i(sh["uMode"], SHADING_TEXTURED_PHONG);
      glUniform3f(sh["uColor"], 0.3f, 0.9f, 0.3f);
      drawLoadedModel(stack, model_tree, bbox_tree, sh);
      
      stack.pop();
   }
   glUseProgram(0);
}

renderable r_trees;
void draw_debugTrees(matrix_stack stack) {
   glUseProgram(shader_basic.program);
   glUniformMatrix4fv(shader_basic["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
   glUniform3f(shader_basic["uColor"], 0.f, 1.0f, 0.f);
   
   r_trees.bind();
   glDrawArrays(GL_LINES, 0, r_trees.vn);
   glUseProgram(0);
}

shader shader_fsq;
renderable r_quad;
void draw_texture(GLint tex_id) {
   GLint at;
   glGetIntegerv(GL_ACTIVE_TEXTURE, &at);
   glUseProgram(shader_fsq.program);

   glActiveTexture(GL_TEXTURE0 + TEXTURE_SHADOWMAP);
   glBindTexture(GL_TEXTURE_2D, tex_id);
   glUniform1i(shader_fsq["uTexture"], TEXTURE_SHADOWMAP);
   r_quad.bind();
   glDisable(GL_CULL_FACE);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
   
   glUseProgram(0);
   glActiveTexture(at);
}

renderable r_cube;
// draws the frustum represented by the given projection matrix
void draw_frustum(glm::mat4 projMatrix, glm::vec3 color) {
   r_cube.bind();
   glUseProgram(shader_basic.program);
   glUniform3f(shader_basic["uColor"], color.r, color.g, color.b);
   glUniformMatrix4fv(shader_basic["uModel"], 1, GL_FALSE, &glm::inverse(projMatrix)[0][0]);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_cube.elements[1].ind);
   glDrawElements(r_cube.elements[1].mode, r_cube.elements[1].count, r_cube.elements[1].itype, 0);
}

// draws a box3
void draw_bbox(box3 bbox, glm::vec3 color) {
   glm::mat4 T = glm::translate(glm::mat4(1.f), bbox.center());
             T = glm::scale(T, glm::abs(bbox.max - bbox.min) / 2.f);
   glUniform3f(shader_basic["uColor"], color.r, color.g, color.b);
   glUniformMatrix4fv(shader_basic["uModel"], 1, GL_FALSE, &T[0][0]);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_cube.elements[1].ind);
   glDrawElements(r_cube.elements[1].mode, r_cube.elements[1].count, r_cube.elements[1].itype, 0);
}


void draw_scene(matrix_stack stack, bool depthOnly) {
   shader sh;
   if (depthOnly) {
      sh = shader_depth;
   }
   else {
      //draw_frame();
      draw_sunDir(r.sunlight_direction());
      sh = shader_world;
   }
   
   // draw terrain and track
   if (depthOnly)
       glDisable(GL_CULL_FACE);   // terrain and track are not watertight
   else {
       glEnable(GL_CULL_FACE);
       glCullFace(GL_BACK);
   }

   glFrontFace(GL_CW);
   draw_terrain(sh, stack);
    check_gl_errors(__LINE__, __FILE__);
   draw_track(sh, stack);
    check_gl_errors(__LINE__, __FILE__);

   // in the depth pass, cull the front face from the following watertight models
   if (depthOnly) {
      glEnable(GL_CULL_FACE);
      glCullFace(GL_FRONT);
   }

   // the following models have opposite polygon handedness
   glFrontFace(GL_CCW);
   //draw_cars(sh, stack);
    check_gl_errors(__LINE__, __FILE__);
   //draw_cameramen(sh, stack);
    check_gl_errors(__LINE__, __FILE__);
   //draw_trees(sh, stack);
    check_gl_errors(__LINE__, __FILE__);
   draw_lamps(sh, stack);
   //draw_lightBulbs(sh);
    check_gl_errors(__LINE__, __FILE__);
}





int main(int argc, char** argv) {
   carousel_loader::load((BASE_DIR + "src/small_test.svg").c_str(), (BASE_DIR + "src/terrain_256.png").c_str(), r);
   
   //add 10 cars
   for (int i = 0; i < 10; ++i)      
      r.add_car();

   GLFWwindow* window;
   if (!glfwInit())
      return -1;
   
   window = glfwCreateWindow(width, height, "CarOusel", NULL, NULL);
   if (!window) {
      glfwTerminate();
      return -1;
   }

   if (glfwRawMouseMotionSupported())
      glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

   glfwMakeContextCurrent(window);
   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
   glfwSetCursorPosCallback(window, mouse_callback);
   glfwSetKeyCallback(window, keyboard_callback);
   glewInit();
   printout_opengl_glsl_info();
   
   // load the 3D models
      load_models();


   // begin creating the world
   
   fram = shape_maker::frame();
   r_sphere = shape_maker::sphere(2);
   r_quad = shape_maker::quad();
   
   shape s_cube;
   shape_maker::cube(s_cube);
   s_cube.compute_edges();
   s_cube.to_renderable(r_cube);

   // prepare the track
      r_track.create();
      game_to_renderable::to_track(r, r_track);
      
      std::vector<GLuint> trackTriangles = generateTrackTriangles(r.t().curbs[0].size());
      r_track.add_indices<GLuint>(&trackTriangles[0], (unsigned int) trackTriangles.size(), GL_TRIANGLES);
      
      std::vector<GLfloat> trackTextureCoords = generateTrackTextureCoords(r.t());
      r_track.add_vertex_attribute<GLfloat>(&trackTextureCoords[0], trackTextureCoords.size(), 4, 2);

      //generateTrackVertexNormals(r.t(), r_track);


   // prepare the terrain
      r_terrain.create();
      game_to_renderable::to_heightfield(r, r_terrain);
      
      std::vector<GLfloat> terrainTextureCoords = generateTerrainTextureCoords(r.ter());
      r_terrain.add_vertex_attribute<GLfloat>(&terrainTextureCoords[0], terrainTextureCoords.size(), 4, 2);

      generateTerrainVertexNormals(r.ter(), r_terrain);
   
   // prepare the debug trees
      r_trees.create();
      game_to_renderable::to_tree(r, r_trees);
   
   draw_cameraman.resize(r.cameramen().size());
   for(unsigned int i=0; i<draw_cameraman.size(); i++)
      draw_cameraman[i] = true;


   glViewport(0, 0, width, height);
   shader_basic.create_program((shaders_path + "basic.vert").c_str(), (shaders_path + "basic.frag").c_str());
   shader_world.create_program((shaders_path + "world.vert").c_str(), (shaders_path + "world.frag").c_str());
   shader_depth.create_program((shaders_path + "depth.vert").c_str(), (shaders_path + "depth.frag").c_str());
   shader_fsq.create_program((shaders_path + "fsq.vert").c_str(), (shaders_path + "fsq.frag").c_str());
      
   glm::mat4 proj = glm::perspective(glm::radians(45.f), (float)width/(float)height, 0.001f, 10.f);
   
   load_textures();
   glUseProgram(shader_world.program);
   glUniformMatrix4fv(shader_world["uProj"], 1, GL_FALSE, &proj[0][0]);
   glUniformMatrix4fv(shader_world["uView"], 1, GL_FALSE, &camera.matrix()[0][0]);
   glUniform1i(shader_world["uColorImage"],0);
   
   glUseProgram(shader_basic.program);
   glUniformMatrix4fv(shader_basic["uProj"], 1, GL_FALSE, &proj[0][0]);
   glUniformMatrix4fv(shader_basic["uView"], 1, GL_FALSE, &camera.matrix()[0][0]);

   
   // center the view on the scene
   matrix_stack stack;
   scale = 1.f/r.bbox().diagonal();
   center = r.bbox().center();
   stack.load_identity();
   stack.push();
   stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(scale)));
   stack.mult(glm::translate(glm::mat4(1.f), -center));

   // initialize sunlight direction
   r.start(11,0,0,600);
   r.update();
   glUseProgram(shader_world.program);
   glUniform3f(shader_world["uSun"], r.sunlight_direction().x, r.sunlight_direction().y, r.sunlight_direction().z);
   
   // we transform the scene's bounding box in world coordinates to pass it to the Sun Projector
   box3 bbox_scene = r.bbox();
   bbox_scene.min = glm::vec3(stack.m() * glm::vec4(bbox_scene.min, 1.0));
   bbox_scene.max = glm::vec3(stack.m() * glm::vec4(bbox_scene.max, 1.0));
   bbox_scene.max.y = 0.1f;
   DirectionalProjector sunProjector(bbox_scene, shadowmapSize, -r.sunlight_direction());
   
   glUseProgram(shader_depth.program);
   glUniformMatrix4fv(shader_depth["uLightMatrix"], 1, GL_FALSE, &sunProjector.lightMatrix()[0][0]);
   
   glUseProgram(shader_world.program);
   glUniform1i(shader_world["uShadowMapSize"], shadowmapSize);
   glUseProgram(0);
   

   // initialize the lamps' and their lights' positions
   lampT = lampTransform(r.t(), r.lamps(), scale, center);
   lampLightPos = lampLightPositions(lampT);
   glUseProgram(shader_world.program);
   glUniform3fv(glGetUniformLocation(shader_world.program, "uLamps"), lampLightPos.size(), &lampLightPos[0][0]);
   glUseProgram(0);
   PositionalProjector lampProjector(bbox_scene, shadowmapSize, lampLightPos[0]);
   
   // initialize the trees' positions
   treeT = treeTransform(r.trees(), scale, center);
   

   glEnable(GL_DEPTH_TEST);
   while (!glfwWindowShouldClose(window)) {
      glClearColor(0.3f, 0.3f, 0.3f, 1.f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
      check_gl_errors(__LINE__, __FILE__);
      
      if (timeStep)
         r.update();
      
      // ruota il sole per farlo allineare con r.sunlight_direction()
      sunProjector.setDirection(-r.sunlight_direction());
      glUseProgram(shader_depth.program);
      glUniformMatrix4fv(shader_depth["uLightMatrix"], 1, GL_FALSE, &sunProjector.lightMatrix()[0][0]);

      
      // ci prepariamo a disegnare il buffer per la shadowmap
      glBindFramebuffer(GL_FRAMEBUFFER, sunProjector.getFrameBufferID());
      glViewport(0, 0, shadowmapSize, shadowmapSize);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      
      // primo passaggio:
      // calcoliamo solo il depth buffer
      draw_scene(stack, true);
      
      
      // bind the shadow map to the shader and update the sun's uniforms
      glUseProgram(shader_world.program);
      glActiveTexture(GL_TEXTURE0 + TEXTURE_SHADOWMAP);
      glBindTexture(GL_TEXTURE_2D, sunProjector.getTextureID());
      glUniform1i(shader_world["uShadowMap"], TEXTURE_SHADOWMAP);
      glUniform3f(shader_world["uSun"], r.sunlight_direction().x, r.sunlight_direction().y, r.sunlight_direction().z);
      glUniformMatrix4fv(shader_world["uSunMatrix"], 1, GL_FALSE, &sunProjector.lightMatrix()[0][0]);
      glUniform1f(shader_world["uLampState"], (lampState)?(1.0):(0.0));
      glUniform1f(shader_world["uSunState"], (sunState) ? (1.0) : (0.0));
      glUseProgram(0);
      
      
      // ci prepariamo a disegnare il buffer che andrà sullo schermo
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, width, height);
      
      // secondo passaggio:
      // disegnamo tutta la scena
      draw_scene(stack, false);
      
      if (debugView) {
         draw_frustum(lampProjector.lightMatrix(), COLOR_YELLOW);
         draw_frustum(sunProjector.lightMatrix(), COLOR_WHITE);
         draw_bbox(bbox_scene, COLOR_BLACK);
         
         // show the shadow map 
         glViewport(0, 0, 200, 200);
         glDisable(GL_DEPTH_TEST);
         draw_texture(sunProjector.getTextureID());
         glEnable(GL_DEPTH_TEST);
         glViewport(0, 0, width, height);
      }

      
      // update camera view according to incoming input
      updateDelta();
      processInput(window, r.ter());
      
      glm::mat4 viewMatrix;
      int currentPOV = POVselected % (1+r.cameramen().size()); 
      if (currentPOV == 0) {
         viewMatrix = camera.matrix();
         draw_cameraman[r.cameramen().size()-1] = true;
      }
      else {
         stack.push();
         stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, 0.f))); // we bump the cameraman's POV upwards a little bit
         // the cameraman's view matrix is the inverse of its frame, but first we scale it back up so that the projection matrix isn't affected
         viewMatrix = glm::inverse( glm::scale(stack.m() * r.cameramen()[currentPOV-1].frame, glm::vec3(1.f/scale)) );
         draw_cameraman[currentPOV-1] = false;
         if (currentPOV > 1)
            draw_cameraman[currentPOV-2] = true;
         stack.pop();
      }
      
      glUseProgram(shader_world.program);  
      glUniformMatrix4fv(shader_world["uView"], 1, GL_FALSE, &viewMatrix[0][0]);
      glUniform1f(shader_world["uDrawShadows"], (drawShadows)?(1.0):(0.0));
      glUseProgram(shader_basic.program);
      glUniformMatrix4fv(shader_basic["uView"], 1, GL_FALSE, &viewMatrix[0][0]);
      
      /* Swap front and back buffers */
      glfwSwapBuffers(window);

      /* Poll for and process events */
      glfwPollEvents();
   }
   glUseProgram(0);
   glfwTerminate();
   return 0;
}