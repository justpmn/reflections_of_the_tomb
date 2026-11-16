#include "Audio.h"
#include <iostream>
#include <map>
#include <vector>
// stb_vorbis supplies OGG decoding to miniaudio in this translation unit.
#define STB_VORBIS_HEADER_ONLY
#include "third_party/stb_vorbis.c"
#define MINIAUDIO_IMPLEMENTATION
#include "third_party/miniaudio.h"
#undef STB_VORBIS_HEADER_ONLY
#include "third_party/stb_vorbis.c"

struct Audio::State {
    ma_engine engine{};
    bool initialized=false;
    std::map<std::string,std::unique_ptr<ma_sound>> sounds;
    ma_sound* load(const std::string& file,bool stream) {
        auto found=sounds.find(file);
        if(found!=sounds.end()) return found->second.get();
        auto sound=std::make_unique<ma_sound>();
        const auto path="assets/audio/Audio and Sounds/"+file;
        const auto result=ma_sound_init_from_file(&engine,path.c_str(),stream?MA_SOUND_FLAG_STREAM:MA_SOUND_FLAG_DECODE,nullptr,nullptr,sound.get());
        if(result!=MA_SUCCESS) {std::cerr<<"Cannot decode audio "<<file<<": "<<result<<'\n';return nullptr;}
        auto* ptr=sound.get(); sounds.emplace(file,std::move(sound));
        return ptr;
    }
    ~State() {
        for(auto& pair:sounds) ma_sound_uninit(pair.second.get());
        if(initialized) ma_engine_uninit(&engine);
    }
};
Audio::Audio():state(std::make_unique<State>()) {
    state->initialized=ma_engine_init(nullptr,&state->engine)==MA_SUCCESS;
    if(!state->initialized) std::cerr<<"No audio device; continuing silently\n";
}
Audio::~Audio()=default;
bool Audio::available() const {return state->initialized;}
void Audio::music() {
    if(!available()) return;
    for(const auto& file:std::vector<std::string>{"Background music and ambience/magical_puzzle_music.mp3",
          "Background music and ambience/temple_exploration _ambience.ogg"}) {
        if(auto* sound=state->load(file,true)) {
            ma_sound_set_volume(sound,.18f);ma_sound_set_looping(sound,MA_TRUE);ma_sound_start(sound);
        }
    }
    std::cout<<"Audio engine active; music and ambience loaded\n";
}
void Audio::effect(const std::string& file) {
    if(!available()||muted) return;
    if(auto* sound=state->load(file,false)) {
        ma_sound_seek_to_pcm_frame(sound,0);ma_sound_set_volume(sound,.6f);ma_sound_start(sound);
    }
}
void Audio::toggle() {
    muted=!muted;
    if(available()) ma_engine_set_volume(&state->engine,muted?0.f:1.f);
}
