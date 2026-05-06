#pragma once

#include <cstdint>
#include <string_view>

namespace MMO {

	// Well-known socket identifiers. Stable wire/disk values — never renumber;
	// deprecated entries become reserved. Custom is the escape hatch for
	// authored names not in this list (the original string travels alongside).
	enum class SocketId : uint8_t
	{
		// Overhead UI
		OverheadName    = 0,  // nameplate, target frame, cast bar default anchor
		OverheadMounted = 1,  // taller anchor when unit is mounted

		// Combat / FX
		Chest           = 2,  // center-of-mass; damage text spawn fallback
		Head            = 3,  // head-bone anchor for crit sparks
		WeaponR         = 4,  // main-hand weapon mount
		WeaponL         = 5,  // off-hand weapon mount
		MuzzleR         = 6,  // projectile spawn at right-hand weapon tip
		MuzzleL         = 7,  // projectile spawn at left-hand weapon tip
		ShieldL         = 8,  // shield mount on off-hand
		SheathBack      = 9,  // sheathed weapon position on back
		SheathHipL      = 10, // sheathed weapon position on left hip
		SheathHipR      = 11, // sheathed weapon position on right hip
		HandR           = 12, // right hand: caster orb, channel beam endpoint
		HandL           = 13, // left hand
		SpellOrigin     = 14, // generic spell-origin (mid-chest fallback)

		// Mount / Riding
		MountSeat       = 15, // where rider's pelvis attaches when mounted
		MountReinsL     = 16, // rider hand IK target left
		MountReinsR     = 17, // rider hand IK target right

		// Equipment slots (cosmetic gear attach)
		EquipHelm       = 18,
		EquipShoulderL  = 19,
		EquipShoulderR  = 20,
		EquipBackpack   = 21,
		EquipCape       = 22,
		EquipBeltFront  = 23,
		EquipBeltBack   = 24,
		EquipQuiver     = 25,

		// Reserved range 26..62 for future well-known additions

		Custom          = 0xFF // not in this list; original name travels alongside
	};

	// Authored name (without "socket_" prefix) -> ID. Case-sensitive; unknown -> Custom.
	SocketId SocketNameToId(std::string_view name);

	// ID -> canonical name. Returns nullptr for Custom or unrecognized values.
	const char* SocketIdToName(SocketId id);

} // namespace MMO
