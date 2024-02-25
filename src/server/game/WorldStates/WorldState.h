#ifndef WORLD_STATE_H
#define WORLD_STATE_H

#include "Common.h"
#include "Define.h"
#include <unordered_map>
#include <unordered_set>

namespace WorldStatesMgrData
{
    enum WorldStates : uint32
    {
        WS_NONE = 0,

        BG_WS_ENABLE_TIMER = 4247,
        BG_WS_CURRENT_TIMER = 4248,
        BG_WS_UNKNOWN = 4249, ///< Used after flag is captured (value: 1)
        BG_WS_FLAG_UNK_ALLIANCE = 1545, ///< Value: -1 when alliance flag is dropped | 1 when alliance flag is on player | 0 On base | -2 ???
        BG_WS_FLAG_UNK_HORDE = 1546, ///< Value: -1 when horde flag is dropped    | 1 when horde flag is on player    | 0 On base | -2 ???
        BG_WS_FLAG_UNKNOWN = 1547, ///< -1 before capturing flag, 0 after both flags respawned
        BG_WS_FLAG_CAPTURES_ALLIANCE = 1581,
        BG_WS_FLAG_CAPTURES_HORDE = 1582,
        BG_WS_FLAG_CAPTURES_MAX = 1601,
        BG_WS_FLAG_STATE_HORDE = 2338, ///< 0 - hide, 1 - flag ok, 2 - flag picked up (flashing), 3 - flag picked up (not flashing)
        BG_WS_FLAG_STATE_ALLIANCE = 2339, ///< 0 - hide, 1 - flag ok, 2 - flag picked up (flashing), 3 - flag picked up (not flashing)
        BG_WS_BATTLE_TIMER = 5333,



        WS_RATED_BG_ENABLED = 5508,
        WS_ARENA_SEASON_ID = 3901,
        WS_PVP_ARENA_ENABLED = 3191,


        ALLIANCE_RESOUCES = 1776,
        ARENA_ALIVE_PLAYERS_GOLD = 3601,
        ARENA_ALIVE_PLAYERS_GREEN = 3600,
        ARENA_END_TIMER = 8529,
        ARENA_SHOW_END_TIMER = 8524,
        AV_AID_A_A = 1326,
        AV_AID_A_C = 1325,
        AV_AID_H_A = 1328,
        AV_AID_H_C = 1327,
        AV_Alliance_Score = 3127,
        AV_DUNN_ASSAULTED = 1379,
        AV_DUNN_CONTROLLED = 1362,
        AV_DUNN_DESTROYED = 1371,
        AV_DUNS_ASSAULTED = 1378,
        AV_DUNS_CONTROLLED = 1361,
        AV_DUNS_DESTROYED = 1370,
        AV_FROSTWOLF_A_A = 1339, //over ac
        AV_FROSTWOLF_A_C = 1337, //over hc
        AV_FROSTWOLF_H_A = 1340, //over aa
        AV_FROSTWOLF_H_C = 1338,
        AV_FROSTWOLFE_ASSAULTED = 1388,
        AV_FROSTWOLFE_CONTROLLED = 1383,
        AV_FROSTWOLFE_DESTROYED = 1366,
        AV_FROSTWOLFE_UNUSED = 1393,
        AV_FROSTWOLFHUT_A_A = 1331, //over ha
        AV_FROSTWOLFHUT_A_C = 1329, //over hc
        AV_FROSTWOLFHUT_H_A = 1332, //over ac
        AV_FROSTWOLFHUT_H_C = 1330,
        AV_FROSTWOLFW_ASSAULTED = 1387, //over destroyed
        AV_FROSTWOLFW_CONTROLLED = 1382,
        AV_FROSTWOLFW_DESTROYED = 1365, //over controlled
        AV_FROSTWOLFW_UNUSED = 1392,
        AV_Horde_Score = 3128,
        AV_ICEBLOOD_A_A = 1348, //over ac
        AV_ICEBLOOD_A_C = 1346, //over hc
        AV_ICEBLOOD_ASSAULTED = 1390,
        AV_ICEBLOOD_CONTROLLED = 1385,
        AV_ICEBLOOD_DESTROYED = 1368,
        AV_ICEBLOOD_H_A = 1349, //over aa
        AV_ICEBLOOD_H_C = 1347,
        AV_ICEBLOOD_UNUSED = 1395,
        AV_ICEWING_ASSAULTED = 1380,
        AV_ICEWING_CONTROLLED = 1363,
        AV_ICEWING_DESTROYED = 1372,
        AV_ICEWING_UNUSED = 1376,
        AV_N_MINE_A = 1358,
        AV_N_MINE_H = 1359,
        AV_N_MINE_N = 1360,
        AV_PIKEGRAVE_A_A = 1335,
        AV_PIKEGRAVE_A_C = 1333,
        AV_PIKEGRAVE_H_A = 1336,
        AV_PIKEGRAVE_H_C = 1334,
        AV_S_MINE_A = 1355,
        AV_S_MINE_H = 1356,
        AV_S_MINE_N = 1357,
        AV_SHOW_A_SCORE = 3134,
        AV_SHOW_H_SCORE = 3133,
        AV_SNOWFALL_A_A = 1343, //over ha
        AV_SNOWFALL_A_C = 1341, //over hc
        AV_SNOWFALL_H_A = 1344, //over ac
        AV_SNOWFALL_H_C = 1342,
        AV_SNOWFALL_N = 1966, //over aa
        AV_STONEH_ASSAULTED = 1381,
        AV_STONEH_CONTROLLED = 1364,
        AV_STONEH_DESTROYED = 1373,
        AV_STONEH_UNUSED = 1377,
        AV_STONEHEART_A_A = 1304, //over hc
        AV_STONEHEART_A_C = 1302,
        AV_STONEHEART_H_A = 1303, //over aa
        AV_STONEHEART_H_C = 1301, //over ac
        AV_TOWERPOINT_ASSAULTED = 1389, //goes over destroyed
        AV_TOWERPOINT_CONTROLLED = 1384,
        AV_TOWERPOINT_DESTROYED = 1367, //goes over controlled
        AV_TOWERPOINT_UNUSED = 1394,
        BATTLEFIELD_WG_WORLD_STATE_ACTIVE = 3801,
        BATTLEFIELD_WG_WORLD_STATE_ATTACKER = 3803,
        BATTLEFIELD_WG_WORLD_STATE_DEFENDER = 3802,
        BATTLEFIELD_WG_WORLD_STATE_MAX_VEHICLE_A = 3681,
        BATTLEFIELD_WG_WORLD_STATE_MAX_VEHICLE_H = 3491,
        BATTLEFIELD_WG_WORLD_STATE_SHOW_WORLDSTATE = 3710,
        BATTLEFIELD_WG_WORLD_STATE_VEHICLE_A = 3680,
        BATTLEFIELD_WG_WORLD_STATE_VEHICLE_H = 3490,
        BG_AB_OP_BLACKSMITH_ICON = 1846,             //Blacksmith map icon (NONE)
        BG_AB_OP_BLACKSMITH_STATE_ALIENCE = 1782,             //Blacksmith map state (ALIENCE)
        BG_AB_OP_BLACKSMITH_STATE_CON_ALI = 1784,             //Blacksmith map state (CON ALIENCE)
        BG_AB_OP_BLACKSMITH_STATE_CON_HOR = 1785,             //Blacksmith map state (CON HORDE)
        BG_AB_OP_BLACKSMITH_STATE_HORDE = 1783,             //Blacksmith map state (HORDE)
        BG_AB_OP_FARM_ICON = 1845,             //Farm map icon (NONE)
        BG_AB_OP_FARM_STATE_ALIENCE = 1772,             //Farm state (ALIENCE)
        BG_AB_OP_FARM_STATE_CON_ALI = 1774,             //Farm state (CON ALIENCE)
        BG_AB_OP_FARM_STATE_CON_HOR = 1775,             //Farm state (CON HORDE)
        BG_AB_OP_FARM_STATE_HORDE = 1773,             //Farm state (HORDE)
        BG_AB_OP_GOLDMINE_ICON = 1843,             //Gold Mine map icon (NONE)
        BG_AB_OP_GOLDMINE_STATE_ALIENCE = 1787,             //Gold Mine map state (ALIENCE)
        BG_AB_OP_GOLDMINE_STATE_CON_ALI = 1789,             //Gold Mine map state (CON ALIENCE
        BG_AB_OP_GOLDMINE_STATE_CON_HOR = 1790,             //Gold Mine map state (CON HORDE)
        BG_AB_OP_GOLDMINE_STATE_HORDE = 1788,             //Gold Mine map state (HORDE)
        BG_AB_OP_LUMBERMILL_ICON = 1844,             //Lumber Mill map icon (NONE)
        BG_AB_OP_LUMBERMILL_STATE_ALIENCE = 1792,             //Lumber Mill map state (ALIENCE)
        BG_AB_OP_LUMBERMILL_STATE_CON_ALI = 1794,             //Lumber Mill map state (CON ALIENCE)
        BG_AB_OP_LUMBERMILL_STATE_CON_HOR = 1795,             //Lumber Mill map state (CON HORDE)
        BG_AB_OP_LUMBERMILL_STATE_HORDE = 1793,             //Lumber Mill map state (HORDE)
        BG_AB_OP_RESOURCES_WARNING = 1955, GILNEAS_BG_OP_RESOURCES_WARNING = 1955,
        BG_AB_OP_STABLE_ICON = 1842,             //Stable map icon (NONE)
        BG_AB_OP_STABLE_STATE_ALIENCE = 1767,             //Stable map state (ALIENCE)
        BG_AB_OP_STABLE_STATE_CON_ALI = 1769,             //Stable map state (CON ALIENCE)
        BG_AB_OP_STABLE_STATE_CON_HOR = 1770,             //Stable map state (CON HORDE)
        BG_AB_OP_STABLE_STATE_HORDE = 1768,             //Stable map state (HORDE)
        BG_IC_ALLIANCE_KEEP_CONFLICT_A = 4342,
        BG_IC_ALLIANCE_KEEP_CONFLICT_H = 4343,
        BG_IC_ALLIANCE_KEEP_CONTROLLED_A = 4339,
        BG_IC_ALLIANCE_KEEP_CONTROLLED_H = 4340,
        BG_IC_ALLIANCE_KEEP_UNCONTROLLED = 4341,
        BG_IC_ALLIANCE_RENFORT = 4226,
        BG_IC_ALLIANCE_RENFORT_SET = 4221,
        BG_IC_DOCKS_CONFLICT_A = 4305,
        BG_IC_DOCKS_CONFLICT_H = 4302,
        BG_IC_DOCKS_CONTROLLED_A = 4304,
        BG_IC_DOCKS_CONTROLLED_H = 4303,
        BG_IC_DOCKS_UNCONTROLLED = 4301,
        BG_IC_GATE_EAST_A_WS_CLOSED = 4326,
        BG_IC_GATE_EAST_A_WS_OPEN = 4325,
        BG_IC_GATE_EAST_H_WS_CLOSED = 4319,
        BG_IC_GATE_EAST_H_WS_OPEN = 4320,
        BG_IC_GATE_FRONT_A_WS_CLOSED = 4328,
        BG_IC_GATE_FRONT_A_WS_OPEN = 4323,
        BG_IC_GATE_FRONT_H_WS_CLOSED = 4317,
        BG_IC_GATE_FRONT_H_WS_OPEN = 4322,
        BG_IC_GATE_WEST_A_WS_CLOSED = 4327,
        BG_IC_GATE_WEST_A_WS_OPEN = 4324,
        BG_IC_GATE_WEST_H_WS_CLOSED = 4318,
        BG_IC_GATE_WEST_H_WS_OPEN = 4321,
        BG_IC_HANGAR_CONFLICT_A = 4300,
        BG_IC_HANGAR_CONFLICT_H = 4297,
        BG_IC_HANGAR_CONTROLLED_A = 4299,
        BG_IC_HANGAR_CONTROLLED_H = 4298,
        BG_IC_HANGAR_UNCONTROLLED = 4296,
        BG_IC_HORDE_KEEP_CONFLICT_A = 4347,
        BG_IC_HORDE_KEEP_CONFLICT_H = 4348,
        BG_IC_HORDE_KEEP_CONTROLLED_A = 4344,
        BG_IC_HORDE_KEEP_CONTROLLED_H = 4345,
        BG_IC_HORDE_KEEP_UNCONTROLLED = 4346,
        BG_IC_HORDE_RENFORT = 4227,
        BG_IC_HORDE_RENFORT_SET = 4222,
        BG_IC_QUARRY_CONFLICT_A = 4310,
        BG_IC_QUARRY_CONFLICT_H = 4307,
        BG_IC_QUARRY_CONTROLLED_A = 4309,
        BG_IC_QUARRY_CONTROLLED_H = 4308,
        BG_IC_QUARRY_UNCONTROLLED = 4306,
        BG_IC_REFINERY_CONFLICT_A = 4315,
        BG_IC_REFINERY_CONFLICT_H = 4312,
        BG_IC_REFINERY_CONTROLLED_A = 4314,
        BG_IC_REFINERY_CONTROLLED_H = 4313,
        BG_IC_REFINERY_UNCONTROLLED = 4311,
        BG_IC_WORKSHOP_CONFLICT_A = 4228,
        BG_IC_WORKSHOP_CONFLICT_H = 4293,
        BG_IC_WORKSHOP_CONTROLLED_A = 4229,
        BG_IC_WORKSHOP_CONTROLLED_H = 4230,
        BG_IC_WORKSHOP_UNCONTROLLED = 4294,
        BG_KT_BLUE_ORB_C = 6969,
        BG_KT_BLUE_ORB_X = 6970,
        BG_KT_GREEN_ORB_C = 6965,
        BG_KT_GREEN_ORB_X = 6966,
        BG_KT_ICON_A = 6308,
        BG_KT_ICON_BLUE_ORB_ICON = 6714,
        BG_KT_ICON_GREEN_ORB_ICON = 6716,
        BG_KT_ICON_H = 6307,
        BG_KT_ICON_ORANGE_ORB_ICON = 6717,
        BG_KT_ICON_PURPLE_ORB_ICON = 6715,
        BG_KT_ORANGE_ORB_C = 6963,
        BG_KT_ORANGE_ORB_X = 6964,
        BG_KT_ORB_POINTS_A = 6303,
        BG_KT_ORB_POINTS_H = 6304,
        BG_KT_ORB_STATE = 6309,
        BG_KT_PURPLE_ORB_C = 6967,
        BG_KT_PURPLE_ORB_X = 6968,
        BG_KT_BRAWL_TWO_ORBS = 13583,
        BG_RV_WORLD_STATE = 3610,
        BG_SA_ALLIANCE_DEFENCE_TOKEN = 3630,
        BG_SA_ALLY_ATTACKS = 4352,
        BG_SA_ANCIENT_GATEWS = 3849,
        BG_SA_BLUE_GATEWS = 3620,
        BG_SA_BONUS_TIMER = 3571,
        BG_SA_CENTER_GY_ALLIANCE = 3637,
        BG_SA_CENTER_GY_HORDE = 3634,
        BG_SA_ENABLE_TIMER = 3564, //3565 second round timer?
        BG_SA_GREEN_GATEWS = 3623,
        BG_SA_HORDE_ATTACKS = 4353,
        BG_SA_HORDE_DEFENCE_TOKEN = 3631,
        BG_SA_LEFT_ATT_TOKEN_ALL = 3626,
        BG_SA_LEFT_ATT_TOKEN_HRD = 3629,
        BG_SA_LEFT_GY_ALLIANCE = 3635,
        BG_SA_LEFT_GY_HORDE = 3633,
        BG_SA_PURPLE_GATEWS = 3614,
        BG_SA_RED_GATEWS = 3617,
        BG_SA_RIGHT_ATT_TOKEN_ALL = 3627,
        BG_SA_RIGHT_ATT_TOKEN_HRD = 3628,
        BG_SA_RIGHT_GY_ALLIANCE = 3636,
        BG_SA_RIGHT_GY_HORDE = 3632,
        BG_SA_TIMER = 3557, //cur time? 1386402226 | 1386402896
        BG_SA_YELLOW_GATEWS = 3638,
        BG_TB_NEXT_BATTLE_TIMER = 5332,
        BLOOD_ELF_ALLIANCE_CONTROL = 2723,
        BLOOD_ELF_HORDE_CONTROL = 2724,
        BLOOD_ELF_UNCONTROL = 2722,
        DG_ALLIANCE_CART_ASSAULT = 7904, // 1 default to show alliance points; 2 cart assault; 3 cart dropeed
        DG_ALLIANCE_NODES = 8230,
        DG_ALLIANCE_POINTS = 7880,
        DG_GOBLIN_MINE_ALLIANCE_ASSAULT = 7864,
        DG_GOBLIN_MINE_HORDE_ASSAULT = 7865,
        DG_GOBLIN_MINE_CAPTURED_BY_TEAM = 7856,
        DG_PANDAREN_MINE_ALLIANCE_ASSAULT = 7857,
        DG_PANDAREN_MINE_HORDE_ASSAULT = 7861,
        DG_PANDAREN_MINE_CAPTURED_BY_TEAM = 7855,
        DG_HORDE_CART_ASSAULT = 7887, // 1 default to show alliance points; 2 cart assault; 3 cart dropeed
        DG_HORDE_NODES = 8231,
        DG_HORDE_POINTS = 7881,
        DG_MIDDLE_MINE_ALLIANCE_ASSAULT = 7934,
        DG_MIDDLE_MINE_HORDE_ASSAULT = 7936,
        DG_MIDDLE_MINE_CAPTURED_BY_TEAM = 7932,
        DG_SHOW_GOBLIN_MINE_ICON = 7938,
        DG_SHOW_MIDDLE_MINE_ICON = 7939,
        DG_SHOW_PANDAREN_MINE_ICON = 7935,
        DRAENEI_RUINS_ALLIANCE_CONTROL = 2732,
        DRAENEI_RUINS_HORDE_CONTROL = 2733,
        DRAENEI_RUINS_UNCONTROL = 2731,
        FEL_REAVER_ALLIANCE_CONTROL = 2726,
        FEL_REAVER_HORDE_CONTROL = 2727,
        FEL_REAVER_UNCONTROL = 2725,
        HORDE_RESOUCES = 1777,
        HP_UI_TOWER_COUNT_A = 0x9ac,
        HP_UI_TOWER_COUNT_H = 0x9ae,
        HP_UI_TOWER_DISPLAY_A = 0x9ba,
        HP_UI_TOWER_DISPLAY_H = 0x9b9,
        HP_UI_TOWER_SLIDER_DISPLAY = 2473,
        HP_UI_TOWER_SLIDER_N = 2475,
        HP_UI_TOWER_SLIDER_POS = 2474,
        IsBattlegroundEnlistmentBonusActive = 13119,
        MAGE_TOWER_ALLIANCE_CONTROL = 2730,
        MAGE_TOWER_HORDE_CONTROL = 2729,
        MAGE_TOWER_UNCONTROL = 2728,
        MAX_TEAM_RESOUCES = 1780,
        NA_MAP_HALAA_ALLIANCE = 2673,
        NA_MAP_HALAA_HORDE = 2672,
        NA_MAP_HALAA_NEU_A = 2676,
        NA_MAP_HALAA_NEU_H = 2677,
        NA_MAP_HALAA_NEUTRAL = 2671,
        NA_MAP_WYVERN_EAST_A = 2661,
        NA_MAP_WYVERN_EAST_H = 2660,
        NA_MAP_WYVERN_EAST_NEU_A = 2659,
        NA_MAP_WYVERN_EAST_NEU_H = 2763,
        NA_MAP_WYVERN_NORTH_A = 2664,
        NA_MAP_WYVERN_NORTH_H = 2663,
        NA_MAP_WYVERN_NORTH_NEU_A = 2662,
        NA_MAP_WYVERN_NORTH_NEU_H = 2762,
        NA_MAP_WYVERN_SOUTH_A = 2669,
        NA_MAP_WYVERN_SOUTH_H = 2668,
        NA_MAP_WYVERN_SOUTH_NEU_A = 2670,
        NA_MAP_WYVERN_SOUTH_NEU_H = 2760,
        NA_MAP_WYVERN_WEST_A = 2666,
        NA_MAP_WYVERN_WEST_H = 2665,
        NA_MAP_WYVERN_WEST_NEU_A = 2667,
        NA_MAP_WYVERN_WEST_NEU_H = 2761,
        NA_UI_ALLIANCE_GUARDS_SHOW = 2502,
        NA_UI_GUARDS_LEFT = 2491,
        NA_UI_GUARDS_MAX = 2493,
        NA_UI_HORDE_GUARDS_SHOW = 2503,
        NA_UI_TOWER_SLIDER_DISPLAY = 2495,
        NA_UI_TOWER_SLIDER_N = 2497,
        NA_UI_TOWER_SLIDER_POS = 2494,
        NETHERSTORM_FLAG = 2757,
        NETHERSTORM_FLAG_STATE_ALLIANCE = 2769,
        NETHERSTORM_FLAG_STATE_HORDE = 2770,
        NETHERSTROM_FLAG_UI_ALLIANCE = 9808,
        NETHERSTROM_FLAG_UI_HORDE = 9809,
        OCCOPIED_BASES_ALLIANCE = 1779,
        OCCOPIED_BASES_HORDE = 1778,
        PROGRESS_BAR_PERCENT_GREY = 2720,                 //100 = empty (only grey), 0 = blue|red (no grey)
        PROGRESS_BAR_SHOW = 2718,                 //1 init, 0 druhy send - bez messagu, 1 = controlled aliance
        PROGRESS_BAR_STATUS = 2719,                 //50 init!, 48 ... hordak bere .. 33 .. 0 = full 100% hordacky, 100 = full alliance
        SI_GATHERED_A = 2313,
        SI_GATHERED_H = 2314,
        SI_SILITHYST_MAX = 2317,
        SSM_INIT_POINTS_ALLIANCE = 6441,
        SSM_INIT_POINTS_HORDE = 6443,
        SSM_MINE_CART_1 = 6439,
        SSM_MINE_CART_2 = 6440,
        SSM_MINE_CART_3 = 6442,
        SSM_MINE_CARTS_DISPLAY = 6436,
        SSM_POINTS_ALLIANCE = 6437,
        SSM_POINTS_HORDE = 6438,
        SSM_PROGRESS_BAR_PERCENT_GREY = 6877,                 //100 = empty (only grey), 0 = blue|red (no grey)
        SSM_PROGRESS_BAR_SHOW = 6875,                 //1 init, 0 druhy send - bez messagu, 1 = controlled aliance
        SSM_PROGRESS_BAR_STATUS = 6876,                 //50 init!, 48 ... hordak bere .. 33 .. 0 = full 100% hordacky, 100 = full alliance
        TF_UI_LOCKED_DISPLAY_ALLIANCE = 0xacf,
        TF_UI_LOCKED_DISPLAY_HORDE = 0xad0,
        TF_UI_LOCKED_DISPLAY_NEUTRAL = 0x9cc,
        TF_UI_LOCKED_TIME_HOURS = 0x9cd,
        TF_UI_LOCKED_TIME_MINUTES_FIRST_DIGIT = 0x9d0,
        TF_UI_LOCKED_TIME_MINUTES_SECOND_DIGIT = 0x9ce,
        TF_UI_TOWER_COUNT_A = 0xa3d,
        TF_UI_TOWER_COUNT_H = 0xa3e,
        TF_UI_TOWER_SLIDER_DISPLAY = 0xa3f,
        TF_UI_TOWER_SLIDER_N = 0xa40,
        TF_UI_TOWER_SLIDER_POS = 0xa41,
        TF_UI_TOWERS_CONTROLLED_DISPLAY = 0xa3c,
        WORLD_STATE_ALGALON_DESPAWN_TIMER = 4131,
        WORLD_STATE_ALGALON_TIMER_ENABLED = 4132,
        WORLD_STATE_BM = 2541,
        WORLD_STATE_BM_RIFT = 2784,
        WORLD_STATE_BM_SHIELD = 2540,
        WORLD_STATE_CENTRIFUGE_CONSTRUCT_AMOUNT = 3486,
        WORLD_STATE_CENTRIFUGE_CONSTRUCT_SHOW = 3524,
        WORLD_STATE_ENEMY = 2453,
        WORLD_STATE_ENEMYCOUNT = 2454,
        WORLD_STATE_HOR_WAVE_COUNT = 4882,
        WORLD_STATE_HOR_WAVES_ENABLED = 4884,
        WORLD_STATE_VH = 3816,
        WORLD_STATE_VH_PRISON_STATE = 3815,
        WORLD_STATE_VH_WAVE_COUNT = 3810,
        WORLD_STATE_WAVES = 2842,
        WORLDSTATE_ATTEMPTS_MAX = 4942,
        WORLDSTATE_ATTEMPTS_REMAINING = 4941,
        WORLDSTATE_CORPOREALITY_MATERIAL = 5049,
        WORLDSTATE_CORPOREALITY_TOGGLE = 5051,
        WORLDSTATE_CORPOREALITY_TWILIGHT = 5050,
        WORLDSTATE_CRATES_REVEALED = 3480,
        WORLDSTATE_EXECUTION_TIME = 4904,
        WORLDSTATE_FRAGMENTS_COLLECTED = 6025,
        WORLDSTATE_SHOW_ATTEMPTS = 4940,
        WORLDSTATE_SHOW_CRATES = 3479,
        WORLDSTATE_SHOW_FRAGMENTS = 6046,
        WORLDSTATE_SHOW_TIMER = 4903,
        WORLDSTATE_TIME_GUARDIAN = 3931,
        WORLDSTATE_TIME_GUARDIAN_SHOW = 3932,
        WORLDSTATE_WAVE_COUNT = 3504,
        WORLD_STATE_ALIVE_FOG_COUNT = 7889,
        WS_ARENA_BE_UNK2547 = 2547,
        WS_CAPTURE_NORTH_TOWER = 8469,
        WS_CAPTURE_SOUTH_TOWER = 8468,
        WS_ENABLE = 6489,
        WS_HEALER_COUNT = 6488,
        WS_KORKRON_CANNON_COUNT = 8373,
        WS_NORTH_TOWER = 8462,
        WS_SHOW_CAPTURE_NORTH_TOWER = 8548,
        WS_SHOW_CAPTURE_SOUTH_TOWER = 8546,
        WS_SHOW_KORKRON_CANNON = 8414,
        WS_SHOW_NORTH_TOWER = 8547,
        WS_SHOW_SOUTH_TOWER = 8545,
        WS_SOUTH_TOWER = 8461,
        WS_SSM_EAST_TRACK_SWITCH = 6467,
        WS_SSM_NORTH_TRACK_SWITCH = 6468,
        WS_TB_ALLIANCE_ATTACK = 5546,
        WS_TB_ALLIANCE_DEFENCE = 5385,
        WS_TB_BATTLE_TIMER_ENABLED = 5346,
        WS_TB_COUNTER_BUILDINGS = 5348,
        WS_TB_COUNTER_BUILDINGS_ENABLED = 5349,
        WS_TB_EAST_CAPTURE_POINT = 5428,
        WS_TB_EAST_SPIRE = 5433,
        WS_TB_HORDE_ATTACK = 5547,
        WS_TB_HORDE_DEFENCE = 5384,
        WS_TB_KEEP_ALLIANCE_DEFENCE = 5470,
        WS_TB_KEEP_HORDE_DEFENCE = 5469,
        WS_TB_NEXT_BATTLE_TIMER_ENABLED = 5387,
        WS_TB_SOUTH_CAPTURE_POINT = 5418,
        WS_TB_SOUTH_SPIRE = 5438,
        WS_TB_WEST_CAPTURE_POINT = 5423,
        WS_TB_WEST_SPIRE = 5443,
        ZM_MAP_ALLIANCE_FLAG_NOT_READY = 2656,
        ZM_MAP_ALLIANCE_FLAG_READY = 2655,
        ZM_MAP_GRAVEYARD_A = 2648,
        ZM_MAP_GRAVEYARD_H = 2649,
        ZM_MAP_GRAVEYARD_N = 2647,
        ZM_MAP_HORDE_FLAG_NOT_READY = 2657,
        ZM_MAP_HORDE_FLAG_READY = 2658,
        ZM_MAP_TOWER_EAST_A = 2650,
        ZM_MAP_TOWER_EAST_H = 2651,
        ZM_MAP_TOWER_EAST_N = 2652,
        ZM_MAP_TOWER_WEST_A = 2644,
        ZM_MAP_TOWER_WEST_H = 2645,
        ZM_MAP_TOWER_WEST_N = 2646,
        ZM_UI_TOWER_EAST_A = 2558,
        ZM_UI_TOWER_EAST_H = 2559,
        ZM_UI_TOWER_EAST_N = 2560,
        ZM_UI_TOWER_SLIDER_DISPLAY_E = 2533,
        ZM_UI_TOWER_SLIDER_DISPLAY_W = 2527,
        ZM_UI_TOWER_SLIDER_N_E = 2535,
        ZM_UI_TOWER_SLIDER_N_W = 2529,
        ZM_UI_TOWER_SLIDER_POS_E = 2534,
        ZM_UI_TOWER_SLIDER_POS_W = 2528,
        ZM_UI_TOWER_WEST_A = 2555,
        ZM_UI_TOWER_WEST_H = 2556,
        ZM_UI_TOWER_WEST_N = 2557,
        ZM_WORLDSTATE_UNK_1 = 2653,
        BG_SP_PURPLE_BOSS = 13198,
        BG_SP_GOLD_BOSS = 13199,
        BG_SP_SHOW_BAR = 13195,
        BG_SP_BAR_STATUS = 13196,
        BG_SP_GREY_BAR = 13197,
    };
}

namespace WorldStatesData
{
    enum Limits : uint16
    {
        Begin = 1000,
        End = 20000
    };

    enum Types : uint8
    {
        Custom = 0,
        World = 1,
        Weekly = 2,
        Event = 3,
        Map = 4,
        Zone = 5,
        Area = 6,
        Battlegound = 7,
        CapturePoint = 8,
        DestructibleObject = 9,
        Max
    };

    enum Flags : uint8
    {
        Initialized = 0,
        Active = 1,
        Saved = 2,
        Expired = 3,
        Updated = 4,
        Deleted = 7,
        Neutral = 8,
        Alliance = 9,
        Horde = 10,
        InitialState = 16,
        PassiaveAtCreate = 17,
        NotExpireable = 18,
        CustomFormat = 24,
        CustomGlobal = 25,
        CustomHidden = 26,
        CustomX = 27,
    };

    enum InitialValue : uint8
    {
        Remove = 0,
        Add = 1,
    };

}

struct WorldStateTemplate
{
    int32 Id = 0;
    int32 DefaultValue = 0;
    uint32 ScriptId = 0;

    std::unordered_set<uint32> MapIds;
    std::unordered_set<uint32> AreaIds;
};

using WorldStateValueContainer = std::unordered_map<int32 /*worldStateId*/, int32 /*value*/>;


#endif
