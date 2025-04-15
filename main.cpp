/**
* Author: RyanWong
* Assignment: PongClone
* Date due: 2025/3/01, 11:59 PM
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>


enum AppStatus { RUNNING, TERMINATED };

constexpr float WINDOW_SIZE_MULT = 1.0f;

constexpr int WINDOW_WIDTH  = 640 * WINDOW_SIZE_MULT,
              WINDOW_HEIGHT = 480 * WINDOW_SIZE_MULT;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

bool automatic = false;

bool left = false;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char LAKERS_SPRITE_FILEPATH[] = "lakers.png",
               CAVS_SPRITE_FILEPATH[] = "cavs.png",
               BALL_SPRITE_FILEPATH[]  = "ball.png";

constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;
constexpr glm::vec3 INIT_SCALE_ball = glm::vec3(0.7f, 0.7f, 0.0f),
                    INIT_SCALE_lakers = glm::vec3(0.8f, 1.8f, 0.0f),
                    INIT_SCALE_cavs = glm::vec3(0.8f, 1.8f, 0.0f),
                    INIT_POS_lakers  = glm::vec3(2.0f, 0.0f, 0.0f),
                    INIT_POS_cavs  = glm::vec3(-6.8f, 0.0f, 0.0f),
                    INIT_POS_ball   = glm::vec3(-4.0f, -2.0f, 0.0f);

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix, g_lakers_matrix, g_cavs_matrix, g_projection_matrix, g_ball_matrix;

float g_previous_ticks = 0.0f;

GLuint g_lakers_texture_id;
GLuint g_cavs_texture_id;
GLuint g_ball_texture_id;

glm::vec3 g_lakers_position = glm::vec3(2.4f, 0.0f, 0.0f);
glm::vec3 g_lakers_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_cavs_position = glm::vec3(2.4f, 0.0f, 0.0f);
glm::vec3 g_cavs_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_cavs_automatic = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_ball_position = glm::vec3(-2.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(1.0f, 0.8f, 0.0f);

float g_lakers_speed = 5.0f;  // move 1 unit per second
float g_cavs_speed = 5.0f;  // move 1 unit per second

float g_ball_speed = 5.0f;

void initialise();
void process_input();
void update();
void render();
void shutdown();

constexpr GLint NUMBER_OF_TEXTURES = 1;  // to be generated, that is
constexpr GLint LEVEL_OF_DETAIL    = 0;  // base image level; Level n is the nth mipmap reduction image
constexpr GLint TEXTURE_BORDER     = 0;  // this value MUST be zero

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("LeBron Pong",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
    
    if (g_display_window == nullptr)
    {
        shutdown();
    }
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_lakers_matrix = glm::mat4(1.0f);
    g_cavs_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, glm::vec3(1.0f, 1.0f, 0.0f));
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);
    
    g_lakers_texture_id = load_texture(LAKERS_SPRITE_FILEPATH);
    g_cavs_texture_id = load_texture(CAVS_SPRITE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_lakers_movement = glm::vec3(0.0f);
    g_cavs_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
                                                                             
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_app_status = TERMINATED;
                        break;
                                                                             
                    default:
                        break;
                }
                                                                             
            default:
                break;
        }
    }
                                                                             
                                                                             
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    
    if (key_state[SDL_SCANCODE_UP])
    {
        g_lakers_movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_DOWN])
    {
        g_lakers_movement.y = -1.0f;
    }
    if (key_state[SDL_SCANCODE_W])
    {
        if (not automatic) {
            g_cavs_movement.y = 1.0f;
        }
    }
    else if (key_state[SDL_SCANCODE_S])
    {
        if (not automatic) {
            g_cavs_movement.y = -1.0f;
        }
    }
    if (key_state[SDL_SCANCODE_T])
    {
        automatic = true;
        g_cavs_automatic.y = 1.0f;
    }
                                                                             
    // This makes sure that the player can't "cheat" their way into moving
    // faster
    if (glm::length(g_lakers_movement) > 1.0f)
    {
        g_lakers_movement = glm::normalize(g_lakers_movement);
    }
    if (glm::length(g_cavs_movement) > 1.0f)
    {
        g_cavs_movement = glm::normalize(g_cavs_movement);
    }
}

void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;
    
    g_ball_position += g_ball_movement * g_ball_speed * delta_time;
    if ((g_ball_position.y > 5.4 and g_ball_movement.y > 0) or (g_ball_position.y < -1.4f and g_ball_movement.y < 0)) {
        g_ball_movement.y *= -1;
    }
    
    if (g_ball_position.x > 8.8f or g_ball_position.x < -0.7f) {
        g_app_status = TERMINATED;
    }

    // Add direction * units per second * elapsed time
    g_lakers_position += g_lakers_movement * g_lakers_speed * delta_time;
    if (g_lakers_position.y > 2.9) {
        g_lakers_position.y = 2.9;
    }
    else if (g_lakers_position.y < -2.9) {
        g_lakers_position.y = -2.9;
    }
    
    g_lakers_matrix = glm::mat4(1.0f);
    g_lakers_matrix = glm::translate(g_lakers_matrix, INIT_POS_lakers);
    g_lakers_matrix = glm::translate(g_lakers_matrix, g_lakers_position);
    
    if (automatic) {
        g_cavs_position += g_cavs_automatic * g_cavs_speed * delta_time;
    }
    else {
        g_cavs_position += g_cavs_movement * g_cavs_speed * delta_time;
    }
    if (g_cavs_position.y >= 2.9) {
        g_cavs_position.y = 2.9;
        if (automatic) {
            g_cavs_automatic.y = -1.0f;
        }
    }
    else if (g_cavs_position.y <= -2.9) {
        g_cavs_position.y = -2.9;
        if (automatic) {
            g_cavs_automatic.y = 1.0f;
        }
    }
    
    g_cavs_matrix = glm::mat4(1.0f);
    g_cavs_matrix = glm::translate(g_cavs_matrix, INIT_POS_cavs);
    g_cavs_matrix = glm::translate(g_cavs_matrix, g_cavs_position);
    
    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, INIT_POS_ball);
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);
    
    g_lakers_matrix = glm::scale(g_lakers_matrix, INIT_SCALE_lakers);
    g_cavs_matrix = glm::scale(g_cavs_matrix, INIT_SCALE_cavs);
    g_ball_matrix  = glm::scale(g_ball_matrix, INIT_SCALE_ball);
    
    // Collision
    float x_lakers_distance = fabs(g_lakers_position.x + INIT_POS_lakers.x - INIT_POS_ball.x - g_ball_position.x) -
        ((INIT_SCALE_ball.x + INIT_SCALE_lakers.x) / 2.0f);
    float y_lakers_distance = fabs(g_lakers_position.y + INIT_POS_lakers.y - INIT_POS_ball.y - g_ball_position.y) -
        ((INIT_SCALE_ball.y + INIT_SCALE_lakers.y) / 2.0f);
    float x_cavs_distance = fabs(g_cavs_position.x + INIT_POS_cavs.x - INIT_POS_ball.x - g_ball_position.x) -
        ((INIT_SCALE_ball.x + INIT_SCALE_cavs.x) / 2.0f);
    float y_cavs_distance = fabs(g_cavs_position.y + INIT_POS_cavs.y - INIT_POS_ball.y - g_ball_position.y) -
        ((INIT_SCALE_ball.y + INIT_SCALE_cavs.y) / 2.0f);
    
    if (not left && x_lakers_distance < 0.0f && y_lakers_distance < 0.0f)
    {
        g_ball_movement.x *= -1;
        left = true;
    }
    else if (left && x_cavs_distance < 0.0f && y_cavs_distance < 0.0f)
    {
        g_ball_movement.x *= -1;
        left = false;
    }
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    // Bind texture
    draw_object(g_lakers_matrix, g_lakers_texture_id);
    draw_object(g_cavs_matrix, g_cavs_texture_id);
    draw_object(g_ball_matrix, g_ball_texture_id);
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();
    
    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
