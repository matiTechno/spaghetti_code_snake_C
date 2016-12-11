#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

struct World;

typedef struct
{
    int x;
    int y;
}
vec2i;

typedef struct
{
    vec2i position;
    vec2i size;
}
Entity;

typedef struct SnakeNode
{
    Entity entity;
    vec2i moveDir;
    struct SnakeNode* next;
    int isNew;
    int counter;
}
SnakeNode;

typedef struct
{
    Entity entity;
    double life; // in seconds
    void (*spawn)(struct World* world, double life);
}
Food;

typedef struct
{
    vec2i winSize;
    int gridSize;
    double food_life_on_reset;

    Entity walls[4];
    SnakeNode* head;
    Food food;
    int GAMEOVER;
}
World;

// main functions
int initAllegro();
World* initWorld(vec2i winSize, int gridSize);
void processInput(ALLEGRO_EVENT_QUEUE* eventQueue, World* world, int* quit);
void update(World* world, double dt);
void render(World* world);
void deleteWorld(World* world);
// other
int isCollision(Entity *e1, Entity *e2);
void move(SnakeNode* head, int gridSize);
void spawn(World* world, double life);
void drawEntity(Entity* entity, ALLEGRO_COLOR color);
void resetSnakeNodes(SnakeNode* head);

int moveFlag = 0;

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    if(!initAllegro())
        return -1;

    srand(time(NULL));

    //##### HOLY TRINITY OF CONFIGURATION ######
    //##### also modify initWorld function for specific things
    const vec2i winSize = {400, 400};
    const int gridSize = 20;
    // in seconds
    const double dt = 0.2;
    //##########################################

    assert(winSize.x % 2 == 0);
    assert(winSize.y % 2 == 0);
    assert(gridSize % 2 == 0);
    assert(winSize.x % gridSize == 0);
    assert(winSize.y % gridSize == 0);

    ALLEGRO_DISPLAY* window = al_create_display(winSize.x, winSize.y);

    ALLEGRO_EVENT_QUEUE* eventQueue = al_create_event_queue();
    al_register_event_source(eventQueue, al_get_display_event_source(window));
    al_register_event_source(eventQueue, al_get_keyboard_event_source());

    World* world = initWorld(winSize, gridSize);

    int quit = false;

    double currentTime = al_get_time();
    double accumulator = 0.0;

    while(!quit)
    {
        processInput(eventQueue, world, &quit);

        double newTime = al_get_time();
        double frameTime = newTime - currentTime;
        accumulator += frameTime;
        currentTime = newTime;

        while(accumulator >= dt)
        {
            if(world->GAMEOVER == 0)
                update(world, dt);
            accumulator -= dt;
        }

        render(world);
    }

    deleteWorld(world);
    al_destroy_event_queue(eventQueue);
    al_destroy_display(window);
    return 0;
}

int initAllegro()
{
    if(!(al_init() && al_install_keyboard() && al_init_primitives_addon()))
        return 0;
    al_init_font_addon();
    return 1;
}

World* initWorld(vec2i winSize, int gridSize)
{
    World* world = malloc(sizeof(World));

    world->winSize = winSize;
    world->gridSize = gridSize;
    world->food_life_on_reset = 60.0;

    world->walls[0] = (Entity){{0, 0}, {gridSize, winSize.y}};
    world->walls[1] = (Entity){{winSize.x - gridSize, 0}, {gridSize, winSize.y}};
    world->walls[2] = (Entity){{0, 0}, {winSize.x, gridSize}};
    world->walls[3] = (Entity){{0, winSize.y - gridSize}, {winSize.x, gridSize}};

    world->head = malloc(sizeof(SnakeNode));
    world->head->entity = (Entity){{winSize.x/2, winSize.y/2}, {gridSize, gridSize}};
    world->head->moveDir = (vec2i){0, -1};
    world->head->isNew = 0;
    world->head->next = 0;

    world->food.spawn = &spawn;
    world->food.entity.size = (vec2i){gridSize, gridSize};
    world->food.spawn(world, world->food_life_on_reset);
    world->GAMEOVER = 0;

    return world;
}

void processInput(ALLEGRO_EVENT_QUEUE* eventQueue, World* world, int* quit)
{
    ALLEGRO_EVENT event;
    while(al_get_next_event(eventQueue, &event))
    {
        if(event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            *quit = true;

        if(!world->GAMEOVER)
        {
            if(event.type == ALLEGRO_EVENT_KEY_DOWN && !moveFlag)
            {
                switch(event.keyboard.keycode)
                {
                case ALLEGRO_KEY_W:
                    if(world->head->moveDir.y !=0)
                        break;
                        world->head->moveDir.y = -1;
                    world->head->moveDir.x = 0;
                    moveFlag = 1;
                    return;

                case ALLEGRO_KEY_S:
                    if(world->head->moveDir.y != 0)
                        break;;
                        world->head->moveDir.y = 1;
                    world->head->moveDir.x = 0;
                    moveFlag = 1;
                    return;

                case ALLEGRO_KEY_A:
                    if(world->head->moveDir.x != 0)
                        break;
                        world->head->moveDir.x = -1;
                    world->head->moveDir.y = 0;
                    moveFlag = 1;
                    return;

                case ALLEGRO_KEY_D:
                    if(world->head->moveDir.x != 0)
                        break;
                        world->head->moveDir.x = 1;
                    world->head->moveDir.y = 0;
                    moveFlag = 1;
                    return;

                case ALLEGRO_KEY_ESCAPE: *quit = true; return;
                default:;
                }
            }
        }
        else
        {
            if(event.type == ALLEGRO_EVENT_KEY_DOWN)
            {
                switch(event.keyboard.keycode)
                {
                case ALLEGRO_KEY_R: world->GAMEOVER = 0;
                    resetSnakeNodes(world->head);
                    world->head->entity.position = (vec2i){world->winSize.x/2, world->winSize.x/2};
                    world->head->moveDir = (vec2i){0, -1};
                    return;
                case ALLEGRO_KEY_ESCAPE: *quit = true; return;
                default:;
                }
            }
        }
    }
}

void update(World* world, double dt)
{
    moveFlag = 0;
    move(world->head, world->gridSize);
    world->food.life -= dt;

    if(world->food.life <= 0)
        world->food.spawn(world, world->food_life_on_reset);

    if(isCollision(&world->head->entity, &world->food.entity))
    {
        SnakeNode* it = world->head;
        int counter = 1;
        while(it->next != 0)
        {
            ++counter;
            it = it->next;
        }
        it->next = malloc(sizeof(SnakeNode));
        it->next->next = 0;
        it->next->counter = counter;
        it->next->isNew = 1;
        it->next->entity = world->food.entity;
        it->next->moveDir = (vec2i){0, 0};

        world->food.spawn(world, world->food_life_on_reset);
    }

    for(unsigned int i = 0; i < 4; ++i)
    {
        if(isCollision(&world->head->entity, &world->walls[i]))
        {
            world->GAMEOVER = 1;
            return;
        }
    }

    SnakeNode* it = world->head->next;
    while(it != 0)
    {
        if(it->isNew == 0)
        {
            if(isCollision(&it->entity, &world->head->entity))
            {
                world->GAMEOVER = 1;
                return;
            }
        }
        it = it->next;
    }
}

void render(World* world)
{
    typedef ALLEGRO_COLOR COL;

    al_clear_to_color((COL){0.f, 0.f, 0.f, 1.f});

    for(unsigned i = 0; i < 4; ++i)
        drawEntity(&world->walls[i], (COL){1.f, 0.f, 0.f, 1.f});

    SnakeNode* it = world->head;
    while(it != 0)
    {
        COL col;
        if(it->isNew == 1)
            col = (COL){0.5f, 0.f, 0.5f, 0.5f};
        else if(it == world->head)
            col = (COL){1.f, 0.5f, 0.f, 0.5f};
        else
            col = (COL){0.f, 1.f, 0.f, 0.5f};

        drawEntity(&it->entity, col);
        it = it->next;
    }

    drawEntity(&world->food.entity, (COL){0.5f, 0.f,0.5f, 1.f});

    if(world->GAMEOVER)
    {
        ALLEGRO_FONT* font = al_create_builtin_font();
        al_draw_text(font, (COL){1.f, 0.f, 0.f, 1.f}, world->winSize.x/2, world->winSize.y/2, ALLEGRO_ALIGN_CENTER,
                     "GAME OVER, press R to restart, ESC to exit");

        char SCORE[20] = "your score: ";
        int x = 0;
        SnakeNode* it = world->head->next;
        while(it)
        {
            ++x;
            it = it->next;
        }

        sprintf(SCORE + 12, "%d", x);

        al_draw_text(font, (COL){1.f, 0.f, 0.f, 1.f}, world->winSize.x/2, world->winSize.y/2 + 50, ALLEGRO_ALIGN_CENTER,
                     SCORE);

        al_destroy_font(font);
    }

    al_flip_display();
}

void deleteWorld(World* world)
{
    SnakeNode* it = world->head;
    while(it != 0)
    {
        SnakeNode* temp = it->next;
        free(it);
        it = temp;
    }
    free(world);
}

int isCollision(Entity *e1, Entity *e2)
{
    int x = e1->position.x < e2->position.x + e2->size.x
            && e2->position.x < e1->position.x + e1->size.x;

    int y = e1->position.y < e2->position.y + e2->size.y
            && e2->position.y < e1->position.y + e1->size.y;

    return x && y;
}

void move(SnakeNode* head, int gridSize)
{
    int counter = 0;
    SnakeNode* it = head;
    while(it != 0)
    {
        if(it->isNew)
        {
            --it->counter;
            if(it->counter <= 0)
            {
                it->isNew = 0;
            }
        }
        else
        {
            it->entity.position.x += it->moveDir.x *gridSize;
            it->entity.position.y += it->moveDir.y *gridSize;
        }
        it = it->next;
        ++counter;
    }
    int counter2 = counter - 2;
    for(int i = 0; i < counter - 1; ++i)
    {
        SnakeNode* it = head->next;
        SnakeNode* prevIt = head;
        for(int k = 0; k < counter2; k++)
        {
            prevIt= it;
            it = it->next;
        }
        --counter2;
        it->moveDir = prevIt->moveDir;
    }
}

void spawn(World* world, double life)
{
    world->food.life = life;
    int gridsX = world->winSize.x / world->gridSize;
    int gridsY = world->winSize.y / world->gridSize;

    while(true)
    {
        vec2i newPos = {(rand() % gridsX)*world->gridSize,
                        (rand() % gridsY)*world->gridSize};
        world->food.entity.position = newPos;

        int flag1 = 0;
        for(unsigned i = 0; i < 4; ++i)
            if(isCollision(&world->walls[i], &world->food.entity))
                flag1 = 1;

        int flag2 = 0;
        SnakeNode* it = world->head;
        while(it != 0)
        {
            if(isCollision(&it->entity, &world->food.entity))
            {
                flag2 = true;
                break;
            }
            it = it->next;
        }

        if(!flag2 && !flag1)
            return;
    }
}

void drawEntity(Entity* entity, ALLEGRO_COLOR color)
{
    al_draw_filled_rectangle(entity->position.x + 1, entity->position.y + 1,
                             entity->position.x + entity->size.x - 1, entity->position.y + entity->size.y - 1,
                             color);
}

void resetSnakeNodes(SnakeNode* head)
{
    SnakeNode* it = head->next;
    head->next = 0;
    while(it != 0)
    {
        SnakeNode* temp = it->next;
        free(it);
        it = temp;
    }
}
