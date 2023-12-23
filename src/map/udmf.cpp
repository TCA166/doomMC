#include "udmf.hpp"
#include <regex>
#include <fstream>
#include <streambuf>

extern "C"{
    #include <stdio.h>
}

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

/*!
 @brief Gets the first match of a certain property in a block
 @param property the property to get the match of
 @param hay the block to search in
 @return the property value
*/
static std::string getMatch(const char* property, size_t propertyLen, std::string hay){
    char* regexStr = (char*)malloc(propertyLen + 30);
    snprintf(regexStr, propertyLen + 30, "%s[\\s]*?[\\s]*?=[\\s]*?([\\S]*?);", property);
    std::regex const regex(regexStr); 
    std::smatch propertyMatch;
    std::regex_search(hay, propertyMatch, regex);
    free(regexStr);
    return propertyMatch[0].str();
}

udmf::udmf(const char* path){
    std::string contents;
    //open the file and read its contents
    {
        std::ifstream t(path);
        std::stringstream buffer;
        buffer << t.rdbuf();
        contents = buffer.str();
    }
    //parse the file
    {
        size_t contentsSz = contents.length();
        //remove all comments
        for(unsigned int i = 0; i < contentsSz; i++){
            if(contents[i] == '/'){
                if(contents[i + 1] == '/'){
                    while(contents[i] != '\n'){
                        contents[i] = ' ';
                        i++;
                    }
                }
                else if(contents[i + 1] == '*'){
                    while(contents[i] != '*' && contents[i + 1] != '/'){
                        contents[i] = ' ';
                        i++;
                    }
                }
            }
        }
        //std::regex const blockRegex(R"([A-Za-z_]+[A-Za-z0-9_]*[\s]*?{([\s\S]*?)})");
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
        std::regex const blockRegex("([A-Za-z_]+[A-Za-z0-9_])*[\\s]*?\\{([\\s\\S]*?)\\}");
        auto blockBegin = std::sregex_iterator(contents.begin(), contents.end(), blockRegex);
        auto blockEnd = std::sregex_iterator();
        for(std::sregex_iterator i = blockBegin; i != blockEnd; i++){
            std::string type = i->str(1);
            size_t typeNameLen = type.length();
            if(typeNameLen == 0){
                continue;
            }
            std::string blockMatch = i->str(2);
            const char* typeName = type.c_str();
            if(strncmp(typeName, LINEDEF_TYPE, typeNameLen) == 0){
                linedefCount++;
                linedefs = (linedef*)realloc(linedefs, sizeof(linedef) * linedefCount);
                linedef* linedef = linedefs + (linedefCount - 1);
            }
            else if(strncmp(typeName, VERTEX_TYPE, typeNameLen) == 0){
                vertexCount++;
                vertices = (vertex*)realloc(vertices, sizeof(vertex) * vertexCount);
                vertex* vertex = vertices + (vertexCount - 1);
            }
            else if(strncmp(typeName, SIDEDEF_TYPE, typeNameLen) == 0){
                sidedefCount++;
                sidedefs = (sidedef*)realloc(sidedefs, sizeof(sidedef) * sidedefCount);
                sidedef* sidedef = sidedefs + (sidedefCount - 1);
            }
            else if(strncmp(typeName, SECTOR_TYPE, typeNameLen) == 0){
                sectorCount++;
                sectors = (mapSector*)realloc(sectors, sizeof(mapSector) * sectorCount);
                mapSector* sector = sectors + (sectorCount - 1);
            }
            else if(strncmp(typeName, THING_TYPE, typeNameLen) == 0){
                thingCount++;
                things = (thing*)realloc(things, sizeof(thing) * thingCount);
                thing* thing = things + (thingCount - 1);
            }
        }
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
            if(vertex->x > width){
                width = vertex->x;
            }
            vertex->y = (vertex->y + yVector) / SCALE;
            if(vertex->y > length){
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
            if(sector->heightceiling > height){
                height = sector->heightceiling;
            }
        }
        //allocate blocks
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
            vertex* start = vertices + linedef->start;
            //vertex* end = vertices + linedef->end;

            //get the front sector
            mapSector* front = sectors + (sidedefs + linedef->sidefront)->sector;
            this->blocks[start->x][front->heightfloor][start->y] = 1;
            if(front->heightceiling > front->heightfloor){
                this->blocks[start->x][front->heightceiling][start->y] = 1;
            }
            //get the back sector
            if(linedef->sideback < 0){
                mapSector* end = sectors + (sidedefs + linedef->sideback)->sector;
                this->blocks[start->x][end->heightfloor][start->y] = 1;
                if(end->heightceiling > end->heightfloor){
                    this->blocks[start->x][end->heightceiling][start->y] = 1;
                }
            }
        }
        free(linedefs);
        free(vertices);
        free(sidedefs);
        free(sectors);
        //TODO handle things
        free(things);
    }
}

udmf::~udmf(){
    free(this->blocks);
    free(this->palette);
}
