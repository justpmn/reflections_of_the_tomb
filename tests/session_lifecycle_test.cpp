#include "gameplay/GameSession.h"
#include <iostream>
#include <stdexcept>

void require(bool condition,const char* message) {if(!condition)throw std::runtime_error(message);}
int main() {
    try {
        // Entities own their animation state, including after vector reallocations.
        std::vector<Ruby> rubyGrowth;
        std::vector<Trap> trapGrowth;
        for(int i=0;i<512;++i) {
            rubyGrowth.emplace_back(true);trapGrowth.emplace_back();
            require(rubyGrowth.back().sealingProgress()==0,"new ruby has animation state before first tick");
            rubyGrowth.back().hitByLight();trapGrowth.back().deactivate();
            for(auto& ruby:rubyGrowth)ruby.update(.016f);
            for(auto& trap:trapGrowth)trap.update(.016f);
            require(rubyGrowth.back().sealingProgress()>0,"first activated tick is valid after reallocation");
        }
        Ruby decoy(false);decoy.hitByLight();decoy.update(1);
        require(decoy.sealingProgress()==0,"false ruby never seals");
        for(int iteration=0;iteration<40;++iteration) {
            GameSession session(LEVELS_DIR);require(session.restart(),"start lifetime iteration");
            for(int level:{0,2,1,2,0}) {
                require(session.levels.loadIndex(level)&&session.reset(),"switch levels with different entity counts");
                for(const auto& ruby:session.rubies)require(ruby.sealingProgress()==0,"level load initializes ruby-owned animation state");
                for(auto& ruby:session.rubies)ruby.hitByLight();
                for(int frame=0;frame<25;++frame)session.update(0,0,.05f);
                for(const auto& ruby:session.rubies)require(ruby.sealingProgress()==1,"sealing survives first frame and completes");
                require(session.solved(),"all linked ruby states unlock door");
                require(session.reset(),"reset after activation");
                for(const auto& ruby:session.rubies)require(!ruby.isActivated()&&ruby.sealingProgress()==0,"reset clears activation and progress together");
                for(const auto& trap:session.traps)require(trap.isActive()&&trap.extension()==1,"reset reconstructs traps and progress together");
            }
            require(session.restart()&&session.levels.index()==0,"restart lifetime");
        } // Repeated destructors run here, with no renderer or audio involved.
        std::cout<<"PASS 512 reallocations, first-seal ticks and 200 level resets/transitions/destructions\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
