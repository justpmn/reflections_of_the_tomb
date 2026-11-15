#pragma once
#include <filesystem>
#include <string>
#include <iostream>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

// Native Windows mixer: existing WAV/MP3 assets, no new runtime dependency.
// Unavailable codecs/devices degrade to silence without interrupting gameplay.
class Audio {
public:
    bool muted = false;
    void music() {
#ifdef _WIN32
        open("Background music and ambience/magical_puzzle_music.mp3", "music", true);
        open("Background music and ambience/temple_exploration _ambience.ogg", "ambience", true);
#endif
    }
    void effect(const std::string& file) {
#ifdef _WIN32
        if (!muted) open(file,"effect",false);
#else
        (void)file;
#endif
    }
    void toggle() {
        muted=!muted;
#ifdef _WIN32
        command(std::string(muted?"pause ":"resume ")+"music");
        command(std::string(muted?"pause ":"resume ")+"ambience");
        if (muted) command("stop effect");
#endif
    }
    ~Audio() {
#ifdef _WIN32
        command("close music"); command("close ambience"); command("close effect");
#endif
    }
private:
#ifdef _WIN32
    static bool command(const std::string& s) {
        return mciSendStringA(s.c_str(),nullptr,0,nullptr)==0;
    }
    static void open(const std::string& file,const std::string& alias,bool loop) {
        command("close "+alias);
        const auto path=std::filesystem::absolute("assets/audio/Audio and Sounds/"+file).string();
        if (!command("open \""+path+"\" alias "+alias)) {
            std::cerr << "Audio unavailable: " << file << '\n'; return;
        }
        command("setaudio "+alias+" volume to "+(loop?"220":"650"));
        command("play "+alias+(loop?" repeat":""));
    }
#endif
};
