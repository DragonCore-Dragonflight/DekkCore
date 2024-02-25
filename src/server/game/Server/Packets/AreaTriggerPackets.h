/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AreaTriggerPackets_h__
#define AreaTriggerPackets_h__

#include "Packet.h"
#include "AreaTriggerTemplate.h"
#include "ObjectGuid.h"
#include "Optional.h"

namespace WorldPackets
{
    namespace AreaTrigger
    {
        struct AreaTriggerSplineInfo
        {
            uint32 TimeToTarget = 0;
            uint32 ElapsedTimeForMovement = 0;
            std::vector<TaggedPosition<Position::XYZ>> Points;
        };

        struct AreaTriggerMovementScriptInfo
        {
            uint32 SpellScriptID = 0;
            TaggedPosition<Position::XYZ> Center;
        };

        class AreaTrigger final : public ClientPacket
        {
        public:
            AreaTrigger(WorldPacket&& packet) : ClientPacket(CMSG_AREA_TRIGGER, std::move(packet)) { }

            void Read() override;

            int32 AreaTriggerID = 0;
            bool Entered = false;
            bool FromClient = false;
        };

        class AreaTriggerDenied final : public ServerPacket
        {
        public:
            AreaTriggerDenied() : ServerPacket(SMSG_AREA_TRIGGER_DENIED, 5) { }

            int32 AreaTriggerID = 0;
            bool Entered = false;

            WorldPacket const* Write() override;
        };

        class AreaTriggerNoCorpse final : public ServerPacket
        {
        public:
            AreaTriggerNoCorpse() : ServerPacket(SMSG_AREA_TRIGGER_NO_CORPSE, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class AreaTriggerRePath final : public ServerPacket
        {
        public:
            AreaTriggerRePath() : ServerPacket(SMSG_AREA_TRIGGER_RE_PATH, 17) { }

            WorldPacket const* Write() override;

            Optional<AreaTriggerSplineInfo> AreaTriggerSpline;
            Optional<AreaTriggerOrbitInfo> AreaTriggerOrbit;
            Optional<AreaTriggerMovementScriptInfo> AreaTriggerMovementScript;
            ObjectGuid TriggerGUID;
        };

        //DekkCore 
        class UpdateAreatriggerVisual final : public ClientPacket
        {
        public:
            UpdateAreatriggerVisual(WorldPacket&& packet) : ClientPacket(CMSG_UPDATE_AREA_TRIGGER_VISUAL, std::move(packet)) { }

            void Read() override;

            int32 AreaTriggerID = 0;
            int32 unk1 = 0;
            int32 unk2 = 0;
            ObjectGuid TriggerGUID;
        };

        //DekkCore
    }
}

ByteBuffer& operator<<(ByteBuffer& data, AreaTriggerOrbitInfo const& areaTriggerCircularMovement);

#endif // AreaTriggerPackets_h__
