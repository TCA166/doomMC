#include "udmf.hpp"
#include <regex>

extern "C"{
    #include <stdio.h>
}

//64 units = 1 block

typedef struct{
    unsigned int x;
    unsigned int y;
} vertex;

typedef struct{
    unsigned int heightfloor;
    unsigned int heightceiling;
} mapSector;

typedef struct{
    unsigned int id;
    int offsetx;
    int offsety;
    mapSector* sector;
} sidedef;

typedef struct{
    unsigned int id;
    vertex* start;
    vertex* end;
    sidedef* sidefront; 
    sidedef* sideback; //may be NULL
} linedef;

#define REGEX_BLOCK(blockType) #blockType "[\\s]*?{([\\s\\S]*?)}"

/*!
    @brief A macro to iterate over all the blocks of a certain type in a UDMF file. Provides a match variable as the match string
    @param blockType The name of the block type
*/
#define foreachBlock(blockType) \
    std::regex const blockType##Regex(REGEX_BLOCK(blockType)); \
    auto blockType## Begin = std::sregex_iterator(contentsStr.begin(), contentsStr.end(), ## blockType ## Regex); \
    auto blockType## End = std::sregex_iterator(); \
    std::string match;\
    for(std::sregex_iterator i = ## blockType ## Begin; i != ## blockType ## End; i++,match = i->str())

#define getMatch(property, hay) \
    std::regex const property##Regex(#property "[\\s]*?=[\\s]*?([\\S]*?);"); \
    std::smatch property##Match; \
    std::regex_search(hay, property##Match, property##Regex); \
    std::string property##Str = property##Match[0].str(); \

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
        //find all the blocks
        std::string contentsStr(contents);
        vertex* vertices = NULL;
        size_t vertexCount = 0;
        foreachBlock(vertex){
            vertexCount++;
            vertices = (vertex*)realloc(vertices, sizeof(vertex) * vertexCount);
            getMatch(x, match);
            vertices[vertexCount - 1].x = atoi(xStr.c_str());
            getMatch(y, match);
            vertices[vertexCount - 1].y = atoi(yStr.c_str());
        }
        //TODO continue parsing

    }
}

