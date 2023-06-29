/*
 * funkinchrpak by UNSTOP4BLE (not the one ckdev made)
 * Packs characters into a binary file for the PSX port
*/

#include <iostream>
#include <fstream>

#include "json.hpp"
using json = nlohmann::json;

typedef int32_t fixed_t;

#define ASCR_REPEAT 0xFF
#define ASCR_CHGANI 0xFE
#define ASCR_BACK   0xFD

struct Animation
{
    //Animation data and script
    uint8_t spd;
    uint8_t script[255];
};

struct CharFrame
{
    uint8_t tex;
    uint16_t src[4];
    int16_t off[2];
};

std::vector<std::string> charAnim{
    "CharAnim_Idle",
    "CharAnim_Left",  "CharAnim_LeftAlt",
    "CharAnim_Down",  "CharAnim_DownAlt",
    "CharAnim_Up",    "CharAnim_UpAlt",
    "CharAnim_Right", "CharAnim_RightAlt",
};

struct CharacterFileHeader
{
    int32_t size_struct;
    int32_t size_frames;
    int32_t size_animation;
    int32_t sizes_scripts[9]; // size of charAnim vector

    //Character information
    uint8_t spec;
    uint8_t health_i; //hud1.tim
    uint32_t health_bar; //hud1.tim
    fixed_t focus_x, focus_y, focus_zoom;
};

std::vector<std::string> charStruct;

int getEnumFromString(std::vector<std::string> &compare, std::string str) {
    for (int i = 0; i < compare.size(); i++)
        if (strcmp(compare[i].c_str(), str.c_str()) == 0) return i;

    std::cout << "invalid enum " << str << std::endl;   
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "usage: funkinchrpak out_chr in_json" << std::endl;
        return 0;
    }
    
    //Read json
    std::ifstream file(argv[2]);
    if (!file.is_open())
    {
        std::cout << "Failed to open " << argv[2] << std::endl;
        return 1;
    }
    json j;
    file >> j;

    CharacterFileHeader new_char;

    new_char.spec = j["spec"];
    new_char.health_i = j["health_i"];
    std::string health_bar_str = j["health_bar"];
    new_char.health_bar = std::stoul(health_bar_str, nullptr, 16);
    new_char.focus_x = j["focus"][0];
    new_char.focus_y = j["focus"][1];
    new_char.focus_zoom = j["focus"][2];

    //parse animation
    for (int i = 0; i < j["struct"].size(); i++) 
        charStruct.push_back(j["struct"][i]);

    new_char.size_struct = j["struct"].size();
    CharFrame frames[j["frames"].size()];
    new_char.size_frames = j["frames"].size();
    //parse frames
    for (int i = 0; i < j["frames"].size(); i++) {
        std::string texstr = j["frames"][i][0];
        frames[i].tex = getEnumFromString(charStruct, j["frames"][i][0]);
        for (int i2 = 0; i2 < 4; i2++)
            frames[i].src[i2] = j["frames"][i][1][i2];
        frames[i].off[0] = j["frames"][i][2][0];
        frames[i].off[1] = j["frames"][i][2][1];
    }

    Animation anims[j["animation"].size()];     
    std::vector<std::vector<uint16_t>> scripts;
    new_char.size_animation = j["animation"].size();

    for (int i = 0; i < j["animation"].size(); i++) {
        scripts.resize(j["animation"].size());
        scripts[i].resize(j["animation"][i][1].size());
        new_char.sizes_scripts[i] = j["animation"][i][1].size();

        anims[i].spd = j["animation"][i][0];

        for (int i2 = 0; i2 < j["animation"][i][1].size(); i2++) {
            if (i2 < j["animation"][i][1].size()-2)
            {
//                std::cout << j["animation"][i][1][i2] << std::endl;
                scripts[i][i2] = j["animation"][i][1][i2];
            }
            else //change string to number
            {
                if (i2 == j["animation"][i][1].size()-2) { // ascr mode
                    //std::cout << j["animation"][i][1][i2] << std::endl;
                    std::string ascrmode = j["animation"][i][1][i2];
                    if (ascrmode == "ASCR_BACK")
                        scripts[i][i2] = ASCR_BACK;
                    else if (ascrmode == "ASCR_CHGANI")
                        scripts[i][i2] = ASCR_CHGANI;
                    else if (ascrmode == "ASCR_REPEAT")
                        scripts[i][i2] = ASCR_REPEAT;
                }
                if (i2 == j["animation"][i][1].size()-1) // back animation
                {
                    bool isstring = !(j["animation"][i][1][i2] == sizeof(uint8_t));
                    if (isstring) {
                        std::string backanim = j["animation"][i][1][i2];
                        scripts[i][i2] = getEnumFromString(charAnim, backanim);
                    }
                    else
                        scripts[i][i2] = j["animation"][i][1][i2];
                    //std::cout << j["animation"][i][1][i2] << std::endl;
                }
            }
        }
    }

    //copy over the shit into the arrays 
    Animation animations[charAnim.size()];
    for (int i = 0; i < charAnim.size(); i++)
    {
        animations[i].spd = anims[i].spd;
        for (int i2 = 0; i2 < 256; i2++)
            animations[i].script[i2] = scripts[i][i2];
    }


    std::ofstream binFile(argv[1], std::ostream::binary);
    binFile.write(reinterpret_cast<const char*>(&new_char), sizeof(new_char));
    binFile.write(reinterpret_cast<const char*>(&animations), sizeof(animations));
    binFile.write(reinterpret_cast<const char*>(&frames), sizeof(frames));
    binFile.close();   

    //test reading
    CharacterFileHeader testchar;
    std::ifstream inFile(argv[1], std::istream::binary);
    inFile.read(reinterpret_cast<char *>(&testchar), sizeof(testchar));
    Animation animationstest[testchar.size_animation];
    CharFrame framestest[testchar.size_frames];
    inFile.read(reinterpret_cast<char *>(&animationstest), testchar.size_animation * sizeof(Animation));
    inFile.read(reinterpret_cast<char *>(&framestest), testchar.size_frames * sizeof(CharFrame));
    inFile.close();   

    //print header
    std::cout << "spec " << static_cast<unsigned int>(testchar.spec) << std::endl;
    std::cout << "icon " << static_cast<unsigned int>(testchar.health_i) << std::endl;
    std::cout << "healthbar " << static_cast<unsigned int>(testchar.health_bar) << std::endl;
    std::cout << "cx " << testchar.focus_x << std::endl;
    std::cout << "cy " << testchar.focus_y << std::endl;
    std::cout << "cz " << testchar.focus_zoom << std::endl;

    //print frames array
    for (int i = 0; i < testchar.size_frames; ++i)
    {
        std::cout << "tex " << static_cast<unsigned int>(framestest[i].tex) << " frames " << framestest[i].src[0] << " " << framestest[i].src[1]  << " " << framestest[i].src[2]  <<" " << framestest[i].src[3] <<" offset " << framestest[i].off[0]  << " " << framestest[i].off[1]  <<" " <<  std::endl;
    }   

    //print script arrays
    for (int i = 0; i < testchar.size_animation; i++) {
        std::cout << "speed " << static_cast<unsigned int>(animationstest[i].spd) << " frames";
        for (int i2 = 0; i2 < testchar.sizes_scripts[i]; i2 ++)
            std::cout << " " << static_cast<unsigned int>(animationstest[i].script[i2]);
        std::cout << std::endl;
    }

    return 0;
}