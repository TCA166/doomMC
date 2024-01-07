#include "udmf.hpp"
#include <fstream>
#include <streambuf>
#include <spdlog/spdlog.h>

extern "C"{
    #include <stdio.h>
}

//https://zdoom.org/wiki/Standard_editor_numbers

#define PLAYER_1_SPAWN 1
#define PLAYER_2_SPAWN 2
#define PLAYER_3_SPAWN 3
#define PLAYER_4_SPAWN 4
#define DEATHMATCH_START 11

/*
Doom maps are made up of sectors.
Sectors are separated by linedefs.
Linedefs define sector sides (sidedefs) which point to sectors.
Linedefs are defined by x and y coordinates stored in vertices.
Confusingly enough because DOOM is 2,5 D the Z axis is the Y axis in the map.
Things are essentially entities.
*/

typedef struct{
    int x;
    int y; //in 3d z axis
} vertex;

typedef struct{
    int heightfloor;
    int heightceiling;
} mapSector;

typedef struct{
    unsigned int id;
    int offsetx;
    int offsety;
    unsigned int sector; //sector pointer
} sidedef;

typedef struct{
    unsigned int id;
    unsigned int start; //vertex pointer
    unsigned int end; //vertex pointer
    unsigned int sidefront; //sidedef pointer
    int sideback; //sidedef pointer that may be NULL
} linedef;

typedef struct{
    unsigned int id;
    float x;
    float y;
    float height;
    unsigned int angle;
    unsigned int type;
} thing;

#define LINEDEF_TYPE "linedef"
#define SIDEDEF_TYPE "sidedef"
#define VERTEX_TYPE "vertex"
#define SECTOR_TYPE "sector"
#define THING_TYPE "thing"

//64 units = 1 block
#define SCALE 64

void addVertexVal(vertex* vertex, const char* key, const char* val){
    if(strncmp(key, "x", 1) == 0){
        vertex->x = (int)atof(val);
    }
    else if(strncmp(key, "y", 1) == 0){
        vertex->y = (int)atof(val);
    }
}

void addSidedefVal(sidedef* sidedef, const char* key, const char* val){
    if(strncmp(key, "id", 2) == 0){
        sidedef->id = atoi(val);
    }
    else if(strncmp(key, "offsetx", 7) == 0){
        sidedef->offsetx = atoi(val);
    }
    else if(strncmp(key, "offsety", 7) == 0){
        sidedef->offsety = atoi(val);
    }
    else if(strncmp(key, "sector", 6) == 0){
        sidedef->sector = atoi(val);
    }
}

void addSectorVal(mapSector* sector, const char* key, const char* val){
    if(strncmp(key, "heightfloor", 11) == 0){
        sector->heightfloor = atoi(val);
    }
    else if(strncmp(key, "heightceiling", 13) == 0){
        sector->heightceiling = atoi(val);
    }
}

void addLinedefVal(linedef* linedef, const char* key, const char* val){
    if(strncmp(key, "id", 2) == 0){
        linedef->id = atoi(val);
    }
    else if(strncmp(key, "v1", 2) == 0){
        linedef->start = atoi(val);
    }
    else if(strncmp(key, "v2", 2) == 0){
        linedef->end = atoi(val);
    }
    else if(strncmp(key, "sidefront", 9) == 0){
        linedef->sidefront = atoi(val);
    }
    else if(strncmp(key, "sideback", 8) == 0){
        linedef->sideback = atoi(val);
    }
}

void addThingVal(thing* thing, const char* key, const char* val){
    if(strncmp(key, "id", 2) == 0){
        thing->id = atoi(val);
    }
    else if(strncmp(key, "x", 1) == 0){
        thing->x = atof(val);
    }
    else if(strncmp(key, "y", 1) == 0){
        thing->y = atof(val);
    }
    else if(strncmp(key, "height", 6) == 0){
        thing->height = atof(val);
    }
    else if(strncmp(key, "angle", 5) == 0){
        thing->angle = atoi(val);
    }
    else if(strncmp(key, "type", 4) == 0){
        thing->type = atoi(val);
    }
}

typedef void (*addVal)(void*, const char*, const char*);

#define clearWhitespace(ptr) \
    while(*ptr == ' '){ptr++;} \
    char* space = strchr(key, ' '); \
    if(space != NULL){*space = '\0';}

/*!
 @brief Parses a UDMF struct block into a structure
 @param structure the structure to parse into
 @param block the block to parse
 @param func the function to call to add a value to the structure
*/
void parseBlock(void* structure, char* block, addVal func){
    char* ptr = block;
    char* key = NULL;
    while(*ptr != '\0'){ //foreach character
        if(*ptr == '='){
            *ptr = '\0';
            key = block;
            clearWhitespace(key)
            block = ptr + 1;
        }
        else if(*ptr == ';'){
            *ptr = '\0';
            clearWhitespace(block)
            func(structure, key, block);
            block = ptr + 1;
        }
        ptr++;
    }
}

udmf::udmf(const char* path){
    /*
    We going really old school here.
    Regex wasn't fast enough I think, so strtok_r it is.
    */
    size_t contentsSize;
    char* contents;
    //open the file and read its contents
    {
        FILE* f = fopen(path, "r");
        if(f == NULL){
            throw "Could not open file";
        }
        fseek(f, 0, SEEK_END);
        contentsSize = ftell(f);
        fseek(f, 0, SEEK_SET);
        contents = (char*)malloc(contentsSize + 1);
        fread(contents, 1, contentsSize, f);
        contents[contentsSize] = '\0';
        fclose(f);
    }
    //parse the file
    {
        //remove all comments
        for(unsigned int i = 0; i < contentsSize; i++){
            if(contents[i] == '/'){
                if(contents[i + 1] == '/'){
                    while(contents[i] != '\n'){
                        contents[i] = ' ';
                        i++;
                    }
                    contents[i] = ' ';
                }
                else if(contents[i + 1] == '*'){
                    while(contents[i] != '*' && contents[i + 1] != '/'){
                        contents[i] = ' ';
                        i++;
                    }
                }
            }
            else if(contents[i] == '\n'){
                contents[i] = ' ';
            }
        }
        linedef* linedefs = NULL;
        size_t linedefCount = 0;

        vertex* vertices = NULL;
        size_t vertexCount = 0;

        sidedef* sidedefs = NULL;
        size_t sidedefCount = 0;

        mapSector* sectors = NULL;
        size_t sectorCount = 0;

        thing* things = NULL;
        size_t thingCount = 0;
        //find all the blocks
        char* savePtr;
        char* block = strtok_r(contents, "}", &savePtr);
        while(block != NULL){
            //find the type of block
            char* start = strchr(block, '{');
            if(start != NULL){
                start--;
                block = start + 2;
                //move back until we find the type
                while(*start == ' '){
                    start--;
                }
                *(start + 1) = '\0';
                size_t len = 1;
                //move back until we find the start of the type
                while(*(start - 1) != ' '){
                    len++;
                    start--;
                }
                if(strncmp(start, LINEDEF_TYPE, len) == 0){
                    linedefCount++;
                    linedefs = (linedef*)realloc(linedefs, linedefCount * sizeof(linedef));
                    linedef* ln = linedefs + linedefCount - 1;
                    memset(ln, 0, sizeof(linedef));
                    parseBlock(ln, block, (addVal)addLinedefVal);
                }
                else if(strncmp(start, VERTEX_TYPE, len) == 0){
                    vertexCount++;
                    vertices = (vertex*)realloc(vertices, vertexCount * sizeof(vertex));
                    vertex* v = vertices + vertexCount - 1;
                    memset(v, 0, sizeof(vertex));
                    parseBlock(v, block, (addVal)addVertexVal);
                }
                else if(strncmp(start, SIDEDEF_TYPE, len) == 0){
                    sidedefCount++;
                    sidedefs = (sidedef*)realloc(sidedefs, sidedefCount * sizeof(sidedef));
                    sidedef* sid = sidedefs + sidedefCount - 1;
                    memset(sid, 0, sizeof(sidedef));
                    parseBlock(sid, block, (addVal)addSidedefVal);
                }
                else if(strncmp(start, SECTOR_TYPE, len) == 0){
                    sectorCount++;
                    sectors = (mapSector*)realloc(sectors, sectorCount * sizeof(mapSector));
                    mapSector* sector = sectors + sectorCount - 1;
                    memset(sector, 0, sizeof(mapSector));
                    parseBlock(sector, block, (addVal)addSectorVal);
                    //TODO sectors can have ceiling lower than floor????
                    if(sector->heightceiling < sector->heightfloor){ //swap for now
                        int cont = sector->heightceiling;
                        sector->heightceiling = sector->heightfloor;
                        sector->heightfloor = cont;
                    }
                }
                else if(strncmp(start, THING_TYPE, len) == 0){
                    thingCount++;
                    things = (thing*)realloc(things, thingCount * sizeof(thing));
                    thing* th = things + thingCount - 1;
                    memset(th, 0, sizeof(thing));
                    parseBlock(th, block, (addVal)addThingVal);
                }
            }
            block = strtok_r(NULL, "}", &savePtr);
        }
        free(contents);
        //find the lowest x and y values
        int minX = 0;
        int minY = 0;
        for(size_t i = 0; i < vertexCount; i++){
            vertex* vertex = vertices + i;
            if(vertex->x < minX){
                minX = vertex->x;
            }
            if(vertex->y < minY){
                minY = vertex->y;
            }
        }
        //then based on these values determine if we need to move the vertices
        unsigned int xVector = 0;
        unsigned int yVector = 0;
        if(minX < 0){
            xVector = -minX;
        }
        if(minY < 0){
            yVector = -minY;
        }
        this->length = 0; //max Y value of vectors
        this->width = 0; //max X value
        //move the vertices to the grid
        for(size_t i = 0; i < vertexCount; i++){
            vertex* vertex = vertices + i;
            vertex->x = (vertex->x + xVector) / SCALE;
            if((unsigned int)vertex->x > width){
                width = vertex->x;
            }
            vertex->y = (vertex->y + yVector) / SCALE;
            if((unsigned int)vertex->y > length){
                length = vertex->y;
            }
        }
        //get min height from sectors
        int minHeight = 0;
        for(size_t i = 0; i < sectorCount; i++){
            mapSector* sector = sectors + i;
            if(sector->heightfloor < minHeight){
                minHeight = sector->heightfloor;
            }
        }
        unsigned int heightVector = 0;
        if(minHeight < 0){
            heightVector = -minHeight;
        }
        this->height = 0;
        //get height from sectors
        for(size_t i = 0; i < sectorCount; i++){
            mapSector* sector = sectors + i;
            sector->heightceiling = (sector->heightceiling + heightVector) / SCALE;
            sector->heightfloor = (sector->heightfloor + heightVector) / SCALE;
            if((unsigned int)sector->heightceiling > height){
                height = sector->heightceiling;
            }
        }
        //printf("width: %d, length: %d, height: %d\n", width, length, height);
        //allocate blocks
        width++;
        length++;
        height++;
        this->blocks = (int32_t***)malloc(width * sizeof(int32_t**));
        for(unsigned int i = 0; i < width; i++){
            this->blocks[i] = (int32_t**)malloc(height * sizeof(int32_t*));
            for(unsigned int j = 0; j < height; j++){
                this->blocks[i][j] = (int32_t*)calloc(length, sizeof(int32_t));
            }
        }
        //allocate palette
        this->palette = (int32_t*)calloc(2, sizeof(int32_t));
        this->paletteSize = 2;
        this->palette[1] = 1;
        //foreach linedef
        for(size_t i = 0; i < linedefCount; i++){
            const linedef* linedef = linedefs + i;
            vertex* start = vertices + linedef->start; //line start
            vertex* end = vertices + linedef->end; //line end
            //get the front sector
            mapSector* front = sectors + (sidedefs + linedef->sidefront)->sector; //front sector
            this->blocks[start->x][front->heightfloor][start->y] = 1;
            int wallStart = front->heightfloor; //y index of the lowest row of blocks of the wall
            int wallEnd = front->heightceiling; //y index of the highest row of blocks of the wall
            if(front->heightceiling != front->heightfloor){    
                this->blocks[start->x][front->heightceiling][start->y] = 1;
            }
            //get the back sector
            if(linedef->sideback >= 0){
                mapSector* end = sectors + (sidedefs + linedef->sideback)->sector;
                //update wall info if possible
                if(end->heightfloor < wallStart){
                    wallStart = end->heightfloor;
                }
                if(end->heightceiling > wallEnd){
                    wallEnd = end->heightceiling;
                }
                
                this->blocks[start->x][end->heightfloor][start->y] = 1;
                if(end->heightceiling != end->heightfloor){
                    this->blocks[start->x][end->heightceiling][start->y] = 1;
                }
            }
            //https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
            int dx = abs(end->x - start->x);
            int dy = abs(end->y - start->y);
            int D = 2 * dy - dx;
            int y = start->y;
            for(int x = start->x; x <= end->x; x++){
                for(int r = wallStart; r <= wallEnd; r++){
                    this->blocks[x][r][y] = 1;
                }
                if(D > 0){
                    y++;
                    D = D - 2 * dx;
                }
                D = D + 2 * dy;
            }
        }
        free(linedefs);
        free(vertices);
        free(sidedefs);
        free(sectors);
        this->spawns = NULL;
        this->spawnCount = 0;
        //foreach thing
        for(size_t i = 0; i < thingCount; i++){
            thing* thing = things + i;
            if(thing->type == PLAYER_1_SPAWN || thing->type == DEATHMATCH_START){
                this->spawnCount++;
                this->spawns = (position*)realloc(this->spawns, this->spawnCount * sizeof(position));
                position* spawn = this->spawns + this->spawnCount - 1;
                *spawn = toPosition((thing->x + xVector) / SCALE, (thing->height + heightVector) / SCALE, (thing->y + yVector) / SCALE);
            }
        }
        //TODO handle things
        free(things);
        spdlog::info("Loaded udmf {} with dimensions {}x{}x{}", path, this->width, this->height, this->length);
    }
}

udmf::~udmf(){
    
}
