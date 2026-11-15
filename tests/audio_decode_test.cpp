#include "third_party/miniaudio.h"
#include <filesystem>
#include <iostream>
#include <set>
int main() {
    const std::filesystem::path dir=AUDIO_DIR;
    const std::set<std::string> required={"magical_puzzle_music.mp3","temple_exploration _ambience.ogg",
        "menu_selection.wav","rotate_mirror.wav","permanent_ruby_sealing.wav",
        "player_defeat.wav","stone_footsteps.wav","stone_door_opening_and_closing.ogg"};
    int failures=0;
    for(const auto& file:std::filesystem::recursive_directory_iterator(dir)) {
        if(!file.is_regular_file()) continue;
        const auto name=file.path().filename().string();
        ma_decoder decoder{};
        const auto config=ma_decoder_config_init(ma_format_f32,2,48000);
        const auto status=ma_decoder_init_file(file.path().string().c_str(),&config,&decoder);
        if(status!=MA_SUCCESS) {
            std::cout<<"UNSUPPORTED/CORRUPT "<<name<<'\n';
            if(required.count(name)) ++failures;
            continue;
        }
        float samples[2048]{};ma_uint64 read=0;
        ma_decoder_read_pcm_frames(&decoder,samples,1024,&read);
        ma_decoder_uninit(&decoder);
        if(required.count(name) && read==0) ++failures;
        std::cout<<"DECODE "<<name<<": "<<read<<" frames\n";
    }
    return failures?1:0;
}
