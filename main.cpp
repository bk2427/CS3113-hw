#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

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

#include <thread>
#include <chrono>


const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 0.100f,
            BG_BLUE = 0.000f,
            BG_GREEN = 0.300f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

SDL_Window* display_window;

ShaderProgram program;

//constants and float values
const float GROWTH_FACTOR = 1.001f;  // growth rate of 1.0% per frame
const float SHRINK_FACTOR = 0.999f;  // growth rate of -1.0% per frame
const int MAX_FRAME = 10;           // this value is, of course, up to you

int frame_counter = 0;

float g_scale = 1;
float fly = 0.0f;
const float ROT_ANGLE = 0.05f;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

float previous_ticks = 0.0f;


float vertices_star_3[] =
{
    -0.5f, -2.5f, 0.5f, -2.5f, 0.5f, -1.5f, //triangle 1
    -0.5f, -2.5f, 0.5f, -1.5f, -0.5f, -1.5f //triangle 2
};

float vertices_star_2[] =
{
    -3.0f, 1.0f, -2.0f, 1.0f, -2.0f, 2.0f, //triangle 1
    -3.0f, 1.0f, -2.0f, 2.0f, -3.0, 2.0f  //triangle 2
};

float vertices_star_1[] =
{
    2.0f, 1.0f, 3.0f, 1.0f, 3.0f, 2.0f, //triangle 1
    2.0f, 1.0f, 3.0f, 2.0f, 2.0f, 2.0f //triangle 2
};

float vertices_moon[] =
{
    -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,  // triangle 1
    -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f   // triangle 2
};

float vertices_kiki[] =
{
    4.0f, -0.5f, 5.0f, -0.5f, 5.0f, 0.5f,  // triangle 1
    4.0f, -0.5f, 5.0f, 0.5f, 4.0f, 0.5f   // triangle 2
};

float texture_coordinates[] =
{
    0.f, 1.f, 1.f, 1.f, 1.f, 0.f, //triangle 1
    0.f, 1.f, 1.f, 0.f, 0.f, 0.f //triangle 2
};

//sprite images
const char KIKI_FILEPATH[] = "kiki.png";
const char MOON_FILEPATH[] = "moon.png";
const char STARS_FILEPATH_1[] = "starss.png";
const char STARS_FILEPATH_2[] = "starss.png";
const char STARS_FILEPATH_3[] = "starss.png";

//texture ids
GLuint kiki_texture_id, moon_texture_id,
star1_texture_id, star2_texture_id, star3_texture_id;
//bools
bool is_growing = true;
bool game_is_running = true;
bool hit_boundary_x_max = false;
bool hit_boundary_y_max = false;

//matrices
glm::mat4 view_matrix, model_matrix_kiki, model_matrix_moon, model_matrix_stars, projection_matrix;
const glm::mat4 MAX_SIZE = glm::mat4(4.0f);

//function prototypes
GLuint load_texture(const char* filepath);
void initialise();
void process_input();
void update();
void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id);
void render();
void shutdown();

//functions
int main(int argc, char* argv[])
{
    initialise();

    while (game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}

GLuint load_texture(const char* filepath) {
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL) {
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    display_window = SDL_CreateWindow("kiki!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(display_window);
    SDL_GL_MakeCurrent(display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    program.Load(V_SHADER_PATH, F_SHADER_PATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    view_matrix = glm::mat4(1.0f);
    model_matrix_kiki = glm::mat4(1.0f);
    model_matrix_moon= glm::mat4(1.0f);
    model_matrix_stars = glm::mat4(1.0f);
    projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projection_matrix);
    program.SetViewMatrix(view_matrix);

    glUseProgram(program.programID);

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    kiki_texture_id = load_texture(KIKI_FILEPATH);
    moon_texture_id = load_texture(MOON_FILEPATH);
    star1_texture_id = load_texture(STARS_FILEPATH_1);
    star2_texture_id = load_texture(STARS_FILEPATH_2);
    star3_texture_id = load_texture(STARS_FILEPATH_3);

}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            game_is_running = false;
        }
    }
}

void update()
{

    float ticks = (float) SDL_GetTicks() / 1000.0f; // get the current number of ticks
    float delta_time = ticks - previous_ticks; // the delta time is the difference from the last frame
    previous_ticks = ticks;
    
//    stars scaling
    glm::vec3 scale_vector;
    frame_counter += 1;
    if (frame_counter >= MAX_FRAME)
    {
        is_growing = !is_growing;
        frame_counter = 0;
    }
    scale_vector = glm::vec3(is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                             is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                             1.0f);
    model_matrix_stars = glm::scale(model_matrix_stars, scale_vector);
    
//    moon spinning
    model_matrix_moon = glm::rotate(model_matrix_moon, ROT_ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
    
//    kiki flying
    fly -= 0.005f * delta_time;
    model_matrix_kiki = glm::translate(model_matrix_kiki, glm::vec3(fly, 0.0f, 0.0f));
}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    program.SetModelMatrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);


    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices_moon);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program.texCoordAttribute);
    draw_object(model_matrix_moon, moon_texture_id);

    //draw stars
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices_star_1);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program.texCoordAttribute);
    draw_object(model_matrix_stars, star1_texture_id);
    
    
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices_star_2);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program.texCoordAttribute);
    draw_object(model_matrix_stars, star2_texture_id);
    
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices_star_3);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program.texCoordAttribute);
    draw_object(model_matrix_stars, star2_texture_id);

    //kiki
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices_kiki);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program.texCoordAttribute);
    draw_object(model_matrix_kiki, kiki_texture_id);

    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

    SDL_GL_SwapWindow(display_window);
}

void shutdown() { SDL_Quit(); }
