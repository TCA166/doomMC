#include "mcr.hpp"

extern "C" {
    #include <unistd.h>
    #include <stdlib.h>
    #include "../C/regionParser.h"
    #include "../../cNBT/nbt.h"
}

minecraftRegion::minecraftRegion(const char* path){
    this->height = 256;
    this->width = 512;
    this->length = 512;
    this->spawnCount = 1;
    this->spawns = (position*)malloc(sizeof(position));
    this->spawns[0] = toPosition(160, 160, 160);
    FILE* f = fopen(path, "rb");
    if(f == NULL){
        throw "Could not open file";
    }
    chunk* rawChunks = getChunks(f);
    fclose(f);
    for(int i = 0; i < chunkN; i++){
        if(!chunkIsNull(rawChunks[i])){
            nbt_node* node = nbt_parse_compressed(rawChunks[i].data, rawChunks[i].byteLength);
            if(node == NULL){
                throw "Could not parse chunk";
            }
            if(node->type != TAG_COMPOUND){
                throw "Chunk is not a compound";
            }
            nbt_node* sectionsNode = nbt_find_by_name(node, "sections");
            if(sectionsNode == NULL){
                throw "Could not find sections node";
            }
            //TODO continue parsing
            struct nbt_list* sectionsList = sectionsNode->payload.tag_list;
            struct list_head* pos;
            int n = 0;
            //foreach section
            list_for_each(pos, &sectionsList->entry){

            }
        }
    }
}   

minecraftRegion::~minecraftRegion(){
    //TODO
}
