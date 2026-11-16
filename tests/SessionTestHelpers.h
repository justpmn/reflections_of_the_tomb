#pragma once
#include "gameplay/GameSession.h"
inline void finishEntry(GameSession& g){for(int i=0;i<180&&!g.victory&&g.phase!=GameSession::Phase::Playing;++i)g.update(0,0,.05f);}
inline void finishRespawn(GameSession& g){for(int i=0;i<70&&g.characterState!=GameSession::CharacterState::Alive;++i)g.update(0,0,.05f);}
