#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#define LOG(statement) std::cout << statement << '\n';
#include <time.h>
using namespace std;

enum Coordinate {
    x_coordinate,
    y_coordinate
};

const int WINDOW_WIDTH = 1000,
WINDOW_HEIGHT = 600;

const float BG_RED = 0.000,
BG_BLUE = 0.000f,
BG_GREEN = 0.000f;
const float BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;


const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";
const char BACKGROUND_SPRITE[] = "court.jpg";
const char PEACH_SPRITE[] = "peach.png";
const char DAISY_SPRITE[] = "daisytennis.png";
const char BALL_SPRITE[] = "ball.png";


GLuint court_texture_id, peach_texture_id, daisy_texture_id, ball_texture_id;

SDL_Window* display_window;
bool game_is_running = true;

ShaderProgram program;
glm::mat4 view_matrix;
glm::mat4 projection_matrix;
glm::mat4 model_matrix_p;
glm::mat4 model_matrix_d;
glm::mat4 model_matrix_ball;
glm::mat4 court_model_matrix;

glm::vec3 peach_pos = glm::vec3(-4.5, 0, 0);
glm::vec3 daisy_pos = glm::vec3(4.5, 0, 0);
glm::vec3 daisy_movement = glm::vec3(0, 0, 0);
glm::vec3 peach_movement = glm::vec3(0, 0, 0);
glm::vec3 player_size = glm::vec3(0.75f, 1.55f, 1.0f);
glm::vec3 ball_pos = glm::vec3(0, 0, 0);
glm::vec3 ball_movement = glm::vec3(0, 0, 0);
glm::vec3 ball_scale = glm::vec3(0.4f, 0.4f, 1.0f);

float peach_speed = 3.5f;
float daisy_speed = 3.5f;
float p_height = 1.0f * player_size.y;
float p_width = 0.5f;
float ball_height = 0.3f;
float ball_width = 0.3f;
float ball_speed = 3.5f;


SDL_Joystick* playerOneController;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0; // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0; // this value MUST be zero

int frame_counter = 0;
bool is_growing = true;


float MILLISECONDS_IN_SECOND = 1000.0f;
float previous_ticks = 0.0f;
float player_speed = 1.0f;



//helper funcs

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    program.SetModelMatrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}


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

bool bounce(glm::vec3 ball_position, glm::vec3 p_pos) {
    float x = fabs(ball_pos.x - p_pos.x) - ((ball_width + p_width) / 2.0f);
    float y = fabs(ball_pos.y - p_pos.y) - ((ball_height + p_height) / 2.0f);
    return (x < 0 && y < 0);
}

bool limit(glm::vec3 position, float height, float top) {
    return !((position.y + (height / 2.0f)) < top);
}

bool floor(glm::vec3 position, float height, float bottom) {
    return !((position.y - (height / 2.0f)) > bottom);
}

void initialise();
void process_input();
void update();
void render();
void shutdown();


//funcs in main
void initialise() {
    // Initialise video and joystick subsystem
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

    // Open the first controller found. Reruen null on error
    playerOneController = SDL_JoystickOpen(0);

    display_window = SDL_CreateWindow("Pong!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(display_window);
    SDL_GL_MakeCurrent(display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT); // Init our camera

    // Load the shaders for handling textures
    program.Load(V_SHADER_PATH, F_SHADER_PATH);

    // Load our player image
    court_texture_id = load_texture(BACKGROUND_SPRITE);
    peach_texture_id = load_texture(PEACH_SPRITE);
    daisy_texture_id = load_texture(DAISY_SPRITE);
    ball_texture_id = load_texture(BALL_SPRITE);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    view_matrix = glm::mat4(1.0f);
    model_matrix_ball = glm::mat4(1.0f);
    court_model_matrix = glm::mat4(1.0f);
    projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    court_model_matrix = glm::scale(court_model_matrix, glm::vec3(10.0f, 10.0f, 5.0f));


    program.SetViewMatrix(view_matrix);
    program.SetProjectionMatrix(projection_matrix);

    glUseProgram(program.programID);

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

}

void process_input() {

    peach_movement = glm::vec3(0);
    daisy_movement = glm::vec3(0);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            game_is_running = false;
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_LEFT:
                break;
            case SDLK_RIGHT:
                break;
            case SDLK_SPACE:
                break;
            }
            break;
        }
    }

    const Uint8* key_states = SDL_GetKeyboardState(NULL);
    if (key_states[SDL_SCANCODE_SPACE]) {
        ball_movement.x = 1.0f;
        ball_movement.y = 1.5f;
    }
    if (glm::length(ball_movement) > 1.0f) {
        ball_movement = glm::normalize(ball_movement);
    }
    if ((key_states[SDL_SCANCODE_UP]) && !limit(daisy_pos, p_height, 3.7f)) {
        daisy_movement.y = 1.0f;
    }
    else if ((key_states[SDL_SCANCODE_DOWN]) && !floor(daisy_pos, p_height, -3.7f)) {
        daisy_movement.y = -1.0f;
    }
    if (glm::length(daisy_movement) > 1.0f) {
        daisy_movement = glm::normalize(daisy_movement);
    }
    if ((key_states[SDL_SCANCODE_W]) && !limit(peach_pos, p_height, 3.7f)) {
        peach_movement.y = 1.0f;
    }
    else if ((key_states[SDL_SCANCODE_S]) && !floor(peach_pos, p_height, -3.7f)) {
        peach_movement.y = -1.0f;
    }
    if (glm::length(peach_movement) > 1.0f) {
        peach_movement = glm::normalize(peach_movement);
    }
    if ((ball_pos.x > 3.75f + 1.0f) || (ball_pos.x < -3.75f - 1.0f)) {
        ball_movement = glm::vec3(0, 0, 0);//freezes the ball in place
        }
}




void update(){
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float delta_time = ticks - previous_ticks;
    previous_ticks = ticks;

    model_matrix_ball = glm::mat4(1.0f);
    model_matrix_p = glm::mat4(1.0f);
    model_matrix_d = glm::mat4(1.0f);
    
    if ((bounce(ball_pos, peach_pos)) || (bounce(ball_pos, daisy_pos))) {
        ball_movement.x *= -1.0f;
    }
    else if ((limit(ball_pos, ball_height, 3.7f) || floor(ball_pos, ball_height, -3.7f))) {
        ball_movement.y *= -1.0f;
    }
    
    ball_pos += ball_movement * ball_speed * delta_time;
    model_matrix_ball = glm::translate(model_matrix_ball, ball_pos);
    model_matrix_ball = glm::scale(model_matrix_ball, ball_scale);

    daisy_pos += daisy_movement * daisy_speed * delta_time;
    model_matrix_d = glm::translate(model_matrix_d, daisy_pos);
    model_matrix_d = glm::scale(model_matrix_d, player_size);

    peach_pos += peach_movement * peach_speed * delta_time;
    model_matrix_p = glm::translate(model_matrix_p, peach_pos);
    model_matrix_p = glm::scale(model_matrix_p, player_size);


}


void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program.texCoordAttribute);

    draw_object(court_model_matrix, court_texture_id);
    draw_object(model_matrix_p, peach_texture_id);
    draw_object(model_matrix_d, daisy_texture_id);
    draw_object(model_matrix_ball, ball_texture_id);

    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

    SDL_GL_SwapWindow(display_window);
}


void shutdown() {
    SDL_Quit();
}


//main
int main(int argc, char* argv[]) {
    initialise();
    while (game_is_running) {
        process_input();
        update();
        render();
    }
    shutdown();
    return 0;
}
