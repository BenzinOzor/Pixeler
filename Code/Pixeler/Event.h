#pragma once

#include <FZN/DataStructure/Variant.h>


namespace Pixeler
{
	class Event
	{
	public:
		enum class Type
		{
			//original_sprite_opacity_changed,		// The opacity of the original sprite has been changed (uint8_t: new opacity value)
			COUNT
		};

		Event() = default;
		Event( Type _type ) : m_type( _type ) {}
		static Event* create( Type _type ) { return new Event( _type ); }

		Type m_type{ Type::COUNT };

		fzn::Variant<uint8_t> m_event_data;
	};
};
