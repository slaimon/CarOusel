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
#include "headlights.h"
#include "projector.h"
#include "lamps.h"

#include <algorithm>
#include <glm/glm.hpp>

#define BASE_DIR std::string("C:/Users/Slaimon/Desktop/unipi/CG/CarOusel/")

std::string shaders_path = BASE_DIR + "src/shaders/";
std::string textures_path = BASE_DIR + "src/assets/textures/";
std::string models_path = BASE_DIR + "src/assets/models/";

int width = 1440;
int height = 900;

#define COLOR_RED    glm::vec3(1.f,0.f,0.f)
#define COLOR_GREEN  glm::vec3(0.f,1.f,0.f)
#define COLOR_BLUE   glm::vec3(0.f,0.f,1.f)
#define COLOR_BLACK  glm::vec3(0.f,0.f,0.f)
#define COLOR_WHITE  glm::vec3(1.f,1.f,1.f)
#define COLOR_YELLOW glm::vec3(1.f,1.f,0.f)

// opening angles of the street lamps' beam
#define LAMP_ANGLE_IN   glm::radians(15.0f)
#define LAMP_ANGLE_OUT  glm::radians(50.0f)

// opening angle of the headlights' beam
#define HEADLIGHT_ANGLE  glm::radians(50.f)
// how many cars should be displayed
#define CARS_NUM 1

// determines the time of day the lights should turn on/off
// insert the angular distance of the Sun above the horizon
#define LAMP_NIGHTTIME_THRESHOLD         20.0
#define HEADLIGHT_NIGHTTIME_THRESHOLD     0.0

float lamp_nighttime = glm::cos(glm::radians(90.0 - LAMP_NIGHTTIME_THRESHOLD));
float headlight_nighttime = glm::cos(glm::radians(90.0 - HEADLIGHT_NIGHTTIME_THRESHOLD));

// shadowmap sizes
#define SUN_SHADOWMAP_SIZE         2048u
#define LAMP_SHADOWMAP_SIZE         512u
#define HEADLIGHT_SHADOWMAP_SIZE    512u

// textures and shading
typedef enum shadingMode {
   SHADING_TEXTURED_FLAT,     // 0
   SHADING_MONOCHROME_FLAT,   // 1
   SHADING_TEXTURED_PHONG,    // 2
   SHADING_MONOCHROME_PHONG   // 3
} shadingMode_t;

typedef enum textureSlot {
   TEXTURE_DEFAULT,
   TEXTURE_GRASS,
   TEXTURE_ROAD,
   TEXTURE_DIFFUSE,
   TEXTURE_SHADOWMAP_SUN,
   TEXTURE_SHADOWMAP_LAMPS,
   TEXTURE_SHADOWMAP_CARS
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

shader shader_basic, shader_world, shader_depth, shader_fsq;
void load_shaders() {
   shader_basic.create_program((shaders_path + "basic.vert").c_str(), (shaders_path + "basic.frag").c_str());
   shader_world.create_program((shaders_path + "world.vert").c_str(), (shaders_path + "world.frag").c_str());
   shader_depth.create_program((shaders_path + "depth.vert").c_str(), (shaders_path + "depth.frag").c_str());
   shader_fsq.create_program((shaders_path + "fsq.vert").c_str(), (shaders_path + "fsq.frag").c_str());
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
      
      glUniform1i(s["uColorImage"], TEXTURE_DIFFUSE);
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
bool drawShadows = true;
bool sunState = true;
bool lampState = false;
bool lampUserState = false;
bool headlightState = false;
bool headlightUserState = false;
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
            lampUserState = !lampUserState;
            break;
         
         case GLFW_KEY_K:
            sunState = !sunState;
            break;

         case GLFW_KEY_J:
            headlightUserState = !headlightUserState;
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

bool isDaytime(glm::vec3 sunlight_direction) {
   return glm::dot(sunlight_direction, glm::vec3(0.f, 1.f, 0.f)) >= 0.f;
}


race r;
renderable fram;
void draw_frame(glm::mat4 F) {
   glUseProgram(shader_basic.program);
   glUniformMatrix4fv(shader_basic["uModel"], 1, GL_FALSE, &F[0][0]);
   glUniform3f(shader_basic["uColor"], -1.f, 0.6f, 0.f);
   fram.bind();
   glDrawArrays(GL_LINES, 0, 6);
   glUseProgram(0);
}

void draw_sunDirection(glm::vec3 sundir) {
   glUseProgram(shader_basic.program);
   glUniform3f(shader_basic["uColor"], 1.f, 1.f, 1.f);
   glUniformMatrix4fv(shader_basic["uModel"], 1, GL_FALSE, &glm::mat4(1.f)[0][0]);
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
   glUniform1f(sh["uShininess"], 25.f);
   glUniform1f(sh["uDiffuse"], 0.9f);
   glUniform1f(sh["uSpecular"], 0.1f);
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
   glUniform1i(sh["uMode"], SHADING_TEXTURED_PHONG);
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
      stack.mult(r.cars()[ic].frame);
      stack.mult(glm::scale(glm::mat4(1.), glm::vec3(3.5f)));
      stack.mult(glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0.f,1.f,0.f)));  // make the car face the direction it's going
      stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.15f, 0.f))); // bump it upwards so the wheels don't clip into the terrain
      
      glUniform1i(sh["uMode"], SHADING_TEXTURED_PHONG);
      glUniform1f(sh["uShininess"], 75.f);
      glUniform1f(sh["uDiffuse"], 0.7f);
      glUniform1f(sh["uSpecular"], 0.7f);
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
      
      glUniform1i(sh["uMode"], SHADING_MONOCHROME_PHONG);
      glUniform3f(sh["uColor"], 0.2f, 0.2f, 0.2f);
      glUniform1f(sh["uShininess"], 50.f);
      glUniform1f(sh["uDiffuse"], 0.9f);
      glUniform1f(sh["uSpecular"], 0.6f);
      
      drawLoadedModel(stack, model_camera, bbox_camera, sh);
      stack.pop();
   }
   glUseProgram(0);
}

renderable r_sphere;
std::vector<glm::mat4> lampT;
void draw_lamps(shader sh, matrix_stack stack) {
   glUseProgram(sh.program);
   glUniform1i(sh["uMode"], SHADING_TEXTURED_PHONG);
   glUniform1f(sh["uShininess"], 75.f);
   glUniform1f(sh["uDiffuse"], 0.7f);
   glUniform1f(sh["uSpecular"], 0.7f);
   for (unsigned int i = 0; i < lampT.size(); ++i) {
      stack.push();
      stack.load(lampT[i]);

      drawLoadedModel(stack, model_lamp, bbox_lamp, sh);
      
      stack.pop();
   }
   glUseProgram(0);
}

std::vector<glm::mat4> treeT;
void draw_trees(shader sh, matrix_stack stack) {
   glUseProgram(sh.program);
   glDisable(GL_CULL_FACE);

   glUniform1i(sh["uMode"], SHADING_TEXTURED_PHONG);
   glUniform1f(sh["uShininess"], 25.f);
   glUniform1f(sh["uDiffuse"], 1.f);
   glUniform1f(sh["uSpecular"], 0.1f);
   for (unsigned int i = 0; i < treeT.size(); i++) {
      stack.push();
      stack.load(treeT[i]);
      
      drawLoadedModel(stack, model_tree, bbox_tree, sh);
      
      stack.pop();
   }
   glUseProgram(0);
}

renderable r_quad;
void draw_texture(GLint tex_id, unsigned int texture_slot) {
   GLint at;
   glGetIntegerv(GL_ACTIVE_TEXTURE, &at);
   glUseProgram(shader_fsq.program);

   glActiveTexture(GL_TEXTURE0 + texture_slot);
   glBindTexture(GL_TEXTURE_2D, tex_id);
   glUniform1i(shader_fsq["uTexture"], texture_slot);
   r_quad.bind();
   glDisable(GL_CULL_FACE);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
   
   glUseProgram(0);
   glActiveTexture(at);
}

renderable r_cube;
// draws the frustum represented by the given projection matrix
void draw_frustum(glm::mat4 projMatrix, glm::vec3 color) {
   glUseProgram(shader_basic.program);
   
   r_cube.bind();
   glUniform3f(shader_basic["uColor"], color.r, color.g, color.b);
   glUniformMatrix4fv(shader_basic["uModel"], 1, GL_FALSE, &glm::inverse(projMatrix)[0][0]);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_cube.elements[1].ind);
   glDrawElements(r_cube.elements[1].mode, r_cube.elements[1].count, r_cube.elements[1].itype, 0);

   glUseProgram(0);
}

// draws a box3
void draw_bbox(box3 bbox, glm::vec3 color) {
   glUseProgram(shader_basic.program);

   r_cube.bind();
   glm::mat4 T = glm::translate(glm::mat4(1.f), bbox.center());
             T = glm::scale(T, glm::abs(bbox.max - bbox.min) / 2.f);
   glUniform3f(shader_basic["uColor"], color.r, color.g, color.b);
   glUniformMatrix4fv(shader_basic["uModel"], 1, GL_FALSE, &T[0][0]);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_cube.elements[1].ind);
   glDrawElements(r_cube.elements[1].mode, r_cube.elements[1].count, r_cube.elements[1].itype, 0);

   glUseProgram(0);
}


void draw_scene(matrix_stack stack, bool depthOnly) {
   shader sh;
   if (depthOnly) {
      sh = shader_depth;
   }
   else {
      //draw_frame(glm::mat4(1.f));
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

   // the following models have opposite polygon handedness
   glFrontFace(GL_CCW);
   draw_cars(sh, stack);

   // in the depth pass, cull the front face from the following watertight models
   if (depthOnly) {
      glEnable(GL_CULL_FACE);
      glCullFace(GL_FRONT);
   }

    //check_gl_errors(__LINE__, __FILE__);
   draw_cameramen(sh, stack);
    //check_gl_errors(__LINE__, __FILE__);
   draw_trees(sh, stack);
    //check_gl_errors(__LINE__, __FILE__);
   draw_lamps(sh, stack);
    check_gl_errors(__LINE__, __FILE__);
}





int main(int argc, char** argv) {
   carousel_loader::load((BASE_DIR + "src/small_test.svg").c_str(), (BASE_DIR + "src/terrain_256.png").c_str(), r);
   
   for (int i = 0; i < CARS_NUM; ++i)      
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

   prepareTrack(r, r_track);
   prepareTerrain(r, r_terrain);
   
   draw_cameraman.resize(r.cameramen().size());
   for(unsigned int i=0; i<draw_cameraman.size(); i++)
      draw_cameraman[i] = true;


   glViewport(0, 0, width, height);
   glm::mat4 proj = glm::perspective(glm::radians(45.f), (float)width/(float)height, 0.001f, 10.f);
   
   load_textures();
   load_shaders();
   glUseProgram(shader_world.program);
   glUniformMatrix4fv(shader_world["uProj"], 1, GL_FALSE, &proj[0][0]);
   glUniformMatrix4fv(shader_world["uView"], 1, GL_FALSE, &camera.matrix()[0][0]);
   glUniform1i(shader_world["uColorImage"], 0);
   
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

   // initialize scene
   r.start(11,0,0,600);
   r.update();
   
   // transform the scene's bounding box in world coordinates to pass it to the Sun Projector
   box3 bbox_scene = r.bbox();
   bbox_scene.min = glm::vec3(stack.m() * glm::vec4(bbox_scene.min, 1.0));
   bbox_scene.max = glm::vec3(stack.m() * glm::vec4(bbox_scene.max, 1.0));
   bbox_scene.max.y = 0.1f;
   DirectionalProjector sunProjector(bbox_scene, SUN_SHADOWMAP_SIZE, r.sunlight_direction());
   bool daytime = isDaytime(r.sunlight_direction());
   
   // initialize the sun's uniforms
   glUseProgram(shader_world.program);
   sunProjector.updateLightDirectionUniform(shader_world, "uSunDirection");
   sunProjector.bindTexture(TEXTURE_SHADOWMAP_SUN);
   glUniform1i(shader_world["uSunShadowmap"], TEXTURE_SHADOWMAP_SUN);
   glUniform1i(shader_world["uSunShadowmapSize"], SUN_SHADOWMAP_SIZE);
   glUseProgram(0);
   

   // initialize the lamps and their lights
   lampT = lampTransform(r.t(), r.lamps(), scale, center);
   LampGroup lamps(lampLightPositions(lampT), LAMP_ANGLE_OUT, LAMP_SHADOWMAP_SIZE, TEXTURE_SHADOWMAP_LAMPS);
   unsigned int numActiveLamps = 3;
   lamps.toggle(10);
   lamps.toggle(11);
   lamps.toggle(14);

   glUseProgram(shader_world.program);
   glUniform1f(shader_world["uLampAngleIn"], glm::cos(LAMP_ANGLE_IN));
   glUniform1f(shader_world["uLampAngleOut"], glm::cos(LAMP_ANGLE_OUT));
   glUniform3f(shader_world["uLampDirection"], 0.f, -1.f, 0.f);
   glUniform3fv(shader_world["uLamps"], lamps.getSize(), &lamps.getPositions()[0][0]);
   glUniformMatrix4fv(shader_world["uLampMatrix"], lamps.getSize(), GL_FALSE, &lamps.getLightMatrices()[0][0][0]);
   glUniform1iv(shader_world["uLampShadowmaps"], lamps.getSize(), &lamps.getTextureSlots()[0]);
   glUniform1i(shader_world["uLampShadowmapSize"], LAMP_SHADOWMAP_SIZE);
   glUseProgram(0);
   
   // initialize the trees
   treeT = treeTransform(r.trees(), scale, center);
   
   // initialize the headlights
   Headlights headlights(HEADLIGHT_ANGLE, center, scale, HEADLIGHT_SHADOWMAP_SIZE);

   int texture_slots_cars[2] =
   { TEXTURE_SHADOWMAP_CARS + r.lamps().size(),
     TEXTURE_SHADOWMAP_CARS + r.lamps().size() + 1 };

   glUseProgram(shader_world.program);
   glUniform1iv(shader_world["uHeadlightShadowmap"], 2, &texture_slots_cars[0]);
   glUniform1i(shader_world["uHeadlightShadowmapSize"], HEADLIGHT_SHADOWMAP_SIZE);
   glUseProgram(0);


   glEnable(GL_DEPTH_TEST);
   while (!glfwWindowShouldClose(window)) {
      // in case the window was resized
      glfwGetWindowSize(window, &width, &height);
      glViewport(0, 0, width, height);

      glClearColor(0.3f, 0.3f, 0.3f, 1.f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
      check_gl_errors(__LINE__, __FILE__);
      
      if (timeStep) {
         r.update();
         lamps.setSunlightSwitch(r.sunlight_direction(), lamp_nighttime);
         headlights.setSunlightSwitch(r.sunlight_direction(), headlight_nighttime);
         daytime = isDaytime(r.sunlight_direction());
      }

      // update the headlights' view matrices
      headlights.setCarFrame(r.cars()[0].frame);

         glUseProgram(shader_world.program);
         headlights.updateLightMatrixUniformArray(shader_world, "uHeadlightMatrix");
         headlights.updatePositionUniformArray(shader_world, "uHeadlightPos");

      // draw the headlights' shadowmaps
      if (headlightState && drawShadows) {
         for (int i = 0; i < 2; ++i) {
            glUseProgram(shader_depth.program);
            headlights.updateLightMatrixUniform(i, shader_depth, "uLightMatrix");
            headlights.bindFramebuffer(i);
            headlights.bindTexture(i, texture_slots_cars[i]);
            draw_scene(stack, true);
            glUseProgram(0);
         }
      }

      lamps.setUserSwitch(lampUserState);
      lampState = lamps.isOn();
      headlights.setUserSwitch(headlightUserState);
      headlightState = headlights.isOn();

      // update the sun's uniform in the depth and world shaders
      sunProjector.setDirection(r.sunlight_direction());
      
         glUseProgram(shader_depth.program);
         sunProjector.updateLightMatrixUniform(shader_depth, "uLightMatrix");
         sunProjector.bindFramebuffer();

         glUseProgram(shader_world.program);
         sunProjector.updateLightDirectionUniform(shader_world, "uSunDirection");
         sunProjector.updateLightMatrixUniform(shader_world, "uSunMatrix");
      
      // draw the sun's shadowmap
      if (sunState && drawShadows && daytime) {
         sunProjector.bindTexture(TEXTURE_SHADOWMAP_SUN);
         draw_scene(stack, true);
      }

      // draw the lamps' shadowmaps
      if (lampState && drawShadows) {
         for (unsigned int i = 0; i < numActiveLamps; ++i) {
            glUseProgram(shader_depth.program);
            lamps.updateLightMatrixUniform(i, shader_depth, "uLightMatrix");
            lamps.bindFramebuffer(i);
            lamps.bindTexture(i);
            draw_scene(stack, true);
         }
         glUseProgram(0);
      }

      // draw the screen buffer
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, width, height);
      draw_scene(stack, false);
      

      if (debugView) {
         draw_frustum(sunProjector.lightMatrix(), COLOR_WHITE);
         draw_bbox(bbox_scene, COLOR_BLACK);
         draw_sunDirection(r.sunlight_direction());

         if (lampState)
            for (unsigned int i = 0; i < numActiveLamps; ++i)
               draw_frustum(lamps.getLightMatrix(i), COLOR_YELLOW);

         if (headlightState) {
            draw_frustum(headlights.getMatrix(0), COLOR_RED);
            draw_frustum(headlights.getMatrix(1), COLOR_RED);
         }

         // show the sun's shadow map
         if (drawShadows && daytime) {
            glViewport(0, 0, 200, 200);
            glDisable(GL_DEPTH_TEST);
            draw_texture(sunProjector.getTextureID(), TEXTURE_SHADOWMAP_SUN);
            glEnable(GL_DEPTH_TEST);
            glViewport(0, 0, width, height);
         }
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
      glUniform1f(shader_world["uSunState"], (sunState) ? (1.0) : (0.0));
      glUniform1f(shader_world["uLampState"], (lampState) ? (1.0) : (0.0));
      glUniform1f(shader_world["uHeadlightState"], (headlightState) ? (1.0) : (0.0));
      glUseProgram(shader_basic.program);
      glUniformMatrix4fv(shader_basic["uView"], 1, GL_FALSE, &viewMatrix[0][0]);
      
      glfwSwapBuffers(window);
      glfwPollEvents();
   }

   glUseProgram(0);
   glfwTerminate();

   return 0;
}