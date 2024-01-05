#include "regionParser.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define createMask(startBit, X) ((((uint64_t)1) << X) - 1) << startBit

int handleFirstSegment(chunk* output, FILE* regionFile){
    //so the numbers are stored as big endian AND as int24
    byte bytes[3];
    size_t s1 = fread(&bytes, 1, 3, regionFile);
    output->offset = (bytes[0] << 16) + (bytes[1] << 8) + bytes[2];
    //do an offset check
    long pos = ftell(regionFile);
    fseek(regionFile, 0, SEEK_END);
    if(output->offset > ftell(regionFile)){
        output->offset = -1;
        return 1;
    }
    fseek(regionFile, pos, SEEK_SET);
    //Luckily reading a single byte is simpler
    size_t s2 = fread(&output->sectorCount, 1, 1, regionFile);
    //0 if everything is fine
    return (s1 + s2 == 4) - 1 || (output->offset == 0 && output->sectorCount == 0);
}

int handleSecondSegment(chunk* output, FILE* regionFile){
    byte bytes[4];
    size_t s1 = fread(&bytes, 1, 4, regionFile);
    output->timestamp = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
    return (s1 == 4) - 1 && output->timestamp > 0;
}

chunk* getChunks(FILE* regionFile){
    chunk* chunks = (chunk*)malloc(sizeof(chunk) * chunkN);
    fseek(regionFile, 0, SEEK_SET);
    //despite what it might seem with these loops we are doing a simple linear go through the file
    //first there is a 4096 byte long section of 1024 4 bit fields. Each field is made up of 3 big endian encoded int24s and a single int8
    for(int i = 0; i < chunkN; i++){
        chunk newChunk;
        if(handleFirstSegment(&newChunk, regionFile) != 0){
            if(chunkIsNull(newChunk)){
                newChunk.byteLength = 0;
                newChunk.data = NULL;
                newChunk.sectorCount = 0;
                newChunk.timestamp = 0;
            }
            else{
                return NULL;
            }
        }
        chunks[i] = newChunk;
    }
    //Then there's an equally long section made up of 1024 int32 timestamps
    for(int i = 0; i < chunkN; i++){
        if(handleSecondSegment(&chunks[i], regionFile) != 0 && !chunkIsNull(chunks[i])){
            return NULL;
        }
    }
    //Then there's encoded chunk data in n*4096 byte long chunks
    //Each of these chunks is made up of a single int32 field that contains the length of the preceding compressed data
    //foreach chunk we have extracted so far
    for(int i = 0; i < chunkN; i++){
        if(!chunkIsNull(chunks[i])){ //if the chunk isn't NULL
            //find the corresponding section
            if(fseek(regionFile, segmentLength * chunks[i].offset, SEEK_SET) != 0){
                return NULL;
            } 
            //get the byteLength
            byte bytes[4];
            if(fread(&bytes, 1, 4, regionFile) != 4){
                return NULL;
            }
            chunks[i].byteLength = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
            //get the compression type
            if(fread(&chunks[i].compression, 1, 1, regionFile) != 1){
                return NULL;
            }
            //fseek(regionFile, 2, SEEK_CUR);
            //Then get the data
            chunks[i].byteLength += 5;
            byte* data = (byte*)malloc(chunks[i].byteLength);
            if(data == NULL){
                return NULL;
            }
            if(fread(data, 1, chunks[i].byteLength, regionFile) != chunks[i].byteLength){
                return NULL;
            }
            chunks[i].data = data;
        }
    }
    return chunks;
}

palettedContainer* getSections(chunk* chunk, size_t* sectionN, cJSON* version){
    nbt_node* node = nbt_parse_compressed(chunk->data, chunk->byteLength);
    if(node == NULL){
        return NULL;
    }
    if(node->type != TAG_COMPOUND){
        return NULL;
    }
    nbt_node* sectionsNode = nbt_find_by_name(node, "sections");
    if(sectionsNode == NULL){
        return NULL;
    }
    cJSON* blocks = cJSON_GetObjectItemCaseSensitive(version, "blocks");
    *sectionN = list_length(sectionsNode->payload.tag_list->entry.flink);
    palettedContainer* sections = (palettedContainer*)malloc(sizeof(palettedContainer) * *sectionN);
    struct nbt_list* sectionsList = sectionsNode->payload.tag_list;
    struct list_head* pos;
    //foreach section
    list_for_each(pos, &sectionsList->entry){ 
        //get the element
        struct nbt_list* el = list_entry(pos, struct nbt_list, entry);
        nbt_node* compound = el->data;
        //get the Y
        nbt_node* yNode = nbt_find_by_name(compound, "Y");
        if(yNode == NULL){
            return NULL;
        }
        int8_t y = yNode->payload.tag_byte;
        palettedContainer newSection; //create new object that will store this data
        //get the block data
        nbt_node* blockNode = nbt_find_by_name(compound, "block_states");
        if(blockNode == NULL){
            return NULL;
        }
        {//Palette handling
            //get the palette
            nbt_node* palette = nbt_find_by_name(blockNode, "palette");
            if(palette == NULL){
                return NULL;
            }
            int* blockPalette = NULL;
            unsigned int i = 0;
            struct list_head* paletteCur;
            //foreach element in palette
            list_for_each(paletteCur, &palette->payload.tag_list->entry){
                //get the list entry
                struct nbt_list* pal = list_entry(paletteCur, struct nbt_list, entry);
                nbt_node* string = nbt_find_by_name(pal->data, "Name");
                nbt_node* blockProperties = nbt_find_by_name(pal->data, "Properties");
                cJSON* block = cJSON_GetObjectItemCaseSensitive(blocks, string->payload.tag_string);
                if(block == NULL){
                    return NULL;
                }
                cJSON* states = cJSON_GetObjectItemCaseSensitive(block, "states");
                if(states == NULL){
                    return NULL;
                }
                int id = -1; //id of the block, -1 if not found
                //foreach state
                cJSON* state = NULL;
                cJSON_ArrayForEach(state, states){
                    cJSON* properties = cJSON_GetObjectItemCaseSensitive(state, "properties");
                    if(properties == NULL){ //if there are no properties on the state then we have found the correct state
                        id = atoi(state->string);
                        break;
                    }
                    bool propertiesMatch = true;
                    cJSON* property = NULL;
                    cJSON_ArrayForEach(property, properties){ //foreach property
                        //get the corresponding property from the block
                        nbt_node* nbtProp = nbt_find_by_name(blockProperties, property->string);
                        if(nbtProp == NULL){
                            propertiesMatch = false;
                            break;
                        }
                        if(nbtProp->type != TAG_STRING){
                            fprintf(stderr, "Unknown property type %d\n", nbtProp->type);
                            propertiesMatch = false;
                            break;
                        }
                        switch(property->type){
                            case cJSON_String:
                                propertiesMatch &= strcmp(nbtProp->payload.tag_string, property->valuestring) == 0;
                                break;
                            case cJSON_False:
                            case cJSON_True:
                                bool val = nbtProp->payload.tag_string[0] == 't';
                                propertiesMatch &= val == property->valueint;
                                break;
                            case cJSON_Number:
                                propertiesMatch &= atoi(nbtProp->payload.tag_string) == property->valueint;
                                break;
                            default:
                                fprintf(stderr, "Unknown property type %d\n", property->type);
                                propertiesMatch = false;
                                break;
                        }
                    }
                    if(propertiesMatch){
                        id = atoi(state->string);
                        break; //if properties match then we have found the correct state
                    }
                }
                if(id == -1){
                    fprintf(stderr, "Could not find block %s in version (section:%d)\n", string->payload.tag_string, y);
                    return NULL;
                }
                blockPalette = realloc(blockPalette, (i + 1) * sizeof(int));
                blockPalette[i] = id;
                i++;
            }
            newSection.palette = blockPalette;
            newSection.paletteSize = i;
        }
        {//block states handling
            //get the individual block data
            nbt_node* blockData = nbt_find_by_name(blockNode, "data");
            if(blockData != NULL){
                //I pray to thee the almighty Omnisiah that this pasted in code from a year ago works
                unsigned short l = (unsigned short)ceilf(log2f((float)newSection.paletteSize));//length of indices in the long
                int m = 0;
                if(l < 4){
                    l = 4;
                }
                unsigned short numPerLong = (unsigned short)(64/l);
                newSection.states = calloc(numPerLong * blockData->payload.tag_long_array.length, sizeof(int32_t));
                //foreach long
                for(int a=0; a < blockData->payload.tag_long_array.length; a++){
                    uint64_t comp = blockData->payload.tag_long_array.data[a];
                    for(unsigned short b = 0; b < numPerLong; b++){
                        if(m >= 4096){
                            break;
                        }
                        unsigned short bits = b * l;
                        uint64_t mask = createMask(bits, l);
                        newSection.states[m] = (unsigned int)((mask & comp) >> bits);
                        m++;
                    }
                }
            }
            else{ //it can be null in which case the entire sector is full of palette[0]
                newSection.states = NULL;
            }
        }
        sections[y + 4] = newSection;
    }
    return sections;
}
