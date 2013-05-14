#pragma once

#include "Effect.h"

typedef std::shared_ptr< class BlackEffect > BlackEffectRef;

class BlackEffect: public Effect
{
	public:
		static BlackEffectRef create() { return BlackEffectRef( new BlackEffect() ); }

	private:
		BlackEffect() : Effect( "Black" ) {}
};

