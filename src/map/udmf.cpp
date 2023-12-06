#include "udmf.hpp"
#include <regex>

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
    @brief A macro to iterate over all matches that a certain regex expression produces
    @param regexExp the expression to iterate over
    @param name the name of the expression
*/
#define foreachRegexMatch(regexExp, regexName, hay) \
    std::regex const regexName##Regex(regexExp); \
    auto regexName##Begin = std::sregex_iterator(hay.begin(), hay.end(), regexName##Regex); \
    auto regexName##End = std::sregex_iterator(); \
    std::string regexName##Match;\
    for(std::sregex_iterator i = regexName##Begin; i != regexName##End; i++,regexName##Match = i->str())

/*!
 @brief Gets the first match of a certain property in a block
 @param property the property to get the match of
 @param hay the block to search in
 @return the property value
*/
static std::string getMatch(const char* property, std::string hay){
    size_t propertyLen = strlen(property);
    char* regexStr = (char*)malloc( + 23);
    snprintf(regexStr, propertyLen + 23, "%s[\\s]*?[\\s]*?=[\\s]*?([\\S]*?);", property);
    std::regex const regex(regexStr); 
    std::smatch propertyMatch;
    std::regex_search(hay, propertyMatch, regex);
    free(regexStr);
    return propertyMatch[0].str();
}

udmf::udmf(const char* path){
    char* contents;
    //open the file and read it's contents
    {
        FILE* f = fopen(path, "r");
        if(f == NULL){
            throw "Could not open file";
        }
        fseek(f, 0, SEEK_END);
        unsigned int size = ftell(f);
        fseek(f, 0, SEEK_SET);
        contents = (char*)malloc(size);
        fread(contents, 1, size, f);
        fclose(f);
    }
    //parse the file
    {
        //remove all comments
        for(unsigned int i = 0; i < strlen(contents); i++){
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
        std::string contentsStr(contents);
        foreachRegexMatch(R"([A-Za-z_]+[A-Za-z0-9_]*[\s]*?{([\s\S]*?)})", block, contentsStr){
            const char* typeName = blockMatch.c_str() - 1;
            size_t typeNameLen = 0;
            //find the block type
            while(*typeName == ' '){
                typeName--;
            }
            while(*typeName != ' '){
                typeName--;
                typeNameLen++;
            }
            if(strncmp(typeName, LINEDEF_TYPE, typeNameLen) == 0){
                linedefCount++;
                linedefs = (linedef*)realloc(linedefs, sizeof(linedef) * linedefCount);
                linedef* linedef = linedefs + (linedefCount - 1);
                std::string id = getMatch("id", blockMatch);
                linedef->id = atoi(id.c_str());
                std::string v1 = getMatch("v1", blockMatch);
                linedef->start = atoi(v1.c_str());
                std::string v2 = getMatch("v2", blockMatch);
                linedef->end = atoi(v2.c_str());
                std::string front = getMatch("sidefront", blockMatch);
                linedef->sidefront = atoi(front.c_str());
                std::string back = getMatch("sideback", blockMatch);
                linedef->sideback = atoi(back.c_str());
            }
            else if(strncmp(typeName, VERTEX_TYPE, typeNameLen) == 0){
                vertexCount++;
                vertices = (vertex*)realloc(vertices, sizeof(vertex) * vertexCount);
                vertex* vertex = vertices + (vertexCount - 1);
                std::string x = getMatch("x", blockMatch);
                vertex->x = atoi(x.c_str());
                std::string y = getMatch("y", blockMatch);
                vertex->y = atoi(y.c_str());
            }
            else if(strncmp(typeName, SIDEDEF_TYPE, typeNameLen) == 0){
                sidedefCount++;
                sidedefs = (sidedef*)realloc(sidedefs, sizeof(sidedef) * sidedefCount);
                sidedef* sidedef = sidedefs + (sidedefCount - 1);
                std::string id = getMatch("id", blockMatch);
                sidedef->id = atoi(id.c_str());
                std::string offsetx = getMatch("offsetx", blockMatch);
                sidedef->offsetx = atoi(offsetx.c_str());
                std::string offsety = getMatch("offsety", blockMatch);
                sidedef->offsety = atoi(offsety.c_str());
                std::string sector = getMatch("sector", blockMatch);
                sidedef->sector = atoi(sector.c_str());
            }
            else if(strncmp(typeName, SECTOR_TYPE, typeNameLen) == 0){
                sectorCount++;
                sectors = (mapSector*)realloc(sectors, sizeof(mapSector) * sectorCount);
                mapSector* sector = sectors + (sectorCount - 1);
                std::string heightfloor = getMatch("heightfloor", blockMatch);
                sector->heightfloor = atoi(heightfloor.c_str());
                std::string heightceiling = getMatch("heightceiling", blockMatch);
                sector->heightceiling = atoi(heightceiling.c_str());
            }
            else if(strncmp(typeName, THING_TYPE, typeNameLen) == 0){
                thingCount++;
                things = (thing*)realloc(things, sizeof(thing) * thingCount);
                thing* thing = things + (thingCount - 1);
                std::string id = getMatch("id", blockMatch);
                thing->id = atoi(id.c_str());
                std::string x = getMatch("x", blockMatch);
                thing->x = atof(x.c_str());
                std::string y = getMatch("y", blockMatch);
                thing->y = atof(y.c_str());
                std::string height = getMatch("height", blockMatch);
                thing->height = atof(height.c_str());
                std::string angle = getMatch("angle", blockMatch);
                thing->angle = atoi(angle.c_str());
                std::string type = getMatch("type", blockMatch);
                thing->type = atoi(type.c_str());
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
        length = 0; //max Y value of vectors
        width = 0; //max X value
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
        height = 0;
        //get height from sectors
        for(size_t i = 0; i < sectorCount; i++){
            mapSector* sector = sectors + i;
            sector->heightceiling = (sector->heightceiling + heightVector) / SCALE;
            if(sector->heightceiling > height){
                height = sector->heightceiling;
            }
        }
        //allocate blocks
        blocks = (unsigned int***)malloc(width * sizeof(unsigned int**));
        for(unsigned int i = 0; i < width; i++){
            blocks[i] = (unsigned int**)malloc(height * sizeof(unsigned int*));
            for(unsigned int j = 0; j < height; j++){
                blocks[i][j] = (unsigned int*)calloc(length, sizeof(unsigned int));
            }
        }
        //allocate palette
        palette = (unsigned int*)calloc(2, sizeof(unsigned int));
        palette[1] = 1;
        //foreach linedef
        for(size_t i = 0; i < linedefCount; i++){
            const linedef* linedef = linedefs + i;
            vertex* start = vertices + linedef->start;
            //vertex* end = vertices + linedef->end;

            //get the front sector
            mapSector* front = sectors + (sidedefs + linedef->sidefront)->sector;
            blocks[start->x][front->heightfloor][start->y] = 1;
            if(front->heightceiling > front->heightfloor){
                blocks[start->x][front->heightceiling][start->y] = 1;
            }
            //get the back sector
            if(linedef->sideback < 0){
                mapSector* end = sectors + (sidedefs + linedef->sideback)->sector;
                blocks[start->x][end->heightfloor][start->y] = 1;
                if(end->heightceiling > end->heightfloor){
                    blocks[start->x][end->heightceiling][start->y] = 1;
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

