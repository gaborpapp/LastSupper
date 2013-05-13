#pragma once

#include "Effect.h"

class BlackEffect;
typedef std::shared_ptr< BlackEffect > BlackEffectRef;

class BlackEffect: public Effect
{
	public:
		static BlackEffectRef create() { return BlackEffectRef( new BlackEffect() ); }

	private:
		BlackEffect() : Effect( "Black" ) {}
};

