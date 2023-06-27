/*
 * funkinchrpak by UNSTOP4BLE (not the one ckdev made)
 * Packs characters into a binary file for the PSX port
*/

#include <iostream>

#include "json.hpp"
using json = nlohmann::json;

typedef int32_t fixed_t;

#define ASCR_REPEAT 0xFF
#define ASCR_CHGANI 0xFE
#define ASCR_BACK   0xFD

typedef struct
{
    //Animation data and script
    uint8_t spd;
    const uint8_t *script;
} Animation;

typedef struct
{
    //Animation state
    const Animation *anims;
    const uint8_t *anim_p;
    uint8_t anim;
    fixed_t anim_time, anim_spd;
    bool ended;
} Animatable;

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

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "usage: funkinchrpak out_json in_json" << std::endl;
        return 0;
    }
    
    //Read json
    std::ifstream i(argv[2]);
    if (!i.is_open())
    {
        std::cout << "Failed to open " << argv[2] << std::endl;
        return 1;
    }
    json j;
    i >> j;



    return 0;
}