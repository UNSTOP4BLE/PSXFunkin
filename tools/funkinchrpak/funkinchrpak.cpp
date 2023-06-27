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
    const uint8_t *script;
};

struct Animatable
{
    //Animation state
    const Animation *anims;
    const uint8_t *anim_p;
    uint8_t anim;
    fixed_t anim_time, anim_spd;
    bool ended;
};

struct CharFrame
{
    uint8_t tex;
    uint16_t src[4];
    int16_t off[2];
};

struct Character
{
    //Character functions
    void (*tick)(struct Character*);
    void (*set_anim)(struct Character*, uint8_t);
    void (*free)(struct Character*);
    
    //Position
    fixed_t x, y;
    
    //Character information
    uint8_t spec;
    uint8_t health_i; //hud1.tim
    uint32_t health_bar; //hud1.tim
    fixed_t focus_x, focus_y, focus_zoom;
    
    //Animation state
    Animatable animatable;
    fixed_t sing_end;
    uint16_t pad_held;
};

std::vector<std::string> charStruct;

std::vector<std::string> charAnim{
    "CharAnim_Idle",
    "CharAnim_Left",  "CharAnim_LeftAlt",
    "CharAnim_Down",  "CharAnim_DownAlt",
    "CharAnim_Up",    "CharAnim_UpAlt",
    "CharAnim_Right", "CharAnim_RightAlt",
};

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
        std::cout << "usage: funkinchrpak out_bin in_json" << std::endl;
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

    Character new_char;

    new_char.tick = nullptr;
    new_char.set_anim = nullptr;
    new_char.free = nullptr;

    new_char.x = 0;
    new_char.y = 0;

    new_char.spec = j["spec"];
    new_char.health_i = j["health_i"];
    std::string health_bar_str = j["health_bar"];
    new_char.health_bar = atoi(health_bar_str.c_str());
    new_char.focus_x = new_char.focus_y = new_char.focus_zoom = 0;
    //parse animation
    for (int i = 0; i < j["struct"].size(); i++) 
        charStruct.push_back(j["struct"][i]);

    CharFrame frames[j["frames"].size()];
    //parse frames
    for (int i = 0; i < j["frames"].size(); i++) {
        std::string texstr = j["frames"][i][0];
        frames[i].tex = getEnumFromString(charStruct, j["frames"][i][0]);
        for (int itwo = 0; itwo < 4; itwo++)
            frames[i].src[itwo] = j["frames"][i][1][itwo];
        frames[i].off[0] = j["frames"][i][2][0];
        frames[i].off[1] = j["frames"][i][2][1];
    }

    new_char.sing_end = 0;
    new_char.pad_held = 0;

    //parse animatable
    new_char.animatable.anims = nullptr;
    new_char.animatable.anim_p = nullptr;
    new_char.animatable.anim = 0;
    new_char.animatable.anim_time = 0;
    new_char.animatable.anim_spd = 0;
    new_char.animatable.ended = false;

    Animation anims[j["animation"].size()];     
    std::vector<std::vector<uint16_t>> scripts;
    for (int i = 0; i < j["animation"].size(); i++) {
        scripts.resize(j["animation"].size());
        scripts[i].resize(j["animation"][i][1].size());
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

    //print header
    std::cout << "spec " << static_cast<unsigned int>(new_char.spec) << std::endl;
    std::cout << "icon " << static_cast<unsigned int>(new_char.health_i) << std::endl;
    std::cout << "healthbar " << static_cast<unsigned int>(new_char.health_bar) << std::endl;
    std::cout << "cx " << new_char.focus_x << std::endl;
    std::cout << "cy " << new_char.focus_y << std::endl;
    std::cout << "cz " << new_char.focus_zoom << std::endl;

    //print frames array
    for (int i = 0; i < j["frames"].size(); ++i)
    {
        std::cout << "tex " << static_cast<unsigned int>(frames[i].tex) << " frames " << frames[i].src[0] << " " << frames[i].src[1]  << " " << frames[i].src[2]  <<" " << frames[i].src[3] <<" offset " << frames[i].off[0]  << " " << frames[i].off[1]  <<" " <<  std::endl;
    }   

    //print script arrays
    for (int i = 0; i < scripts.size(); i++) {
        std::cout << "speed " << static_cast<unsigned int>(anims[i].spd) << " frames";
        for (int i2 = 0; i2 < scripts[i].size(); i2 ++)
            std::cout << " " << scripts[i][i2];
        std::cout << std::endl;
    }

    return 0;
}