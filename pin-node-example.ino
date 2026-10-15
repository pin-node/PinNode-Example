/**************************************************************************
    This pinball code is distributed in the hope that it
    will be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
*/

#include "RPU_Config.h"
#include "RPU.h"
#include "DropTargets.h"
#include "ExampleMachineMEGA.h"
#include "OperatorMenus.h"
#include "AudioHandler.h"
#include "DisplayHandler.h"
#include "LampAnimations.h"
#ifndef RPU_OS_USE_PINNODE_LINK
#define PINNODE_NUM_ADJUSTMENTS 0
#define PINNODE_ADJUSTMENT_ITEM(index)
#endif
#ifdef RPU_OS_USE_PINNODE_LINK
// PinNode: the installed PinNode library (the link to the node and the PinNodeCtrl_ API)
// and the manifest constants (PinNodeGame.h, generated from pinnode.game.json).
#include <PinNode.h>
#include "PinNodeGame.h"
#endif
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
#include "ALB-Communication.h"
#endif
#include <EEPROM.h>


#define GAME_MAJOR_VERSION  2026
#define GAME_MINOR_VERSION  1
#define DEBUG_MESSAGES  1

#if (DEBUG_MESSAGES==1)
//#define DEBUG_SHOW_LOOPS_PER_SECOND
#endif

#ifdef RPU_SIMPLIFY_DISPLAY_FOR_7VOLUTION
#define DISPLAY_DASH_STYLE  DISPLAY_DASH_INTERMITTENT_FLASH
#else
#define DISPLAY_DASH_STYLE  DISPLAY_DASH_ROLLING_BLANK
#endif


/*********************************************************************

    Game specific code

*********************************************************************/

// MachineState
//  0 - Attract Mode
//  negative - no longer used
//  positive - game play
boolean InOperatorMenu = false;
char MachineState = 0;
boolean MachineStateChanged = true;
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 99
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_MATCH_MODE      110
#define MACHINE_STATE_DIAGNOSTICS     120

// Indices of EEPROM save locations
#define EEPROM_RPOS_INIT_PROOF_UL                 90

// CUSTOMIZATION TODO:
// This value needs to be set to a UNIQUE value for the 
// game code. You can pick any number, or do like I did and 
// ASCII encode some text (this value translates to "EM01")
#define RPOS_INIT_PROOF                           0x454D3031

#define EEPROM_BALL_SAVE_BYTE                     100
#define EEPROM_FREE_PLAY_BYTE                     101
#define EEPROM_TILT_WARNING_BYTE                  104
#define EEPROM_AWARD_OVERRIDE_BYTE                105
#define EEPROM_BALLS_OVERRIDE_BYTE                106
#define EEPROM_TOURNAMENT_SCORING_BYTE            107
#define EEPROM_SFX_VOLUME_BYTE                    108
#define EEPROM_MUSIC_VOLUME_BYTE                  109
#define EEPROM_SCROLLING_SCORES_BYTE              110
#define EEPROM_CALLOUTS_VOLUME_BYTE               111
#define EEPROM_CRB_HOLD_TIME                      118
#define EEPROM_TROUGH_EJECT_STRENGTH              131
#define EEPROM_SAUCER_EJECT_STRENGTH              134
#define EEPROM_SLINGSHOT_STRENGTH                 135 
#define EEPROM_POP_BUMPER_STRENGTH                136
#define EEPROM_GAME_RULES_SELECTION               137
#define EEPROM_MATCH_FEATURE_BYTE                 138
#define EEPROM_EXTRA_BALL_SCORE_UL                160
#define EEPROM_SPECIAL_SCORE_UL                   164


#define GAME_MODE_SKILL_SHOT                        1
#define GAME_MODE_UNSTRUCTURED_PLAY                 2
#define GAME_MODE_EXAMPLE_1                         3

#define SOUND_EFFECT_NONE                     0
#define SOUND_EFFECT_SPINNER                  2
#define SOUND_EFFECT_TILT                     3
#define SOUND_EFFECT_TILT_WARNING             4
#define SOUND_EFFECT_SCORE_TICK               5
#define SOUND_EFFECT_POP_BUMPER               6
#define SOUND_EFFECT_LEFT_SLING               7
#define SOUND_EFFECT_RIGHT_SLING              8
#define SOUND_EFFECT_GAME_OVER                21
#define SOUND_EFFECT_MATCH_SPIN               28
#define SOUND_EFFECT_DROP_TARGET_SOUND_1      30
#define SOUND_EFFECT_DROP_TARGET_COMPLETE     40
#define SOUND_EFFECT_BONUS_1                  81
#define SOUND_EFFECT_BONUS_2                  82
#define SOUND_EFFECT_BONUS_3                  83
#define SOUND_EFFECT_BONUS_4                  84
#define SOUND_EFFECT_BONUS_5                  85
#define SOUND_EFFECT_BONUS_6                  86
#define SOUND_EFFECT_BONUS_7                  87
#define SOUND_EFFECT_BONUS_8                  88
#define SOUND_EFFECT_STARTUP_1                100
#define SOUND_EFFECT_STARTUP_2                101

#define SOUND_EFFECT_COIN_DROP_1              105
#define SOUND_EFFECT_COIN_DROP_2              106
#define SOUND_EFFECT_COIN_DROP_3              107

#define SOUND_EFFECT_BACKGROUND_SONG_1                400
#define SOUND_EFFECT_BACKGROUND_SONG_2                401
#define SOUND_EFFECT_BACKGROUND_SONG_3                402
#define SOUND_EFFECT_BACKGROUND_SONG_4                403
#define SOUND_EFFECT_BACKGROUND_SONG_5                404
#define SOUND_EFFECT_BACKGROUND_SONG_6                405

#define SOUND_EFFECT_VP_GAME_RULES_EASY               423
#define SOUND_EFFECT_VP_GAME_RULES_MEDIUM             424
#define SOUND_EFFECT_VP_GAME_RULES_HARD               425

#define SOUND_EFFECT_RALLY_MUSIC_1                    450
#define SOUND_EFFECT_RALLY_MUSIC_2                    451
#define SOUND_EFFECT_RALLY_MUSIC_3                    452
#define SOUND_EFFECT_RALLY_MUSIC_4                    453
#define SOUND_EFFECT_RALLY_MUSIC_5                    454


// Game play status callouts
#define SOUND_EFFECT_VP_PLAYER_1_UP                   301
#define SOUND_EFFECT_VP_PLAYER_2_UP                   302
#define SOUND_EFFECT_VP_PLAYER_3_UP                   303
#define SOUND_EFFECT_VP_PLAYER_4_UP                   304
#define SOUND_EFFECT_VP_EXTRA_BALL                    305

#define SOUND_EFFECT_VP_ADD_PLAYER_1        306
#define SOUND_EFFECT_VP_ADD_PLAYER_2        (SOUND_EFFECT_VP_ADD_PLAYER_1+1)
#define SOUND_EFFECT_VP_ADD_PLAYER_3        (SOUND_EFFECT_VP_ADD_PLAYER_1+2)
#define SOUND_EFFECT_VP_ADD_PLAYER_4        (SOUND_EFFECT_VP_ADD_PLAYER_1+3)
#define SOUND_EFFECT_VP_SHOOT_AGAIN         310

#define SOUND_EFFECT_VP_BALL_SAVE                       326

#define SOUND_EFFECT_DIAG_START                   1900
#define SOUND_EFFECT_DIAG_CREDIT_RESET_BUTTON     1900
#define SOUND_EFFECT_DIAG_SELECTOR_SWITCH_ON      1901
#define SOUND_EFFECT_DIAG_SELECTOR_SWITCH_OFF     1902
#define SOUND_EFFECT_DIAG_STARTING_ORIGINAL_CODE  1903
#define SOUND_EFFECT_DIAG_STARTING_NEW_CODE       1904
#define SOUND_EFFECT_DIAG_ORIGINAL_CPU_DETECTED   1905
#define SOUND_EFFECT_DIAG_ORIGINAL_CPU_RUNNING    1906
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_U10         1907
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_U11         1908
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_1           1909
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_2           1910
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_3           1911
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_4           1912
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_5           1913
#define SOUND_EFFECT_DIAG_STARTING_DIAGNOSTICS    1914

// CUSTOMIZATION TODO:
// Decide how much bonus you want to be able to display (based on the lamps you
// have available)
#define MAX_DISPLAY_BONUS     21
#define TILT_WARNING_DEBOUNCE_TIME        1000
#define BALL_SAVE_GRACE_PERIOD            3000

/*********************************************************************

    Machine state and options

*********************************************************************/
byte Credits = 0;
byte BallSaveNumSeconds = 0;
byte MaximumCredits = 40;
byte BallsPerGame = 3;
byte ScoreAwardReplay = 0;
byte MusicVolume = 6;
byte SoundEffectsVolume = 8;
byte CalloutsVolume = 10;
byte ChuteCoinsInProgress[3];
byte TotalBallsLoaded = 1;
byte TimeRequiredToResetGame = 1;
boolean FreePlayMode = false;
boolean MatchEnabled = true;
boolean HighScoreReplay = true;
boolean MatchFeature = true;
boolean TournamentScoring = false;
boolean ScrollingScores = true;
boolean GIInOriginal = true;
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
unsigned long CurrentTime = 0;
unsigned long HighScore = 0;
unsigned long AwardScores[3];
unsigned long CreditResetPressStarted = 0;
unsigned long OperatorSwitchPressStarted = 0;

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
unsigned long ALBWatchdogResetLastSent;
#endif

#define NUM_CPC_PAIRS 9
boolean CPCSelectionsHaveBeenRead = false;
byte CPCPairs[NUM_CPC_PAIRS][2] = {
  {1, 5},
  {1, 4},
  {1, 3},
  {1, 2},
  {1, 1},
  {2, 3},
  {2, 1},
  {3, 1},
  {4, 1}
};
byte CPCSelection[3];

AudioHandler  Audio;
OperatorMenus Menus;

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
AccessoryLampBoard TopperALB;
#endif


/*********************************************************************

    Game State

*********************************************************************/
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
byte NumberOfBallsLocked;
byte NumberOfBallsInPlay;
byte Bonus[RPU_NUMBER_OF_PLAYERS_ALLOWED];
byte BonusX[RPU_NUMBER_OF_PLAYERS_ALLOWED];
byte GameMode = GAME_MODE_SKILL_SHOT;
byte LastGameMode = 0;
byte MaxTiltWarnings = 2;
byte NumTiltWarnings = 0;
byte CurrentAchievements[RPU_NUMBER_OF_PLAYERS_ALLOWED];
byte NumberOfBallSavesRemaining;

boolean SamePlayerShootsAgain = false;
boolean BallSaveUsed = false;
boolean SpecialAvailable = false;
boolean SpecialCollected = false;
boolean TimersPaused = true;

unsigned long CurrentScores[RPU_NUMBER_OF_PLAYERS_ALLOWED];
unsigned long BallFirstSwitchHitTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long GameModeStartTime = 0;
unsigned long GameModeEndTime = 0;
unsigned long LastTiltWarningTime;
unsigned long PlayfieldMultiplier;
unsigned long LastTimeThroughLoop;
unsigned long LastSwitchHitTime;
unsigned long BallSaveEndTime;


/*********************************************************************

    Game Specific State Variables

*********************************************************************/
byte ExtraBallAvailable[RPU_NUMBER_OF_PLAYERS_ALLOWED];
byte GameRulesSelection;
// CUSTOMIZATION TODO:
// if a solenoid is going to have adjustable strength, create variables for them
byte BallServeSolenoidStrength;  
byte SaucerSolenoidStrength;

int PopBumperProgress;
int SpinnerProgress[RPU_NUMBER_OF_PLAYERS_ALLOWED];

#define GAME_RULES_EASY         1
#define GAME_RULES_MEDIUM       2
#define GAME_RULES_HARD         3
#define GAME_RULES_PROGRESSIVE  4
#define GAME_RULES_CUSTOM       5

unsigned long PlayfieldMultiplierTimeLeft;
unsigned long BonusChanged;
unsigned long BonusXAnimationStart;
unsigned long LastTimePopHit;
unsigned long LastTimeBallServed;
unsigned long LastSpinnerHitTime;
unsigned long LastTimeSaucerSeen;

#ifdef MACHINE_BALLY_STAR_TREK
// the pulse lengths the Star Trek games use
#define DROP_TARGET_RESET_STRENGTH    12
#define KNOCKER_SOLENOID_STRENGTH     3
#else
#define DROP_TARGET_RESET_STRENGTH    10
#define KNOCKER_SOLENOID_STRENGTH     5
#endif

// CUSTOMIZATION TODO:
// If your game has drop targets, set up objects here. Be sure to
// check out the types of target banks supported in DropTargtes.h
#ifdef MACHINE_BALLY_STAR_TREK
DropTargetBank LeftDropTargets(4, 1, DROP_TARGET_TYPE_BLY_1, DROP_TARGET_RESET_STRENGTH);
#else
DropTargetBank LeftDropTargets(3, 1, DROP_TARGET_TYPE_BLY_1, DROP_TARGET_RESET_STRENGTH);
DropTargetBank RightDropTargets(3, 1, DROP_TARGET_TYPE_BLY_1, DROP_TARGET_RESET_STRENGTH);
#endif

byte PlayerUpLamps[4] = {LAMP_HEAD_PLAYER_1_UP, LAMP_HEAD_PLAYER_2_UP, LAMP_HEAD_PLAYER_3_UP, LAMP_HEAD_PLAYER_4_UP};


/******************************************************

   Adjustments Serialization

*/
void SetAllParameterDefaults() {

  // In the event that the EEPROM has not been initialized,
  // these are the values that will be used
  HighScore = 10000;
  Credits = 4;
  FreePlayMode = false;
  BallSaveNumSeconds = 15;
  MusicVolume = 10;
  SoundEffectsVolume = 10;
  CalloutsVolume = 10;
  AwardScores[0] = 1000000;
  AwardScores[1] = 3000000;
  AwardScores[2] = 5000000;
  TournamentScoring = false;
  MaxTiltWarnings = 2;
  ScoreAwardReplay = 0x07;
  BallsPerGame = 3;
  ScrollingScores = true;
  MatchFeature = true;
  ExtraBallValue = 20000;
  SpecialValue = 40000;
  TimeRequiredToResetGame = 2;
  CPCSelection[0] = 4;
  CPCSelection[1] = 4;
  CPCSelection[2] = 4;

  // EASY / MEDIUM / HARD rules
  GameRulesSelection = GAME_RULES_MEDIUM;

  // CUSTOMIZATION TODO:
  // Set up default/reset values for any game
  // parameters that can be controlled through 
  // the operator menus
#ifdef MACHINE_BALLY_STAR_TREK
  BallServeSolenoidStrength = 4;    // the outhole kicker serves the ball
#else
  BallServeSolenoidStrength = 10;
#endif
  SaucerSolenoidStrength = 5;
}


// CUSTOMIZATION TODO:
// Load in defaults for easy / medium / hard rules
boolean LoadRuleDefaults(byte ruleLevel) {
  // This function just puts the rules in RAM.
  // It's up to the caller to ensure that they're 
  // written to EEPROM when desired (with WriteParameters)
  if (ruleLevel==GAME_RULES_EASY) {
    BallSaveNumSeconds = 20;
    // Game rules variables initialized to EASY here:
  } else if (ruleLevel==GAME_RULES_MEDIUM) {
    BallSaveNumSeconds = 10;
    // Game rules variables initialized to MEDIUM here:
  } else if (ruleLevel==GAME_RULES_HARD) {
    BallSaveNumSeconds = 0;
    // Game rules variables initialized to HARD here:
  } else {
    return false;
  }

  return true;
}


void WriteParameters(boolean onlyWriteRulesParameters = true) {
  if (!onlyWriteRulesParameters) {
    RPU_WriteULToEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, HighScore);
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    RPU_WriteByteToEEProm(RPU_CPC_CHUTE_1_SELECTION_BYTE, CPCSelection[0]);
    RPU_WriteByteToEEProm(RPU_CPC_CHUTE_2_SELECTION_BYTE, CPCSelection[1]);
    RPU_WriteByteToEEProm(RPU_CPC_CHUTE_3_SELECTION_BYTE, CPCSelection[2]);
        
    RPU_WriteByteToEEProm(EEPROM_FREE_PLAY_BYTE, FreePlayMode);
    RPU_WriteByteToEEProm(EEPROM_MUSIC_VOLUME_BYTE, MusicVolume);
    RPU_WriteByteToEEProm(EEPROM_SFX_VOLUME_BYTE, SoundEffectsVolume);
    RPU_WriteByteToEEProm(EEPROM_CALLOUTS_VOLUME_BYTE, CalloutsVolume);

    RPU_WriteULToEEProm(RPU_AWARD_SCORE_1_EEPROM_START_BYTE, AwardScores[0]);
    RPU_WriteULToEEProm(RPU_AWARD_SCORE_2_EEPROM_START_BYTE, AwardScores[1]);
    RPU_WriteULToEEProm(RPU_AWARD_SCORE_3_EEPROM_START_BYTE, AwardScores[2]);

    RPU_WriteByteToEEProm(EEPROM_TOURNAMENT_SCORING_BYTE, TournamentScoring);
    RPU_WriteByteToEEProm(EEPROM_AWARD_OVERRIDE_BYTE, ScoreAwardReplay);
    RPU_WriteByteToEEProm(EEPROM_BALLS_OVERRIDE_BYTE, BallsPerGame);
    RPU_WriteByteToEEProm(EEPROM_SCROLLING_SCORES_BYTE, ScrollingScores);
    RPU_WriteByteToEEProm(EEPROM_MATCH_FEATURE_BYTE, MatchFeature);
    
    RPU_WriteULToEEProm(EEPROM_EXTRA_BALL_SCORE_UL, ExtraBallValue);
    RPU_WriteULToEEProm(EEPROM_SPECIAL_SCORE_UL, SpecialValue);
    RPU_WriteByteToEEProm(EEPROM_CRB_HOLD_TIME, TimeRequiredToResetGame);

    // Set baseline for audits
    // (the coin counters are 4 bytes each: writing one byte leaves 0xFFFFFF00 in a fresh EEPROM)
    RPU_WriteULToEEProm(RPU_CHUTE_1_COINS_START_BYTE, 0);
    RPU_WriteULToEEProm(RPU_CHUTE_2_COINS_START_BYTE, 0);
    RPU_WriteULToEEProm(RPU_CHUTE_3_COINS_START_BYTE, 0);
    RPU_WriteByteToEEProm(EEPROM_GAME_RULES_SELECTION, GameRulesSelection);
    RPU_WriteULToEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, 0);
    RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, 0);
    RPU_WriteULToEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, 0);
  }

  RPU_WriteByteToEEProm(EEPROM_BALL_SAVE_BYTE, BallSaveNumSeconds);
  RPU_WriteByteToEEProm(EEPROM_TILT_WARNING_BYTE, MaxTiltWarnings); 
  
  // CUSTOMIZATION TODO:
  // Write any custom parameters to EEPROM here
  RPU_WriteByteToEEProm(EEPROM_TROUGH_EJECT_STRENGTH, BallServeSolenoidStrength);
  RPU_WriteByteToEEProm(EEPROM_SAUCER_EJECT_STRENGTH, SaucerSolenoidStrength);
}

void ReadStoredParameters() {
  for (byte count = 0; count < 3; count++) {
    ChuteCoinsInProgress[count] = 0;
  }

  // The first time the EEPROM has been written with good values for this game,
  // the EEPROM_RPOS_INIT_PROOF_UL will be written to a known state (RPOS_INIT_PROOF)
  // if that value hasn't been written, then we load defaults and save them to EEPROM.
  // This should only happen the first time a device is run with this game code.
  unsigned long RPUProofValue = RPU_ReadULFromEEProm(EEPROM_RPOS_INIT_PROOF_UL, 0);
  if (RPUProofValue!=RPOS_INIT_PROOF) {
    // Doesn't look like this memory has been initialized
    RPU_WriteULToEEProm(EEPROM_RPOS_INIT_PROOF_UL, RPOS_INIT_PROOF);
    SetAllParameterDefaults();
    WriteParameters(false);
  } else {

    // Read machine settings
    HighScore = RPU_ReadULFromEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, 10000);
    Credits = RPU_ReadByteFromEEProm(RPU_CREDITS_EEPROM_BYTE);
    if (Credits > MaximumCredits) Credits = MaximumCredits;
  
    FreePlayMode = ReadSetting(EEPROM_FREE_PLAY_BYTE, false, true);
    MusicVolume = ReadSetting(EEPROM_MUSIC_VOLUME_BYTE, 10, 10);
    SoundEffectsVolume = ReadSetting(EEPROM_SFX_VOLUME_BYTE, 10, 10);
    CalloutsVolume = ReadSetting(EEPROM_CALLOUTS_VOLUME_BYTE, 10, 10);
    Audio.SetMusicVolume(MusicVolume);
    Audio.SetSoundFXVolume(SoundEffectsVolume);
    Audio.SetNotificationsVolume(CalloutsVolume);

    AwardScores[0] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_1_EEPROM_START_BYTE);
    AwardScores[1] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_2_EEPROM_START_BYTE);
    AwardScores[2] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_3_EEPROM_START_BYTE);
  
    TournamentScoring = ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, false, true);
    ScoreAwardReplay = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 0x03, 0x07);
    BallsPerGame = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 3, 10);
    ScrollingScores = ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, true, true);
    MatchFeature = ReadSetting(EEPROM_MATCH_FEATURE_BYTE, true, true);

    CPCSelection[0] = ReadSetting(RPU_CPC_CHUTE_1_SELECTION_BYTE, 4, 8);
    CPCSelection[1] = ReadSetting(RPU_CPC_CHUTE_2_SELECTION_BYTE, 4, 8);
    CPCSelection[2] = ReadSetting(RPU_CPC_CHUTE_3_SELECTION_BYTE, 4, 8);
    CPCSelectionsHaveBeenRead = true;

    ExtraBallValue = RPU_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_UL);
    if (ExtraBallValue % 1000 || ExtraBallValue > 1000000) ExtraBallValue = 20000;
  
    SpecialValue = RPU_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_UL);
    if (SpecialValue % 1000 || SpecialValue > 1000000) SpecialValue = 40000;
  
    TimeRequiredToResetGame = ReadSetting(EEPROM_CRB_HOLD_TIME, 1, 99);
    if (TimeRequiredToResetGame > 3 && TimeRequiredToResetGame != 99) TimeRequiredToResetGame = 1;
    
    // Read game rules
    GameRulesSelection = ReadSetting(EEPROM_GAME_RULES_SELECTION, GAME_RULES_MEDIUM, GAME_RULES_CUSTOM);

    BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15, 20);  
    MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2, 3);
    
#ifdef MACHINE_BALLY_STAR_TREK
    BallServeSolenoidStrength = ReadSetting(EEPROM_TROUGH_EJECT_STRENGTH, 4, 10);
    if (BallServeSolenoidStrength < 3) BallServeSolenoidStrength = 4;
#else
    BallServeSolenoidStrength = ReadSetting(EEPROM_TROUGH_EJECT_STRENGTH, 10, 60);
#endif
    SaucerSolenoidStrength = ReadSetting(EEPROM_SAUCER_EJECT_STRENGTH, 5, 15);
  }
}



byte GetCPCSelection(byte chuteNumber) {
  if (chuteNumber>2) return 0xFF;

  if (CPCSelectionsHaveBeenRead==false) {
    CPCSelection[0] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_1_SELECTION_BYTE);
    if (CPCSelection[0]>=NUM_CPC_PAIRS) {
      CPCSelection[0] = 4;
      RPU_WriteByteToEEProm(RPU_CPC_CHUTE_1_SELECTION_BYTE, 4);
    }
    CPCSelection[1] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_2_SELECTION_BYTE);  
    if (CPCSelection[1]>=NUM_CPC_PAIRS) {
      CPCSelection[1] = 4;
      RPU_WriteByteToEEProm(RPU_CPC_CHUTE_2_SELECTION_BYTE, 4);
    }
    CPCSelection[2] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_3_SELECTION_BYTE);  
    if (CPCSelection[2]>=NUM_CPC_PAIRS) {
      CPCSelection[2] = 4;
      RPU_WriteByteToEEProm(RPU_CPC_CHUTE_3_SELECTION_BYTE, 4);
    }
    CPCSelectionsHaveBeenRead = true;
  }
  
  return CPCSelection[chuteNumber];
}


byte GetCPCCoins(byte cpcSelection) {
  if (cpcSelection>=NUM_CPC_PAIRS) return 1;
  return CPCPairs[cpcSelection][0];
}


byte GetCPCCredits(byte cpcSelection) {
  if (cpcSelection>=NUM_CPC_PAIRS) return 1;
  return CPCPairs[cpcSelection][1];
}


void QueueDIAGNotification(unsigned short notificationNum) {
  // This is optional, but the machine can play an audio message at boot
  // time to indicate any errors and whether it's going to boot to original
  // or new code.
  //Audio.QueuePrioritizedNotification(notificationNum, 0, 10, CurrentTime);
  (void)notificationNum;
}

// CUSTOMIZATION TODO:
// This function only needs to be updated if the display
// numbers (in test mode) don't match the indices.
// In most games, the operator friendly numbers are just
// the display numbers + 1
// I'm doing this as a function instead of an array because 
// memory is short and spending 44 bytes on a converstion array
// seems wasteful when the board has tons and tons of code space.
// There's a way to store this data in code space and then convert
// it when needed, but that's slow compared to this (ugly) method.
byte LampConvertDisplayNumberToIndex(byte displayNumber) {

  if (displayNumber<60) return displayNumber + 1;
  return OPERATOR_MENU_VALUE_OUT_OF_RANGE;
}


// CUSTOMIZATION TODO:
// Fill in the "operator friendly" solenoid number in the "case" 
// (this is the number reported in the game manual)
// and then the hardware index in the return (from your defines)
#ifdef MACHINE_BALLY_STAR_TREK
// Star Trek: the test number is the coil's own number (as in the RPUMachines name list)
unsigned short SolenoidConvertDisplayNumberToIndex(byte displayNumber) {
  if (displayNumber >= SOL_KNOCKER && displayNumber <= SOL_DROP_TARGET_RESET) return displayNumber;
  if (displayNumber == 15) return 0x4000; // Flipper Mute
  if (displayNumber <= 19) return OPERATOR_MENU_VALUE_UNUSED;
  return OPERATOR_MENU_VALUE_OUT_OF_RANGE;
}

byte SolenoidConvertDisplayNumberToTestStrength(byte displayNumber) {
  switch (displayNumber) {
    case SOL_KNOCKER: return KNOCKER_SOLENOID_STRENGTH;
    case SOL_OUTHOLE: return BallServeSolenoidStrength;
    case SOL_SAUCER: return SaucerSolenoidStrength;
    case SOL_LEFT_BUMPER: case SOL_RIGHT_BUMPER: case SOL_CENTER_BUMPER: return 3;
    case SOL_LEFT_SLINGSHOT: case SOL_RIGHT_SLINGSHOT: return 4;
    case SOL_DROP_TARGET_RESET: return DROP_TARGET_RESET_STRENGTH;
    case 15: return 4;
  }
  if (displayNumber <= 19) return OPERATOR_MENU_VALUE_UNUSED;
  return OPERATOR_MENU_VALUE_OUT_OF_RANGE;
}
#else
unsigned short SolenoidConvertDisplayNumberToIndex(byte displayNumber) {
  switch (displayNumber) {
    case  0: return OPERATOR_MENU_VALUE_UNUSED;
    case  1: return OPERATOR_MENU_VALUE_UNUSED;
    case  2: return OPERATOR_MENU_VALUE_UNUSED;
    case  3: return OPERATOR_MENU_VALUE_UNUSED;
    case  4: return SOL_POP_BUMPER;
    case  5: return SOL_DROP_BANK_R_RESET;
    case  6: return SOL_DROP_BANK_L_RESET;
    case  7: return SOL_KNOCKER;
    case  8: return SOL_SAUCER;
    case  9: return SOL_LEFT_SLING;
    case 10: return SOL_RIGHT_SLING;
    case 11: return OPERATOR_MENU_VALUE_UNUSED;
    case 12: return OPERATOR_MENU_VALUE_UNUSED;
    case 13: return SOL_SERVE_BALL;
    case 14: return OPERATOR_MENU_VALUE_UNUSED;
    case 15: return 0x4000; // Flipper Mute
    case 16: return OPERATOR_MENU_VALUE_UNUSED;
    case 17: return OPERATOR_MENU_VALUE_UNUSED; // unused 0x1000
    case 18: return OPERATOR_MENU_VALUE_UNUSED; // unused 0x8000
    case 19: return 0x2000; // Topper
    default: return OPERATOR_MENU_VALUE_OUT_OF_RANGE;
  }
}

// CUSTOMIZATION TODO:
// In the operator menu's test mode, these are the strengths
// that will be used 
byte SolenoidConvertDisplayNumberToTestStrength(byte displayNumber) {
  switch (displayNumber) {
    case  0: return OPERATOR_MENU_VALUE_UNUSED;
    case  1: return 4;
    case  2: return 4;
    case  3: return 4;
    case  4: return 4;
    case  5: return 4;
    case  6: return 4;
    case  7: return 4;
    case  8: return SaucerSolenoidStrength;
    case  9: return 4;
    case 10: return 4;
    case 11: return 4;
    case 12: return 4;
    case 13: return BallServeSolenoidStrength;
    case 14: return 4;
    case 15: return 4;
    case 16: return OPERATOR_MENU_VALUE_UNUSED;
    case 17: return OPERATOR_MENU_VALUE_UNUSED;
    case 18: return OPERATOR_MENU_VALUE_UNUSED;
    case 19: return OPERATOR_MENU_VALUE_UNUSED; 
    default: return OPERATOR_MENU_VALUE_OUT_OF_RANGE;
  }
}
#endif




#ifdef RPU_OS_USE_PINNODE_LINK
////////////////////////////////////////////////////////////////////////////
//
//  PinNode tables
//    The app's machine options are the operator adjustments below: the same
//    EEPROM bytes the operator menu edits. The position in each table is the
//    index in pinnode.game.json (SET_* / AUD_* in PinNodeGame.h), so keep the
//    two in step and never reorder a released index. A difficulty setting
//    (the manifest's "difficulty") can't be changed while a tournament holds
//    the machine's settings.
//
////////////////////////////////////////////////////////////////////////////
const PinNodeSetting PinNodeSettingsTable[] = {
  // eeprom                              size  min  max        scale liveSafe difficulty step
  {EEPROM_FREE_PLAY_BYTE,                  1,  0,   1,          1,  false, false, 0},  // SET_FREE_PLAY
  {EEPROM_BALL_SAVE_BYTE,                  1,  0,   20,         1,  false, true , 0},  // SET_BALL_SAVE
  {EEPROM_TILT_WARNING_BYTE,               1,  0,   2,          1,  false, true , 0},  // SET_TILT_WARNINGS
  {EEPROM_MUSIC_VOLUME_BYTE,               1,  0,   10,         1,  true , false, 0},  // SET_MUSIC_VOLUME
  {EEPROM_SFX_VOLUME_BYTE,                 1,  0,   10,         1,  true , false, 0},  // SET_SFX_VOLUME
  {EEPROM_CALLOUTS_VOLUME_BYTE,            1,  0,   10,         1,  true , false, 0},  // SET_CALLOUTS_VOLUME
  {EEPROM_BALLS_OVERRIDE_BYTE,             1,  3,   10,         1,  false, true , 0},  // SET_BALLS_PER_GAME
  {EEPROM_TOURNAMENT_SCORING_BYTE,         1,  0,   1,          1,  false, true , 0},  // SET_TOURNAMENT_SCORING
  {EEPROM_EXTRA_BALL_SCORE_UL,             4,  0,   99999990,   1,  false, true , 0},  // SET_EXTRA_BALL_VALUE
  {EEPROM_SPECIAL_SCORE_UL,                4,  0,   99999990,   1,  false, true , 0},  // SET_SPECIAL_VALUE
  {EEPROM_CRB_HOLD_TIME,                   1,  0,   99,         1,  false, false, 0},  // SET_RESET_HOLD_TIME
  {RPU_AWARD_SCORE_1_EEPROM_START_BYTE,    4,  0,   99999990,   1,  false, false, 0},  // SET_REPLAY_SCORE_1
  {RPU_AWARD_SCORE_2_EEPROM_START_BYTE,    4,  0,   99999990,   1,  false, false, 0},  // SET_REPLAY_SCORE_2
  {RPU_AWARD_SCORE_3_EEPROM_START_BYTE,    4,  0,   99999990,   1,  false, false, 0},  // SET_REPLAY_SCORE_3
  {EEPROM_AWARD_OVERRIDE_BYTE,             1,  0,   7,          1,  false, false, 0},  // SET_SCORE_AWARD_REPLAY
  {EEPROM_SCROLLING_SCORES_BYTE,           1,  0,   1,          1,  true , false, 0},  // SET_SCROLLING_SCORES
  {RPU_CPC_CHUTE_1_SELECTION_BYTE,         1,  0,   8,          1,  false, false, 0},  // SET_COINS_PER_CREDIT_1
  {RPU_CPC_CHUTE_2_SELECTION_BYTE,         1,  0,   8,          1,  false, false, 0},  // SET_COINS_PER_CREDIT_2
  {RPU_CPC_CHUTE_3_SELECTION_BYTE,         1,  0,   8,          1,  false, false, 0},  // SET_COINS_PER_CREDIT_3
  {EEPROM_MATCH_FEATURE_BYTE,              1,  0,   1,          1,  false, false, 0},  // SET_MATCH_FEATURE
  {EEPROM_GAME_RULES_SELECTION,            1,  1,   5,          1,  false, true , 0},  // SET_GAME_RULES
#ifdef MACHINE_BALLY_STAR_TREK
  {EEPROM_TROUGH_EJECT_STRENGTH,           1,  3,   10,         1,  false, false, 0},  // SET_TROUGH_EJECT_STRENGTH
#else
  {EEPROM_TROUGH_EJECT_STRENGTH,           1,  10,  60,         1,  false, false, 0},  // SET_TROUGH_EJECT_STRENGTH
#endif
  {EEPROM_SAUCER_EJECT_STRENGTH,           1,  5,   15,         1,  false, false, 0},  // SET_SAUCER_EJECT_STRENGTH
  {PINNODE_EEPROM_APP_STARTS,              1,  0,   1,          1,  false, false, 0},  // SET_APP_STARTS
};

const PinNodeAudit PinNodeAuditsTable[] = {
  {RPU_TOTAL_PLAYS_EEPROM_START_BYTE,   4},  // AUD_TOTAL_PLAYS
  {RPU_CHUTE_1_COINS_START_BYTE,        4},  // AUD_CHUTE_1_COINS
  {RPU_CHUTE_2_COINS_START_BYTE,        4},  // AUD_CHUTE_2_COINS
  {RPU_CHUTE_3_COINS_START_BYTE,        4},  // AUD_CHUTE_3_COINS
  {RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, 4},  // AUD_TOTAL_REPLAYS
  {RPU_TOTAL_HISCORE_BEATEN_START_BYTE, 4},  // AUD_HIGH_SCORE_BEATEN
};

// The manifest hash in PinNodeGame.h is 16 hex characters; HELLO carries it as 8 bytes
static void PinNodeManifestBytes(byte *hashBytes) {
  const char *hex = PINNODE_MANIFEST_HASH;
  for (byte count = 0; count < 16; count++) {
    char c = hex[count];
    byte nibble = (c >= 'a') ? (c - 'a' + 10) : (c >= 'A') ? (c - 'A' + 10) : (c - '0');
    if (count % 2 == 0) hashBytes[count / 2] = nibble << 4;
    else hashBytes[count / 2] |= nibble;
  }
}

// Score at the start of the ball in play, for STAT_BEST_BALL
unsigned long PinNodeScoreAtBallStart = 0;

// The app changed a setting: it's in EEPROM already, so re-read as after the operator menu
static void PinNodeSettingsApplied() {
  byte musicVolumeBefore = MusicVolume;
  CPCSelectionsHaveBeenRead = false;
  ReadStoredParameters();

  // A new music volume only reaches the WAV Trigger when a track starts, so the music
  // that's playing is started again at the new volume (or stopped, at volume 0).
  if (MusicVolume != musicVolumeBefore && Audio.GetBackgroundSong() != BACKGROUND_TRACK_NONE) {
    if (MusicVolume == 0) Audio.StopAllMusic();
    else Audio.PlayBackgroundSong(Audio.GetBackgroundSong());
  }
}


////////////////////////////////////////////////////////////////////////////
//
//  Operator actions from the phone
//    The tests and one-shot operations of the operator menu, as plain
//    functions a request from the app can start and stop. They don't touch
//    the coin-door menus (OperatorMenus).
//      - a test runs only in MACHINE_STATE_DIAGNOSTICS, which the phone enters
//        (from attract only) and leaves with the two diagnostics actions;
//      - a one-shot runs in attract or diagnostics;
//      - nothing runs during a game, or while the coin-door menu is open.
//    The position in PinNodeActionsTable is the index in the manifest's
//    actions[] list (the ACT_ constants): keep the two in step.
//
////////////////////////////////////////////////////////////////////////////
// The action numbers are the ACT_ constants of PinNodeGame.h (from the manifest)
#define REMOTE_TEST_NONE            0xFF

const PinNodeAction PinNodeActionsTable[] = {
  {PINNODE_ACTION_TEST | PINNODE_ACTION_LOCAL},                           // lamp-test
  {PINNODE_ACTION_TEST | PINNODE_ACTION_LOCAL},                           // display-test
  {PINNODE_ACTION_TEST | PINNODE_ACTION_LOCAL},                           // solenoid-test
  {PINNODE_ACTION_TEST | PINNODE_ACTION_LOCAL},                           // switch-test
  {PINNODE_ACTION_TEST | PINNODE_ACTION_LOCAL},                           // sound-test
  {PINNODE_ACTION_LOCAL},                                                 // eject-balls
  {PINNODE_ACTION_CONFIRM},                                               // clear-audits
  {PINNODE_ACTION_CONFIRM | PINNODE_ACTION_LOCK},                         // factory-defaults
  {PINNODE_ACTION_CONFIRM | PINNODE_ACTION_LOCK},                         // preset-easy
  {PINNODE_ACTION_CONFIRM | PINNODE_ACTION_LOCK},                         // preset-medium
  {PINNODE_ACTION_CONFIRM | PINNODE_ACTION_LOCK},                         // preset-hard
  {PINNODE_ACTION_CONFIRM | PINNODE_ACTION_LOCK},                         // preset-progressive
  {PINNODE_ACTION_LOCAL | PINNODE_ACTION_ENTER_DIAG},                     // enter-diagnostics
  {PINNODE_ACTION_LOCAL | PINNODE_ACTION_EXIT_DIAG},                      // exit-diagnostics
  {PINNODE_ACTION_CONFIRM},                                               // clear-credits
};

#define SOLENOID_TEST_PERIOD_MS           2000
#define SWITCH_TEST_NUM_SWITCHES          40      // the switches polled for open/closed changes

byte RemoteTest = REMOTE_TEST_NONE;            // the test running now
uint16_t RemoteTestReqId = 0;
long RemoteTestArg = 0;
long RemoteTestValue = 0;                 // what the app shows: lamp, digit, solenoid, closures, track
unsigned long RemoteTestLastStep = 0;
byte RemoteSwitchStates[(SWITCH_TEST_NUM_SWITCHES + 7) / 8];
boolean RemoteSwitchTestFirstPass = false;


void StopTest() {
  if (RemoteTest == REMOTE_TEST_NONE) return;
  RPU_TurnOffAllLamps();
  for (byte count = 0; count < RPU_NUMBER_OF_PLAYER_DISPLAYS; count++) RPU_SetDisplayBlank(count, 0x00);
  Audio.StopAllAudio();
  RemoteTest = REMOTE_TEST_NONE;
}

// lamp: 0 = all lamps flashing, 1-60 = that one lamp (lamp index + 1)
void StartLampTest(long lamp) {
  RPU_TurnOffAllLamps();
  if (lamp <= 0 || lamp > RPU_MAX_LAMPS) {
    for (int count = 0; count < RPU_MAX_LAMPS; count++) RPU_SetLampState(count, 1, 0, 500);
    lamp = 0;
  } else {
    RPU_SetLampState(lamp - 1, 1);
  }
  RemoteTest = ACT_LAMP_TEST;
  RemoteTestValue = lamp;
}

// digit: 0 = every digit counting 0-9, 1-6 (or 7) = only that digit
void StartDisplayTest(long digit) {
  if (digit < 0 || digit > RPU_OS_NUM_DIGITS) digit = 0;
  RemoteTest = ACT_DISPLAY_TEST;
  RemoteTestValue = digit;
}

// solenoid: the operator menu's solenoid test number; 0 = each one in turn.
// It fires every two seconds until the test is stopped.
void StartSolenoidTest(long solenoid) {
  if (solenoid < 0 || solenoid > 19) solenoid = 0;
  RemoteTest = ACT_SOLENOID_TEST;
  RemoteTestValue = solenoid;
  RemoteTestLastStep = 0;
}

void StartSwitchTest() {
  for (byte count = 0; count < sizeof(RemoteSwitchStates); count++) RemoteSwitchStates[count] = 0;
  RemoteTest = ACT_SWITCH_TEST;
  RemoteTestValue = 0;      // closures seen since the test started
  RemoteSwitchTestFirstPass = true;   // switches already closed are reported, but not counted
}

// track: the WAV Trigger track number, played once
void PlaySoundTest(long track) {
  Audio.StopAllAudio();
  if (track > 0) Audio.PlaySound((unsigned short)track, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
  RemoteTest = ACT_SOUND_TEST;
  RemoteTestValue = track;
}

void EjectAllBalls() {
  if (CountBallsInTrough()) RPU_PushToTimedSolenoidStack(SOL_SERVE_BALL, BallServeSolenoidStrength, CurrentTime + 100, true);
  if (RPU_ReadSingleSwitchState(SW_SAUCER)) RPU_PushToTimedSolenoidStack(SOL_SAUCER, SaucerSolenoidStrength, CurrentTime + 600, true);
}

// The values go to the node first (an audit snapshot), so what they were is on record.
void ClearAudits() {
  PinNodeCtrl_SendAuditsNow();
  RPU_WriteULToEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, 0);
  RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, 0);
  RPU_WriteULToEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, 0);
  RPU_WriteULToEEProm(RPU_CHUTE_1_COINS_START_BYTE, 0);
  RPU_WriteULToEEProm(RPU_CHUTE_2_COINS_START_BYTE, 0);
  RPU_WriteULToEEProm(RPU_CHUTE_3_COINS_START_BYTE, 0);
  PinNodeCtrl_AuditsChanged();
}

// The credits go to zero; the count before goes to the node first, so it is on record.
void ClearCredits() {
  PinNodeCtrl_Credits(Credits, FreePlayMode);
  Credits = 0;
  RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
  // (in diagnostics the credit display belongs to the operator menu)
  if (MachineState == MACHINE_STATE_ATTRACT) RPU_SetDisplayCredits(Credits, !FreePlayMode);
}

// The adjustments go back to their defaults. Credits, the high score and the audits stay.
void RestoreDefaults() {
  byte savedCredits = Credits;
  unsigned long savedHighScore = HighScore;
  unsigned long savedAudits[6];
  for (byte count = 0; count < 6; count++) savedAudits[count] = RPU_ReadULFromEEProm(PinNodeAuditsTable[count].eeprom);

  SetAllParameterDefaults();
  Credits = savedCredits;
  HighScore = savedHighScore;
  WriteParameters(false);
  for (byte count = 0; count < 6; count++) RPU_WriteULToEEProm(PinNodeAuditsTable[count].eeprom, savedAudits[count]);

  CPCSelectionsHaveBeenRead = false;
  ReadStoredParameters();
  RPU_SetDisplayCredits(Credits, !FreePlayMode);
  PinNodeCtrl_SettingsChanged();
}

// rulesLevel: GAME_RULES_EASY, MEDIUM, HARD or PROGRESSIVE
void InstallPreset(byte rulesLevel) {
  GameRulesSelection = rulesLevel;
  RPU_WriteByteToEEProm(EEPROM_GAME_RULES_SELECTION, GameRulesSelection);
  if (LoadRuleDefaults(rulesLevel)) WriteParameters();
  PinNodeCtrl_SettingsChanged();
}


// Runs the test in progress. Called on every pass of loop() while in diagnostics.
void UpdateRemoteTest() {
  if (RemoteTest == ACT_DISPLAY_TEST) {
    RPU_CycleAllDisplays(CurrentTime, (byte)RemoteTestValue);
  } else if (RemoteTest == ACT_SOLENOID_TEST) {
    if (RemoteTestLastStep == 0 || (CurrentTime - RemoteTestLastStep) >= SOLENOID_TEST_PERIOD_MS) {
      RemoteTestLastStep = CurrentTime;
      byte solenoidNumber = (byte)RemoteTestArg;
      if (RemoteTestArg == 0) {
        // each solenoid the menu knows, in turn
        for (byte tries = 0; tries < 20; tries++) {
          RemoteTestValue = (RemoteTestValue % 19) + 1;
          if (SolenoidConvertDisplayNumberToIndex((byte)RemoteTestValue) < 0x00FD) break;
        }
        solenoidNumber = (byte)RemoteTestValue;
        // tell the app which coil this is (a repeated "ok" updates its readout)
        PinNodeCtrl_ActionResult(RemoteTestReqId, ACT_SOLENOID_TEST, PINNODE_ACTION_OK, RemoteTestValue);
      }
      unsigned short solenoidIndex = SolenoidConvertDisplayNumberToIndex(solenoidNumber);
      // (continuous solenoids, in the upper byte, are left to the coin-door menu)
      if (solenoidIndex < 0x00FD) {
        byte strength = SolenoidConvertDisplayNumberToTestStrength(solenoidNumber);
        if (strength >= OPERATOR_MENU_VALUE_UNUSED) strength = 4;
        RPU_PushToSolenoidStack((byte)solenoidIndex, strength, true);
      }
    }
  } else if (RemoteTest == ACT_SWITCH_TEST) {
    for (byte sw = 0; sw < SWITCH_TEST_NUM_SWITCHES; sw++) {
      boolean closed = RPU_ReadSingleSwitchState(sw);
      boolean wasClosed = (RemoteSwitchStates[sw / 8] >> (sw % 8)) & 0x01;
      if (closed != wasClosed) {
        if (closed) {
          RemoteSwitchStates[sw / 8] |= (1 << (sw % 8));
          if (!RemoteSwitchTestFirstPass) RemoteTestValue += 1;
        } else {
          RemoteSwitchStates[sw / 8] &= ~(1 << (sw % 8));
        }
        PinNodeCtrl_SwitchEvent(sw, closed);
      }
    }
    RemoteSwitchTestFirstPass = false;
  }
}


// A test that was running when the machine left diagnostics is reported as stopped
void EndDiagnostics() {
  if (RemoteTest != REMOTE_TEST_NONE) {
    byte runningTest = RemoteTest;
    long value = RemoteTestValue;
    StopTest();
    PinNodeCtrl_ActionResult(RemoteTestReqId, runningTest, PINNODE_ACTION_DONE, value, PINNODE_REASON_DIAGNOSTICS);
  }
  RPU_TurnOffAllLamps();
  Audio.StopAllAudio();
}


// Takes a request from the app, where the game can act on it: attract and diagnostics.
// (Anywhere else nobody takes it, and the library answers "not now".)
int HandleOperatorAction(int curState) {
  int returnState = curState;
  boolean inDiagnostics = (curState == MACHINE_STATE_DIAGNOSTICS);

  PinNodeActionRequest request;
  if (!PinNodeCtrl_TakeAction(&request)) return returnState;

  if (request.op == PINNODE_OP_STOP) {
    long value = 0;
    if (RemoteTest == request.action) {
      value = RemoteTestValue;
      StopTest();
    }
    PinNodeCtrl_ActionResult(request.reqId, request.action, PINNODE_ACTION_DONE, value, request.reason);
    return returnState;
  }

  if (request.action <= ACT_SOUND_TEST) {
    // a test
    if (!inDiagnostics) {
      PinNodeCtrl_ActionResult(request.reqId, request.action, PINNODE_ACTION_NOT_NOW, 0);
      return returnState;
    }
    if (RemoteTest != REMOTE_TEST_NONE && RemoteTest != request.action) StopTest();
    RemoteTestReqId = request.reqId;
    RemoteTestArg = request.arg;
    switch (request.action) {
      case ACT_LAMP_TEST: StartLampTest(request.arg); break;
      case ACT_DISPLAY_TEST: StartDisplayTest(request.arg); break;
      case ACT_SOLENOID_TEST: StartSolenoidTest(request.arg); break;
      case ACT_SWITCH_TEST: StartSwitchTest(); break;
      case ACT_SOUND_TEST: PlaySoundTest(request.arg); break;
    }
    PinNodeCtrl_ActionResult(request.reqId, request.action, PINNODE_ACTION_OK, RemoteTestValue);
    return returnState;
  }

  // a one-shot (the two clears answer with what was there before: total plays, credits)
  long previousValue = 0;
  switch (request.action) {
    case ACT_EJECT_BALLS: EjectAllBalls(); break;
    case ACT_CLEAR_AUDITS:
      previousValue = (long)RPU_ReadULFromEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE);
      ClearAudits();
      break;
    case ACT_CLEAR_CREDITS:
      previousValue = Credits;
      ClearCredits();
      break;
    case ACT_FACTORY_DEFAULTS: RestoreDefaults(); break;
    case ACT_PRESET_EASY: InstallPreset(GAME_RULES_EASY); break;
    case ACT_PRESET_MEDIUM: InstallPreset(GAME_RULES_MEDIUM); break;
    case ACT_PRESET_HARD: InstallPreset(GAME_RULES_HARD); break;
    case ACT_PRESET_PROGRESSIVE: InstallPreset(GAME_RULES_PROGRESSIVE); break;
    case ACT_ENTER_DIAGNOSTICS: returnState = MACHINE_STATE_DIAGNOSTICS; break;
    case ACT_EXIT_DIAGNOSTICS:
      if (inDiagnostics) {
        EndDiagnostics();
        returnState = MACHINE_STATE_ATTRACT;
      }
      break;
    default:
      PinNodeCtrl_ActionResult(request.reqId, request.action, PINNODE_ACTION_UNKNOWN, 0);
      return returnState;
  }
  PinNodeCtrl_ActionResult(request.reqId, request.action, PINNODE_ACTION_DONE, previousValue, PINNODE_REASON_FINISHED);
  return returnState;
}


// The tournament lock: while it's on, a difficulty setting can't be changed at the coin
// door either. The menu has already saved the edit by the time the game hears of it, so
// the old value is remembered when the adjustment is opened and put back.
byte *GuardedAdjustmentByte = NULL;
unsigned long *GuardedAdjustmentUL = NULL;
unsigned short GuardedAdjustmentEeprom = 0;
unsigned long GuardedAdjustmentValue = 0;
boolean AdjustmentGuarded = false;

void GuardAdjustmentIfLocked(unsigned short eepromAddress, byte *adjustmentByte, unsigned long *adjustmentUL) {
  AdjustmentGuarded = false;
  if (eepromAddress == 0 || (adjustmentByte == NULL && adjustmentUL == NULL)) return;
  for (byte count = 0; count < (sizeof(PinNodeSettingsTable) / sizeof(PinNodeSettingsTable[0])); count++) {
    if (PinNodeSettingsTable[count].eeprom == eepromAddress && PinNodeCtrl_SettingLocked(count)) {
      AdjustmentGuarded = true;
      GuardedAdjustmentEeprom = eepromAddress;
      GuardedAdjustmentByte = adjustmentByte;
      GuardedAdjustmentUL = adjustmentUL;
      GuardedAdjustmentValue = adjustmentUL ? (*adjustmentUL) : (unsigned long)(*adjustmentByte);
      return;
    }
  }
}

// true if the edit was refused
boolean RevertGuardedAdjustment() {
  if (!AdjustmentGuarded) return false;
  if (GuardedAdjustmentUL) {
    *GuardedAdjustmentUL = GuardedAdjustmentValue;
    RPU_WriteULToEEProm(GuardedAdjustmentEeprom, GuardedAdjustmentValue);
  } else {
    *GuardedAdjustmentByte = (byte)GuardedAdjustmentValue;
    RPU_WriteByteToEEProm(GuardedAdjustmentEeprom, (byte)GuardedAdjustmentValue);
  }
  return true;
}
#endif


////////////////////////////////////////////////////////////////////////////
//
//  Setup
//    Arduino calls this function at power up and reset.
//    It's used to initialize the hardware and 
//    certain variables and structures used by 
//    the code.
//
////////////////////////////////////////////////////////////////////////////

#ifdef RPU_OS_USE_PINNODE_LINK
// Start the link and say HELLO (PinNodeCtrl_Update repeats it whenever the link comes back).
// The tables are registered first, so HELLO carries the settings count.
void StartPinNodeLink() {
  PinNodeCtrl_Settings(PinNodeSettingsTable, sizeof(PinNodeSettingsTable) / sizeof(PinNodeSettingsTable[0]), PinNodeSettingsApplied);
  PinNodeCtrl_Audits(PinNodeAuditsTable, sizeof(PinNodeAuditsTable) / sizeof(PinNodeAuditsTable[0]));
  PinNodeCtrl_Actions(PinNodeActionsTable, sizeof(PinNodeActionsTable) / sizeof(PinNodeActionsTable[0]));
  RPULink_BeginUart();   // the ESP32 in the WiFi socket, on USART2
  byte PinNodeManifest[8];
  PinNodeManifestBytes(PinNodeManifest);
  PinNodeCtrl_Begin(PINNODE_RULESET_VERSION, PINNODE_RULESET_ID, PinNodeManifest, RPU_NUMBER_OF_PLAYERS_ALLOWED, 0, RPU_OS_HARDWARE_REV);
}
#endif

// What this sketch still has to do while the original ROM game runs
void OriginalGameIdle() {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
  // Send a periodic message to keep GI on
  if (GIInOriginal) {
    if (millis() >  (ALBWatchdogResetLastSent + ALB_WATCHDOG_TIMER_DURATION/2)) {
      TopperALB.WatchdogTimerReset();
      ALBWatchdogResetLastSent = millis();
    }
  }
#endif
}

void setup() {

  if (DEBUG_MESSAGES) {
    // If debug is on, set up the Serial port for communication
    Serial.begin(115200);
    Serial.write("Starting\n");
  }

  // Set up the Audio handler in order to play boot messages
  CurrentTime = millis();
  Audio.InitDevices(AUDIO_PLAY_TYPE_WAV_TRIGGER | AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS);
  Audio.StopAllAudio();
  Audio.SetMusicDuckingGain(25);
  Audio.SetSoundFXDuckingGain(20);

  // Tell the OS about game-specific switches
  // (this is for software-controlled pop bumpers and slings)
#if (RPU_MPU_ARCHITECTURE<10)
  // Machines with a -17, -35, 100, and 200 architecture
  // almost always have software based switch-triggered solenoids.
  // For those, you can define an array of solenoids and the switches
  // that will trigger them:
  RPU_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, SolenoidAssociatedSwitches);

#endif

  // Set up the chips and interrupts
  unsigned long initResult = 0;
  if (DEBUG_MESSAGES) Serial.write("Initializing MPU\n");

  // If the hardware has the ability to switch on the Credit/Reset button (requires Rev 4 or greater)
  // then that can be used to choose Original or New code. Otherwise, the hardware switch
  // will choose Original if open, and New if closed
  unsigned long initOptions = RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED |
                              RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN | RPU_CMD_PERFORM_MPU_TEST;
#ifdef RPU_OS_USE_PINNODE_LINK
  // The owner can choose the original ROM game from the app or the web. With that choice
  // saved, the machine boots the original unless Credit/Reset is held at power-on (which
  // boots this game and clears the choice).
  initOptions = PinNode_BootOptions(initOptions);
#endif
  initResult = RPU_InitializeMPU(initOptions, SW_CREDIT_RESET);
#ifdef RPU_OS_USE_PINNODE_LINK
  PinNode_BootChosen(initResult);
#endif

  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Return from init = 0x%04lX\n", initResult);
    Serial.write(buf);
    if (initResult & RPU_RET_6800_DETECTED) Serial.write("Detected 6800 clock\n");
    else if (initResult & RPU_RET_6802_OR_8_DETECTED) Serial.write("Detected 6802/8 clock\n");
    Serial.write("Back from init\n");
  }

  if (initResult & RPU_RET_SELECTOR_SWITCH_ON) QueueDIAGNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_ON);
  else QueueDIAGNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_OFF);

  if (initResult & RPU_RET_CREDIT_RESET_BUTTON_HIT) QueueDIAGNotification(SOUND_EFFECT_DIAG_CREDIT_RESET_BUTTON);

  if (initResult & RPU_RET_DIAGNOSTIC_REQUESTED) {
    QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_DIAGNOSTICS);
    // Run diagnostics here:
  }

  if (initResult & RPU_RET_ORIGINAL_CODE_REQUESTED) {
    if (DEBUG_MESSAGES) Serial.write("Asked to run original code\n");
    delay(100);
    QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_ORIGINAL_CODE);
    delay(100);

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
    TopperALB.InitOutogingCommunication();
    TopperALB.SetTargetDeviceAddress(8);
    TopperALB.EnableLamps();
    TopperALB.WatchdogTimerReset();
    TopperALB.AdjustSetting(SET_COLOR_0, 0, 0, 200);
    TopperALB.AdjustSetting(SET_COLOR_1, 200, 200, 0);
    ALBWatchdogResetLastSent = millis();
#endif

    while (Audio.Update(millis()));
#ifdef RPU_OS_USE_PINNODE_LINK
    // The original game has the machine. This keeps the link to the node up (it reports
    // "original"), so the owner can switch back from the app; it never returns.
    StartPinNodeLink();
    PinNode_RunOriginal(OriginalGameIdle);
#else
    // Arduino should hang if original code is running
    while (1) {
      OriginalGameIdle();
    }
#endif
  }
  QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_NEW_CODE);

  RPU_DisableSolenoidStack();
  RPU_SetDisableFlippers(true);

  // Read parameters from EEProm
  ReadStoredParameters();

#ifdef RPU_OS_USE_PINNODE_LINK
  StartPinNodeLink();
#endif

  CurrentScores[0] = GAME_MAJOR_VERSION;
  CurrentScores[1] = GAME_MINOR_VERSION;
  CurrentScores[2] = RPU_OS_MAJOR_VERSION;
  CurrentScores[3] = RPU_OS_MINOR_VERSION;

  CurrentAchievements[0] = 0;
  CurrentAchievements[1] = 0;
  CurrentAchievements[2] = 0;
  CurrentAchievements[3] = 0;

  // CUSTOMIZATION TODO:
  // If your game has drop targets, this is a good place
  // to configure the drop target switches and solenoids
  // Initialize any drop target variables here
  LeftDropTargets.DefineSwitch(0, SW_DROP_L_1);
  LeftDropTargets.DefineSwitch(1, SW_DROP_L_2);
  LeftDropTargets.DefineSwitch(2, SW_DROP_L_3);
  LeftDropTargets.DefineResetSolenoid(0, SOL_DROP_BANK_L_RESET);
#ifdef MACHINE_BALLY_STAR_TREK
  LeftDropTargets.DefineSwitch(3, SW_DROP_L_4);
#else
  RightDropTargets.DefineSwitch(0, SW_DROP_R_1);
  RightDropTargets.DefineSwitch(1, SW_DROP_R_2);
  RightDropTargets.DefineSwitch(2, SW_DROP_R_3);
  RightDropTargets.DefineResetSolenoid(0, SOL_DROP_BANK_R_RESET);
#endif

  CurrentTime = millis();

  // CUSTOMIZATION TODO:
  // You can play a machine startup sound here
  Audio.QueueSound(SOUND_EFFECT_STARTUP_1 + (micros()) % 2, AUDIO_PLAY_TYPE_WAV_TRIGGER, CurrentTime + 1200);
  OperatorSwitchPressStarted = 0;
  InOperatorMenu = false;

  // CUSTOMIZATION TODO:
  // The operator menus can use different configurations for navigation buttons.
  // I've left a commented out examples from Black Knight and Firepower to show how forward/back buttons
  // can be configured if you have them
  Menus.SetNavigationButtons(SWITCH_STACK_EMPTY, SWITCH_STACK_EMPTY, SW_CREDIT_RESET, SW_SELF_TEST_SWITCH);
  // Menus.SetNavigationButtons(SW_RIGHT_MAGNET_BUTTON, SW_LEFT_MAGNET_BUTTON, SW_CREDIT_RESET, SW_SELF_TEST_SWITCH);
  // Menus.SetNavigationButtons(SW_RIGHT_FLIPPER, SW_HIGH_SCORE_RESET, SW_CREDIT_RESET, SW_SELF_TEST_SWITCH);

  Menus.SetLampsLookupCallback(LampConvertDisplayNumberToIndex);
  Menus.SetSolenoidIDLookupCallback(SolenoidConvertDisplayNumberToIndex);
  Menus.SetSolenoidStrengthLookupCallback(SolenoidConvertDisplayNumberToTestStrength);
  Menus.SetMenuButtonDebounce(250);

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
  TopperALB.InitOutogingCommunication();
  TopperALB.SetTargetDeviceAddress(8);
  TopperALB.EnableLamps();
  TopperALB.WatchdogTimerReset();
  TopperALB.AdjustSetting(SET_COLOR_0, 0, 0, 200);
  TopperALB.AdjustSetting(SET_COLOR_1, 200, 200, 0);
  ALBWatchdogResetLastSent = millis();
#endif

  LastTiltWarningTime = 0;
}

byte ReadSetting(byte setting, byte defaultValue, byte maxValue) {
  byte value = EEPROM.read(setting);
  if (value == 0xFF || value>maxValue) {
    EEPROM.write(setting, defaultValue);
    return defaultValue;
  }
  return value;
}

// This function is useful for checking the status of drop target switches
byte CheckSequentialSwitches(byte startingSwitch, byte numSwitches) {
  byte returnSwitches = 0;
  for (byte count = 0; count < numSwitches; count++) {
    returnSwitches |= (RPU_ReadSingleSwitchState(startingSwitch + count) << count);
  }
  return returnSwitches;
}

// CUSTOMIZATION TODO:
// Set up lamp functions so the lamps will reflect
// the current game state. For organization, create a different
// function for groups of similar lamps, like ShowTopLanes()
////////////////////////////////////////////////////////////////////////////
//
//  Lamp Management functions
//    These functions are called each time through the gameplay loop.
//    They use the current status variables to set each lamp to 
//    on, off, dim (depending on hardware), or flashing. The lamps are
//    actually set in hardware by the Interrupt Service Routine, so
//    these functions simply update the state arrays used by the ISR.
//
//    If a special animation is required (sweeps and such), then these
//    functions are turned off and one of the special functions in
//    "LampAnimations.h" is used to set those lamps.
//
////////////////////////////////////////////////////////////////////////////
void SetGeneralIlluminationOn(boolean setGIOn = true) {
  // Since this machine doesn't have GI control,
  // this line prevents compiler warnings.
  (void)setGIOn;
}

void ShowPlayerLamps() {  
  for (byte count = 0; count < 4; count++) {
    if (count==CurrentPlayer) RPU_SetLampState(PlayerUpLamps[count], 1, 0, 250);
    else if (count<CurrentNumPlayers) RPU_SetLampState(PlayerUpLamps[count], 1);
    else RPU_SetLampState(PlayerUpLamps[count], 0);
  }  
}

byte BonusLampAssignments[11] = { LAMP_BONUS_1K, LAMP_BONUS_2K, LAMP_BONUS_3K, LAMP_BONUS_4K, LAMP_BONUS_5K, LAMP_BONUS_6K, 
                                  LAMP_BONUS_7K, LAMP_BONUS_8K, LAMP_BONUS_9K, LAMP_BONUS_10K, LAMP_BONUS_20K};

void ShowBonusLamps() {
  if (GameMode == GAME_MODE_SKILL_SHOT) {
    byte bonusPhase = (CurrentTime/100)%11;
    for (byte count=0; count<11; count++) RPU_SetLampState(BonusLampAssignments[count], count==bonusPhase);
  } else {
    RPU_SetLampState(LAMP_BONUS_20K, Bonus[CurrentPlayer]>=20);
    RPU_SetLampState(LAMP_BONUS_10K, Bonus[CurrentPlayer]>=10 && Bonus[CurrentPlayer]<20);
    for (byte count=1; count<10; count++) {
      RPU_SetLampState(BonusLampAssignments[count-1], (Bonus[CurrentPlayer]%10)==count);
    }
  }
}



void ShowShootAgainLamp() {

  if ( (BallFirstSwitchHitTime == 0 && BallSaveNumSeconds) || (BallSaveEndTime && CurrentTime < BallSaveEndTime) ) {
    unsigned long msRemaining = 5000;
    if (BallSaveEndTime != 0) msRemaining = BallSaveEndTime - CurrentTime;
    RPU_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, (msRemaining < 5000) ? 100 : 500);
  } else {
    RPU_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
  }
}





////////////////////////////////////////////////////////////////////////////
//
//  Machine State Helper functions
//
////////////////////////////////////////////////////////////////////////////
boolean AddPlayer(boolean resetNumPlayers = false) {

  if (Credits < 1 && !FreePlayMode) return false;
  if (resetNumPlayers) CurrentNumPlayers = 0;
  if (CurrentNumPlayers >= RPU_NUMBER_OF_PLAYERS_ALLOWED) return false;

  CurrentNumPlayers += 1;
  RPU_SetDisplay(CurrentNumPlayers - 1, 0, true, 2);
#ifdef RPU_OS_USE_PINNODE_LINK
  // Player 1 is reported by PinNodeCtrl_GameStart; this is players 2-4 joining
  if (CurrentNumPlayers > 1) PinNodeCtrl_PlayerAdded(CurrentNumPlayers);
#endif
  
  if (CurrentNumPlayers > 1) {
    // If this is second, third, or fourth player, then playe the announcment
    QueueNotification(SOUND_EFFECT_VP_ADD_PLAYER_1 + (CurrentNumPlayers - 1), 10);
  }

  for (byte count = 0; count < 4; count++) {
    if (count==CurrentPlayer) RPU_SetLampState(PlayerUpLamps[count], 1, 0, 250);
    else if (count<CurrentNumPlayers) RPU_SetLampState(PlayerUpLamps[count], 1);
    else RPU_SetLampState(PlayerUpLamps[count], 0);
  }

  if (!FreePlayMode) {
    Credits -= 1;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(false);
  }

  RPU_WriteULToEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

  return true;
}


unsigned short ChuteAuditByte[] = {RPU_CHUTE_1_COINS_START_BYTE, RPU_CHUTE_2_COINS_START_BYTE, RPU_CHUTE_3_COINS_START_BYTE};
void AddCoinToAudit(byte chuteNum) {
  if (chuteNum > 2) return;
  unsigned short coinAuditStartByte = ChuteAuditByte[chuteNum];
  RPU_WriteULToEEProm(coinAuditStartByte, RPU_ReadULFromEEProm(coinAuditStartByte) + 1);
}


void AddCredit(boolean playSound = false, byte numToAdd = 1) {
  if (Credits < MaximumCredits) {
    Credits += numToAdd;
    if (Credits > MaximumCredits) Credits = MaximumCredits;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    if (playSound) {
      //PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
      RPU_PushToSolenoidStack(SOL_KNOCKER, KNOCKER_SOLENOID_STRENGTH, true);
    }
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(false);
  } else {
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(true);
  }

}

byte SwitchToChuteNum(byte switchHit) {
  byte chuteNum = 0;
  if (switchHit == SW_COIN_2) chuteNum = 1;
  else if (switchHit == SW_COIN_3) chuteNum = 2;
  return chuteNum;
}

boolean AddCoin(byte chuteNum) {
  boolean creditAdded = false;
  if (chuteNum > 2) return false;
  byte cpcSelection = GetCPCSelection(chuteNum);

  // Find the lowest chute num with the same ratio selection
  // and use that ChuteCoinsInProgress counter
  byte chuteNumToUse;
  for (chuteNumToUse = 0; chuteNumToUse <= chuteNum; chuteNumToUse++) {
    if (GetCPCSelection(chuteNumToUse) == cpcSelection) break;
  }

  PlaySoundEffect(SOUND_EFFECT_COIN_DROP_1 + (CurrentTime % 3));

  byte cpcCoins = GetCPCCoins(cpcSelection);
  byte cpcCredits = GetCPCCredits(cpcSelection);
  byte coinProgressBefore = ChuteCoinsInProgress[chuteNumToUse];
  ChuteCoinsInProgress[chuteNumToUse] += 1;

  if (ChuteCoinsInProgress[chuteNumToUse] == cpcCoins) {
    if (cpcCredits > cpcCoins) AddCredit(cpcCredits - (coinProgressBefore));
    else AddCredit(cpcCredits);
    ChuteCoinsInProgress[chuteNumToUse] = 0;
    creditAdded = true;
  } else {
    if (cpcCredits > cpcCoins) {
      AddCredit(1);
      creditAdded = true;
    } else {
    }
  }

  return creditAdded;
}


void AddSpecialCredit() {
  AddCredit(false, 1);
  RPU_PushToTimedSolenoidStack(SOL_KNOCKER, KNOCKER_SOLENOID_STRENGTH, CurrentTime, true);
  RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
}

void AwardSpecial() {
  if (SpecialCollected) return;
  SpecialCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += SpecialValue * PlayfieldMultiplier;
  } else {
    AddSpecialCredit();
  }
}

boolean AwardExtraBall(boolean basedOnScore = false) {
  if (ExtraBallAvailable[CurrentPlayer]==1 || (basedOnScore && ExtraBallAvailable[CurrentPlayer]!=2)) {
    ExtraBallAvailable[CurrentPlayer] = 2;
    if (TournamentScoring) {
      CurrentScores[CurrentPlayer] += ExtraBallValue * PlayfieldMultiplier;
    } else {
      SamePlayerShootsAgain = true;
      RPU_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
      QueueNotification(SOUND_EFFECT_VP_EXTRA_BALL, 8);
    }
    return true;
  }
  return false;
}


void IncreasePlayfieldMultiplier(unsigned long duration) {
  PlayfieldMultiplierTimeLeft += duration;

  PlayfieldMultiplier += 1;
  if (PlayfieldMultiplier > 5) {
    PlayfieldMultiplier = 5;
  }
}


void SetBallSave(unsigned long duration, byte numberOfSaves = 0xFF, boolean addToBallSave = false) {

  if (duration == 0) {
    BallSaveEndTime = 0;
    NumberOfBallSavesRemaining = 0;
  } else if (addToBallSave) {
    if (BallSaveEndTime) BallSaveEndTime += duration;
  } else {
    BallSaveEndTime = CurrentTime + duration;
    NumberOfBallSavesRemaining = numberOfSaves;
  }
}




#define SOUND_EFFECT_OM_CPC_VALUES                  180
#define SOUND_EFFECT_OM_CRB_VALUES                  210
#define SOUND_EFFECT_OM_DIFFICULTY_VALUES           220

#define SOUND_EFFECT_AP_TOP_LEVEL_MENU_ENTRY    1700
#define SOUND_EFFECT_AP_TEST_MENU               1701
#define SOUND_EFFECT_AP_AUDITS_MENU             1702
#define SOUND_EFFECT_AP_BASIC_ADJUSTMENTS_MENU  1703
#define SOUND_EFFECT_AP_GAME_RULES_LEVEL        1704
#define SOUND_EFFECT_AP_GAME_SPECIFIC_ADJ_MENU  1705

#define SOUND_EFFECT_AP_TEST_LAMPS              1710
#define SOUND_EFFECT_AP_TEST_DISPLAYS           1711
#define SOUND_EFFECT_AP_TEST_SOLENOIDS          1712
#define SOUND_EFFECT_AP_TEST_SWITCHES           1713
#define SOUND_EFFECT_AP_TEST_SOUNDS             1714
#define SOUND_EFFECT_AP_TEST_EJECT_BALLS        1715

#define SOUND_EFFECT_AP_AUDIT_TOTAL_PLAYS       1720
#define SOUND_EFFECT_AP_AUDIT_CHUTE_1_COINS     1721
#define SOUND_EFFECT_AP_AUDIT_CHUTE_2_COINS     1722
#define SOUND_EFFECT_AP_AUDIT_CHUTE_3_COINS     1723
#define SOUND_EFFECT_AP_AUDIT_TOTAL_REPLAYS     1724
#define SOUND_EFFECT_AP_AUDIT_AVG_BALL_TIME     1725
#define SOUND_EFFECT_AP_AUDIT_HISCR_BEAT        1726
#define SOUND_EFFECT_AP_AUDIT_TOTAL_BALLS       1727
#define SOUND_EFFECT_AP_AUDIT_NUM_MATCHES       1728
#define SOUND_EFFECT_AP_AUDIT_MATCH_PERCENTAGE  1729
#define SOUND_EFFECT_AP_AUDIT_LIFETIME_PLAYS    1730
#define SOUND_EFFECT_AP_AUDIT_MINUTES_ON        1731
#define SOUND_EFFECT_AP_AUDIT_CLEAR_AUDITS      1732

#define OM_BASIC_ADJ_IDS_FREEPLAY               0
#define OM_BASIC_ADJ_IDS_BALL_SAVE              1
#define OM_BASIC_ADJ_IDS_TILT_WARNINGS          2
#define OM_BASIC_ADJ_IDS_MUSIC_VOLUME           3
#define OM_BASIC_ADJ_IDS_SOUNDFX_VOLUME         4
#define OM_BASIC_ADJ_IDS_CALLOUTS_VOLUME        5
#define OM_BASIC_ADJ_IDS_BALLS_PER_GAME         6
#define OM_BASIC_ADJ_IDS_TOURNAMENT_MODE        7
#define OM_BASIC_ADJ_IDS_EXTRA_BALL_VALUE       8
#define OM_BASIC_ADJ_IDS_SPECIAL_VALUE          9 
#define OM_BASIC_ADJ_IDS_RESET_DURING_GAME      10
#define OM_BASIC_ADJ_IDS_SCORE_LEVEL_1          11
#define OM_BASIC_ADJ_IDS_SCORE_LEVEL_2          12
#define OM_BASIC_ADJ_IDS_SCORE_LEVEL_3          13
#define OM_BASIC_ADJ_IDS_SCORE_AWARDS           14
#define OM_BASIC_ADJ_IDS_SCROLLING_SCORES       15
#define OM_BASIC_ADJ_IDS_HISCR                  16
#define OM_BASIC_ADJ_IDS_CREDITS                17
#define OM_BASIC_ADJ_IDS_CPC_1                  18
#define OM_BASIC_ADJ_IDS_CPC_2                  19
#define OM_BASIC_ADJ_IDS_CPC_3                  20
#define OM_BASIC_ADJ_IDS_MATCH_FEATURE          21
#define OM_BASIC_ADJ_FINISHED                   22
#define SOUND_EFFECT_AP_FREEPLAY                (1740 + OM_BASIC_ADJ_IDS_FREEPLAY)
#define SOUND_EFFECT_AP_BALL_SAVE_SECONDS       (1740 + OM_BASIC_ADJ_IDS_BALL_SAVE)
#define SOUND_EFFECT_AP_TILT_WARNINGS           (1740 + OM_BASIC_ADJ_IDS_TILT_WARNINGS)
#define SOUND_EFFECT_AP_MUSIC_VOLUME            (1740 + OM_BASIC_ADJ_IDS_MUSIC_VOLUME)
#define SOUND_EFFECT_AP_SOUNDFX_VOLUME          (1740 + OM_BASIC_ADJ_IDS_SOUNDFX_VOLUME)
#define SOUND_EFFECT_AP_CALLOUTS_VOLUME         (1740 + OM_BASIC_ADJ_IDS_CALLOUTS_VOLUME)
#define SOUND_EFFECT_AP_BALLS_PER_GAME          (1740 + OM_BASIC_ADJ_IDS_BALLS_PER_GAME)
#define SOUND_EFFECT_AP_TOURNAMENT_MODE         (1740 + OM_BASIC_ADJ_IDS_TOURNAMENT_MODE)
#define SOUND_EFFECT_AP_EXTRA_BALL_VALUE        (1740 + OM_BASIC_ADJ_IDS_EXTRA_BALL_VALUE)
#define SOUND_EFFECT_AP_SPECIAL_VALUE           (1740 + OM_BASIC_ADJ_IDS_SPECIAL_VALUE)
#define SOUND_EFFECT_AP_RESET_DURING_GAME       (1740 + OM_BASIC_ADJ_IDS_RESET_DURING_GAME)
#define SOUND_EFFECT_AP_ADJ_SCORE_LEVEL_1       (1740 + OM_BASIC_ADJ_IDS_SCORE_LEVEL_1)
#define SOUND_EFFECT_AP_ADJ_SCORE_LEVEL_2       (1740 + OM_BASIC_ADJ_IDS_SCORE_LEVEL_2)
#define SOUND_EFFECT_AP_ADJ_SCORE_LEVEL_3       (1740 + OM_BASIC_ADJ_IDS_SCORE_LEVEL_3)
#define SOUND_EFFECT_AP_SCORE_AWARDS            (1740 + OM_BASIC_ADJ_IDS_SCORE_AWARDS)
#define SOUND_EFFECT_AP_SCROLLING_SCORES        (1740 + OM_BASIC_ADJ_SCROLLING_SCORES)
#define SOUND_EFFECT_AP_ADJ_HISCR               (1740 + OM_BASIC_ADJ_IDS_HISCR)
#define SOUND_EFFECT_AP_ADJ_CREDITS             (1740 + OM_BASIC_ADJ_IDS_CREDITS)
#define SOUND_EFFECT_AP_ADJ_CPC_1               (1740 + OM_BASIC_ADJ_IDS_CPC_1)
#define SOUND_EFFECT_AP_ADJ_CPC_2               (1740 + OM_BASIC_ADJ_IDS_CPC_2)
#define SOUND_EFFECT_AP_ADJ_CPC_3               (1740 + OM_BASIC_ADJ_IDS_CPC_3)
#define SOUND_EFFECT_AP_MATCH_FEATURE           (1740 + OM_BASIC_ADJ_IDS_MATCH_FEATURE)

#define SOUND_EFFECT_OM_EASY_RULES_INSTRUCTIONS           1770
#define SOUND_EFFECT_OM_MEDIUM_RULES_INSTRUCTIONS         1771
#define SOUND_EFFECT_OM_HARD_RULES_INSTRUCTIONS           1772
#define SOUND_EFFECT_OM_PROGRESSIVE_RULES_INSTRUCTIONS    1773
#define SOUND_EFFECT_OM_CUSTOM_RULES_INSTRUCTIONS         1774

#define OM_GAME_ADJ_EASY_DIFFICULTY                 0
#define OM_GAME_ADJ_MEDIUM_DIFFICULTY               1
#define OM_GAME_ADJ_HARD_DIFFICULTY                 2
#define OM_GAME_ADJ_PROGRESSIVE_DIFFICULTY          3
#define OM_GAME_ADJ_CUSTOM_DIFFICULTY               4
#define SOUND_EFFECT_AP_DIFFICULTY                  (1790 + OM_GAME_ADJ_EASY_DIFFICULTY)

// CUSTOMIZATION TODO:
// Create defines for custom parameters and be sure to 
// create the associated callouts (starting at 1800)
#define OM_GAME_ADJ_TROUGH_EJECT_STRENGTH                   0
#define OM_GAME_ADJ_SAUCER_EJECT_STRENGTH                   1
#define OM_GAME_ADJ_FINISHED                                2   // then the PinNode adjustments (the library's)
#define SOUND_EFFECT_AP_GAME_ADJUSTMENT_CALLOUTS_START      (1800 + OM_GAME_ADJ_TROUGH_EJECT_STRENGTH)

unsigned long SoundSettingTimeout;
unsigned long SoundTestStart;
byte SoundTestSequence;
unsigned short RestoreBackgroundTrack = BACKGROUND_TRACK_NONE;
  
// The operator menu opens on its "return to game" page, which shows nothing: every display
// is blank and the game is stopped until a button is pressed. Left alone there it closes
// by itself, so a stray self-test signal can't leave the machine looking dead.
#define OPERATOR_MENU_IDLE_EXIT_MS    15000
unsigned long OperatorMenuLastActivity = 0;

void RunOperatorMenu() {
  boolean menuDone = !Menus.UpdateMenu(CurrentTime);

  if (!menuDone && Menus.GetTopLevel() == OPERATOR_MENU_RETURN_TO_GAME && OperatorMenuLastActivity != 0 &&
      (CurrentTime - OperatorMenuLastActivity) > OPERATOR_MENU_IDLE_EXIT_MS) {
    Menus.ExitOperatorMenu();
    menuDone = true;
  }

#ifdef RPU_OS_USE_PINNODE_LINK
  // From the phone, "exit diagnostics" closes the coin-door menu too; nothing else runs here
  PinNodeActionRequest menuRequest;
  if (!menuDone && PinNodeCtrl_TakeAction(&menuRequest)) {
    if (menuRequest.op == PINNODE_OP_START && menuRequest.action == ACT_EXIT_DIAGNOSTICS) {
      Menus.ExitOperatorMenu();
      menuDone = true;
      PinNodeCtrl_ActionResult(menuRequest.reqId, menuRequest.action, PINNODE_ACTION_DONE, 0, PINNODE_REASON_FINISHED);
    } else {
      PinNodeCtrl_ActionResult(menuRequest.reqId, menuRequest.action,
                               menuRequest.op == PINNODE_OP_STOP ? PINNODE_ACTION_DONE : PINNODE_ACTION_NOT_NOW, 0, menuRequest.reason);
    }
  }
#endif

  if (menuDone) {
    // Menu is done
    OperatorMenuLastActivity = 0;
#ifdef RPU_OS_USE_PINNODE_LINK
    // Tell the node what the operator changed (it diffs), and the audits (they may have been cleared)
    PinNodeCtrl_SettingsChanged();
    PinNodeCtrl_AuditsChanged();
#endif
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    Audio.StopAllAudio();
    RPU_TurnOffAllLamps();
    if (MachineState==MACHINE_STATE_ATTRACT) {
      RPU_SetDisplayBallInPlay(0, true);
    } else {
      RPU_SetDisplayBallInPlay(CurrentBallInPlay);
    }
    SoundSettingTimeout = 0;
    return;
  }

  // It's up to this function to eject balls if requested
  if (Menus.BallEjectInProgress()) {
    if (CountBallsInTrough()) {
      if (CurrentTime > (LastTimeBallServed+1500)) {
        LastTimeBallServed = CurrentTime;
        RPU_PushToSolenoidStack(SOL_SERVE_BALL, BallServeSolenoidStrength, true);
      }
    }
  } else {
    LastTimeBallServed = 0;
  }
  
  byte topLevel = Menus.GetTopLevel();
  byte subLevel = Menus.GetSubLevel();

  if (OperatorMenuLastActivity == 0) OperatorMenuLastActivity = CurrentTime;
  if (Menus.HasTopLevelChanged()) {
    OperatorMenuLastActivity = CurrentTime;
    // Play an audio prompt for the top level
    SoundTestStart = 0;
    if (Audio.GetBackgroundSong()!=BACKGROUND_TRACK_NONE) {
      RestoreBackgroundTrack = Audio.GetBackgroundSong();
    }
    Audio.StopAllAudio();
    Audio.PlaySound((unsigned short)topLevel + SOUND_EFFECT_AP_TOP_LEVEL_MENU_ENTRY, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
    if (Menus.GetTopLevel()==OPERATOR_MENU_GAME_RULES_LEVEL) Menus.SetNumSubLevels(4);
    if (Menus.GetTopLevel()==OPERATOR_MENU_BASIC_ADJ_MENU) {
      GetCPCSelection(0); // make sure CPC values have been read
      Menus.SetNumSubLevels(OM_BASIC_ADJ_FINISHED);
    }
    if (Menus.GetTopLevel()==OPERATOR_MENU_GAME_ADJ_MENU) Menus.SetNumSubLevels(OM_GAME_ADJ_FINISHED + PINNODE_NUM_ADJUSTMENTS);
  }
  if (Menus.HasSubLevelChanged()) {
    OperatorMenuLastActivity = CurrentTime;
    SoundTestStart = 0;
    // Play an audio prompt for the sub level    
    Audio.StopAllAudio();
    if (topLevel==OPERATOR_MENU_SELF_TEST_MENU) {
      Audio.PlaySound((unsigned short)subLevel + SOUND_EFFECT_AP_TEST_LAMPS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);

      if (subLevel==OPERATOR_MENU_TEST_SOUNDS) {
        SoundTestStart = CurrentTime + 1000;
        SoundTestSequence = 0;
      } else {
        SoundTestStart = 0;
      }
    } else if (topLevel==OPERATOR_MENU_AUDITS_MENU) {
      unsigned long *currentAdjustmentUL = NULL;
      byte currentAdjustmentStorageByte = 0;
      byte adjustmentType = OPERATOR_MENU_AUD_CLEARABLE;

      switch (subLevel) {
        case 0:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_TOTAL_PLAYS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_TOTAL_PLAYS_EEPROM_START_BYTE;
          break;
        case 1:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_CHUTE_1_COINS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_CHUTE_1_COINS_START_BYTE;
          break;
        case 2:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_CHUTE_2_COINS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_CHUTE_2_COINS_START_BYTE;
          break;
        case 3:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_CHUTE_3_COINS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_CHUTE_3_COINS_START_BYTE;
          break;
        case 4:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_TOTAL_REPLAYS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_TOTAL_REPLAYS_EEPROM_START_BYTE;
          break;
        case 5:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_HISCR_BEAT, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_TOTAL_HISCORE_BEATEN_START_BYTE;
          break;
      }

      Menus.SetAuditControls(currentAdjustmentUL, currentAdjustmentStorageByte, adjustmentType);

    } else if (topLevel==OPERATOR_MENU_BASIC_ADJ_MENU) {
      Audio.PlaySound((unsigned short)subLevel + SOUND_EFFECT_AP_FREEPLAY, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);

      byte *currentAdjustmentByte = NULL;
      byte currentAdjustmentStorageByte = 0;
      byte adjustmentValues[8] = {0};
      byte numAdjustmentValues = 2;
      byte adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
      short parameterCallout = 0;
      unsigned long *currentAdjustmentUL = NULL;
      
      adjustmentValues[1] = 1;

      switch(subLevel) {
        case OM_BASIC_ADJ_IDS_FREEPLAY:
          currentAdjustmentByte = (byte *)&FreePlayMode;
          currentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_BALL_SAVE:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 5;
          adjustmentValues[1] = 5;
          adjustmentValues[2] = 10;
          adjustmentValues[3] = 15;
          adjustmentValues[4] = 20;
          currentAdjustmentByte = &BallSaveNumSeconds;
          currentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_TILT_WARNINGS:
          adjustmentValues[1] = 2;
          currentAdjustmentByte = &MaxTiltWarnings;
          currentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_MUSIC_VOLUME:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          currentAdjustmentByte = &MusicVolume;
          currentAdjustmentStorageByte = EEPROM_MUSIC_VOLUME_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_SOUNDFX_VOLUME:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          currentAdjustmentByte = &SoundEffectsVolume;
          currentAdjustmentStorageByte = EEPROM_SFX_VOLUME_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_CALLOUTS_VOLUME:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          currentAdjustmentByte = &CalloutsVolume;
          currentAdjustmentStorageByte = EEPROM_CALLOUTS_VOLUME_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_BALLS_PER_GAME:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          numAdjustmentValues = 8;
          adjustmentValues[0] = 3;
          adjustmentValues[1] = 10;
          currentAdjustmentByte = &BallsPerGame;
          currentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_TOURNAMENT_MODE:
          currentAdjustmentByte = (byte *)&TournamentScoring;
          currentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_EXTRA_BALL_VALUE:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &ExtraBallValue;
          currentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_UL;
          break;
        case OM_BASIC_ADJ_IDS_SPECIAL_VALUE:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &SpecialValue;
          currentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_UL;
          break;
        case OM_BASIC_ADJ_IDS_RESET_DURING_GAME:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 5;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 1;
          adjustmentValues[2] = 2;
          adjustmentValues[3] = 3;
          adjustmentValues[4] = 99;
          currentAdjustmentByte = &TimeRequiredToResetGame;
          currentAdjustmentStorageByte = EEPROM_CRB_HOLD_TIME;
          parameterCallout = SOUND_EFFECT_OM_CRB_VALUES;
          break;
        case OM_BASIC_ADJ_IDS_SCORE_LEVEL_1:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &AwardScores[0];
          currentAdjustmentStorageByte = RPU_AWARD_SCORE_1_EEPROM_START_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_SCORE_LEVEL_2:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &AwardScores[1];
          currentAdjustmentStorageByte = RPU_AWARD_SCORE_2_EEPROM_START_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_SCORE_LEVEL_3:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &AwardScores[2];
          currentAdjustmentStorageByte = RPU_AWARD_SCORE_3_EEPROM_START_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_SCORE_AWARDS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX_DEFAULT;
          adjustmentValues[1] = 7;
          currentAdjustmentByte = &ScoreAwardReplay;
          currentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_SCROLLING_SCORES:
          currentAdjustmentByte = (byte *)&ScrollingScores;
          currentAdjustmentStorageByte = EEPROM_SCROLLING_SCORES_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_HISCR:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &HighScore;
          currentAdjustmentStorageByte = RPU_HIGHSCORE_EEPROM_START_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_CREDITS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 40;
          currentAdjustmentByte = &Credits;
          currentAdjustmentStorageByte = RPU_CREDITS_EEPROM_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_CPC_1:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_CPC;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = (NUM_CPC_PAIRS-1);
          currentAdjustmentByte = &(CPCSelection[0]);
          currentAdjustmentStorageByte = RPU_CPC_CHUTE_1_SELECTION_BYTE;
          parameterCallout = SOUND_EFFECT_OM_CPC_VALUES;
          break;
        case OM_BASIC_ADJ_IDS_CPC_2:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_CPC;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = (NUM_CPC_PAIRS-1);
          currentAdjustmentByte = &(CPCSelection[1]);
          currentAdjustmentStorageByte = RPU_CPC_CHUTE_2_SELECTION_BYTE;
          parameterCallout = SOUND_EFFECT_OM_CPC_VALUES;
          break;
        case OM_BASIC_ADJ_IDS_CPC_3:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_CPC;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = (NUM_CPC_PAIRS-1);
          currentAdjustmentByte = &(CPCSelection[2]);
          currentAdjustmentStorageByte = RPU_CPC_CHUTE_3_SELECTION_BYTE;
          parameterCallout = SOUND_EFFECT_OM_CPC_VALUES;
          break;
        case OM_BASIC_ADJ_IDS_MATCH_FEATURE:
          currentAdjustmentByte = (byte *)&MatchFeature;
          currentAdjustmentStorageByte = EEPROM_MATCH_FEATURE_BYTE;
          break;
      }

      Menus.SetParameterControls(   adjustmentType, numAdjustmentValues, adjustmentValues, parameterCallout,
                                    currentAdjustmentStorageByte, currentAdjustmentByte, currentAdjustmentUL );
#ifdef RPU_OS_USE_PINNODE_LINK
      GuardAdjustmentIfLocked(currentAdjustmentStorageByte, currentAdjustmentByte, currentAdjustmentUL);
#endif
    } else if (topLevel==OPERATOR_MENU_GAME_RULES_LEVEL) {
      Audio.PlaySound((unsigned short)subLevel + SOUND_EFFECT_AP_DIFFICULTY, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
      byte *currentAdjustmentByte = &GameRulesSelection;
      byte adjustmentValues[8] = {0};
      adjustmentValues[0] = 0;
      // if one of the below parameters is installed, the "HasParameterChanged" 
      // check below will install Easy / Medium / Hard rules

      switch (subLevel) {
        case 0:
          adjustmentValues[1] = 1;
          break;
        case 1:
          adjustmentValues[1] = 2;
          break;
        case 2:
          adjustmentValues[1] = 3;
          break;
        case 3:
          adjustmentValues[1] = 4;
          break;
      }

      Menus.SetParameterControls(   OPERATOR_MENU_ADJ_TYPE_LIST, 2, adjustmentValues, (short)SOUND_EFFECT_OM_EASY_RULES_INSTRUCTIONS-1,
                                    EEPROM_GAME_RULES_SELECTION, currentAdjustmentByte, NULL );
#ifdef RPU_OS_USE_PINNODE_LINK
      GuardAdjustmentIfLocked(EEPROM_GAME_RULES_SELECTION, currentAdjustmentByte, NULL);
#endif
                  
    } else if (topLevel==OPERATOR_MENU_GAME_ADJ_MENU) {
      Audio.PlaySound((unsigned short)subLevel + SOUND_EFFECT_AP_GAME_ADJUSTMENT_CALLOUTS_START, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);

      byte *currentAdjustmentByte = NULL;
      byte currentAdjustmentStorageByte = 0;
      byte adjustmentValues[8] = {0};
      byte numAdjustmentValues = 2;
      byte adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
      short parameterCallout = 0;
      unsigned long *currentAdjustmentUL = NULL;
      
      adjustmentValues[1] = 1;

      // CUSTOMIZATION TODO:
      // Add adjustment values here for game rules

      switch (subLevel) {
        case OM_GAME_ADJ_TROUGH_EJECT_STRENGTH:
#ifdef MACHINE_BALLY_STAR_TREK
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 3;
          adjustmentValues[1] = 10;
#else
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 6;
          adjustmentValues[0] = 10;
          adjustmentValues[1] = 20;
          adjustmentValues[2] = 30;
          adjustmentValues[3] = 40;
          adjustmentValues[4] = 50;
          adjustmentValues[5] = 60;
#endif
          currentAdjustmentByte = &BallServeSolenoidStrength;
          currentAdjustmentStorageByte = EEPROM_TROUGH_EJECT_STRENGTH;
          break;
        case OM_GAME_ADJ_SAUCER_EJECT_STRENGTH:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 5;
          adjustmentValues[1] = 15;
          currentAdjustmentByte = &SaucerSolenoidStrength;
          currentAdjustmentStorageByte = EEPROM_SAUCER_EJECT_STRENGTH;
          break;
        default:
          // the PinNode adjustments (App starts, PinNode WiFi reset, and any a later library adds)
          PINNODE_ADJUSTMENT_ITEM(subLevel - OM_GAME_ADJ_FINISHED);
          break;
      }
      
      Menus.SetParameterControls(   adjustmentType, numAdjustmentValues, adjustmentValues, parameterCallout,
                                    currentAdjustmentStorageByte, currentAdjustmentByte, currentAdjustmentUL );
#ifdef RPU_OS_USE_PINNODE_LINK
      GuardAdjustmentIfLocked(currentAdjustmentStorageByte, currentAdjustmentByte, currentAdjustmentUL);
#endif
    }    
  }

  boolean parameterChanged = Menus.HasParameterChanged();
  if (parameterChanged) OperatorMenuLastActivity = CurrentTime;
#ifdef RPU_OS_USE_PINNODE_LINK
  // a difficulty setting under a tournament lock goes back to what it was
  if (parameterChanged && RevertGuardedAdjustment()) parameterChanged = false;
#endif
  if (parameterChanged) {
    short parameterCallout = Menus.GetParameterCallout();
    if (parameterCallout) {
      Audio.StopAllAudio();
      Audio.PlaySound((unsigned short)parameterCallout + Menus.GetParameterID(), AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
    }
    if (Menus.GetTopLevel()==OPERATOR_MENU_GAME_RULES_LEVEL) {
      // Install the new rules level
      if (LoadRuleDefaults(GameRulesSelection)) {
        WriteParameters();
      }
    } else if (Menus.GetTopLevel()==OPERATOR_MENU_BASIC_ADJ_MENU) {
      if (Menus.GetSubLevel()==OM_BASIC_ADJ_IDS_MUSIC_VOLUME) {
        if (SoundSettingTimeout) Audio.StopAllAudio();
        Audio.PlaySound(SOUND_EFFECT_BACKGROUND_SONG_1, AUDIO_PLAY_TYPE_WAV_TRIGGER, MusicVolume);
        Audio.SetMusicVolume(MusicVolume);
        SoundSettingTimeout = CurrentTime + 5000;
      } else if (Menus.GetSubLevel()==OM_BASIC_ADJ_IDS_SOUNDFX_VOLUME) {
        if (SoundSettingTimeout) Audio.StopAllAudio();
        Audio.PlaySound(SOUND_EFFECT_SPINNER, AUDIO_PLAY_TYPE_WAV_TRIGGER, SoundEffectsVolume);
        Audio.SetSoundFXVolume(SoundEffectsVolume);
        SoundSettingTimeout = CurrentTime + 5000;
      } else if (Menus.GetSubLevel()==OM_BASIC_ADJ_IDS_CALLOUTS_VOLUME) {
        if (SoundSettingTimeout) Audio.StopAllAudio();
        Audio.PlaySound(SOUND_EFFECT_VP_SHOOT_AGAIN, AUDIO_PLAY_TYPE_WAV_TRIGGER, CalloutsVolume);
        Audio.SetNotificationsVolume(CalloutsVolume);
        SoundSettingTimeout = CurrentTime + 3000;        
      }
    } else if (Menus.GetTopLevel()==OPERATOR_MENU_GAME_ADJ_MENU) {
    }
  }

  if (SoundSettingTimeout && CurrentTime>SoundSettingTimeout) {
    SoundSettingTimeout = 0;
    Audio.StopAllAudio();
  }

  if (SoundTestStart && CurrentTime>SoundTestStart) {
    if (SoundTestSequence==0) {
      PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1);
      SoundTestSequence = 1;
    } else if (SoundTestSequence==1 && CurrentTime>(SoundTestStart+5000)) {
      PlaySoundEffect(SOUND_EFFECT_SPINNER);
      SoundTestSequence = 2;
    } else if (SoundTestSequence==2 && CurrentTime>(SoundTestStart+10000)) {
      Audio.QueuePrioritizedNotification(SOUND_EFFECT_VP_EXTRA_BALL, 0, 10, CurrentTime);
      SoundTestSequence = 3;
    }
  }
  
}



////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////
void PlayBackgroundSong(unsigned int songNum) {

  if (MusicVolume == 0) return;

  Audio.PlayBackgroundSong(songNum);

}


unsigned long NextSoundEffectTime = 0;

void PlaySoundEffect(unsigned int soundEffectNum) {

  if (MachineState == MACHINE_STATE_INIT_GAMEPLAY) return;

  // Play digital samples on the WAV trigger (numbered same
  // as SOUND_EFFECT_ defines)
  Audio.PlaySound(soundEffectNum, AUDIO_PLAY_TYPE_WAV_TRIGGER);

  // SOUND_EFFECT_ defines can also be translated into
  // commands for the sound card
  switch (soundEffectNum) {
      /*
          case SOUND_EFFECT_LEFT_SHOOTER_LANE:
            Audio.PlaySoundCardWhenPossible(12, CurrentTime, 0, 500, 7);
            break;
          case SOUND_EFFECT_RETURN_TO_SHOOTER_LANE:
            Audio.PlaySoundCardWhenPossible(22, CurrentTime, 0, 500, 8);
            break;
          case SOUND_EFFECT_SAUCER:
            Audio.PlaySoundCardWhenPossible(14, CurrentTime, 0, 500, 7);
            break;
          case SOUND_EFFECT_DROP_TARGET_HURRY:
            Audio.PlaySoundCardWhenPossible(2, CurrentTime, 0, 45, 3);
            break;
          case SOUND_EFFECT_DROP_TARGET_COMPLETE:
            Audio.PlaySoundCardWhenPossible(9, CurrentTime, 0, 1400, 4);
            Audio.PlaySoundCardWhenPossible(19, CurrentTime, 1500, 10, 4);
            break;
          case SOUND_EFFECT_HOOFBEATS:
            Audio.PlaySoundCardWhenPossible(12, CurrentTime, 0, 100, 10);
            break;
          case SOUND_EFFECT_STOP_BACKGROUND:
            Audio.PlaySoundCardWhenPossible(19, CurrentTime, 0, 10, 10);
            break;
          case SOUND_EFFECT_DROP_TARGET_HIT:
            Audio.PlaySoundCardWhenPossible(7, CurrentTime, 0, 150, 5);
            break;
          case SOUND_EFFECT_SPINNER:
            Audio.PlaySoundCardWhenPossible(6, CurrentTime, 0, 25, 2);
            break;
      */
  }
}


void QueueNotification(unsigned int soundEffectNum, byte priority) {
  if (CalloutsVolume == 0) return;

  // With RPU_OS_HARDWARE_REV 4 and above, the WAV trigger has two-way communication,
  // so it's not necesary to tell it the length of a notification. For support for
  // earlier hardware, you'll need an array of VoicePromptLengths for each prompt
  // played (for queueing and ducking)
  //  Audio.QueuePrioritizedNotification(soundEffectNum, VoicePromptLengths[soundEffectNum-SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START], priority, CurrentTime);
  Audio.QueuePrioritizedNotification(soundEffectNum, 0, priority, CurrentTime);

}


void AlertPlayerUp() {
  QueueNotification(SOUND_EFFECT_VP_PLAYER_1_UP + CurrentPlayer, 1);
}




////////////////////////////////////////////////////////////////////////////
//
//  Diagnostics Mode
//
////////////////////////////////////////////////////////////////////////////

// The diagnostics state: where the tests asked for from the phone run. The app enters and
// leaves it with the two diagnostics actions; the self-test or credit button at the
// machine leaves it too. After five minutes with no test and no request, the PinNode
// library asks for the exit action itself (when the phone was what entered diagnostics).
int RunDiagnosticsMode(int curState, boolean curStateChanged) {

  int returnState = curState;

#ifdef RPU_OS_USE_PINNODE_LINK
  if (curStateChanged) {
    RPU_DisableSolenoidStack();
    RPU_SetDisableFlippers(true);
    RPU_TurnOffAllLamps();
    Audio.StopAllAudio();
    Display_ClearOverride(0xFF);
    for (byte count = 0; count < RPU_NUMBER_OF_PLAYER_DISPLAYS; count++) RPU_SetDisplayBlank(count, 0x00);
    RPU_SetDisplayCredits(0, false);
    RPU_SetDisplayBallInPlay(0, false);
    RemoteTest = REMOTE_TEST_NONE;
  }

  returnState = HandleOperatorAction(curState);
  if (returnState != curState) return returnState;

  UpdateRemoteTest();

  byte switchHit;
  while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    if (switchHit == SW_SELF_TEST_SWITCH || (switchHit == SW_CREDIT_RESET && RemoteTest != ACT_SWITCH_TEST)) {
      returnState = MACHINE_STATE_ATTRACT;
    }
  }

  if (returnState != curState) EndDiagnostics();
#else
  (void)curStateChanged;
  returnState = MACHINE_STATE_ATTRACT;
#endif

  return returnState;
}




////////////////////////////////////////////////////////////////////////////
//
//  Attract Mode
//
////////////////////////////////////////////////////////////////////////////
byte AttractLastHeadMode = 255;
boolean AttractCheckedForTrappedBall;
unsigned long AttractModeStartTime;

int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  if (curStateChanged) {
    // Some sound cards have a special index
    // for a "sound" that will turn off
    // the current background drone or currently
    // playing sound
    RPU_DisableSolenoidStack();
    RPU_TurnOffAllLamps();
    RPU_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }
    AttractLastHeadMode = 0;
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    Display_ClearOverride(0xFF);
    Display_UpdateDisplays(0xFF);
    AttractCheckedForTrappedBall = false;
    AttractModeStartTime = CurrentTime;

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
    TopperALB.StopAnimation();
    TopperALB.PlayAnimation(GI_0_SET_COLOR, 250, 0, 0);
    TopperALB.PlayAnimation(GI_1_SET_COLOR, 150, 27, 0);
#endif
  }

  if (CurrentTime > (AttractModeStartTime + 5000) && !AttractCheckedForTrappedBall) {
    AttractCheckedForTrappedBall = true;
    if (DEBUG_MESSAGES) {
      Serial.write("In Attract for 10 seconds - make sure there are no balls trapped\n");
    }

    if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
      RPU_PushToSolenoidStack(SOL_SAUCER, SaucerSolenoidStrength, true);
    }
  }

  //RPU_SetLampState(LAMP_APRON_CREDITS, (FreePlayMode || Credits) ? true : false, 0, 200);

  // Claim codes: after a game the node sends a 6-digit code for each player nobody checked
  // in as. The PinNode library shows it in that player's display (the code blinking for
  // 5 seconds, the score for 2, the player-up lamp flashing with the code) and says which
  // displays it has: nothing else draws those on this pass. (It lights its lamp again on
  // every pass, so the attract show turning lamps off doesn't stop the flashing.)
  byte claimDisplays = 0;
#ifdef RPU_OS_USE_PINNODE_LINK
  claimDisplays = PinNode_ShowClaimCodes(CurrentTime, CurrentScores, PlayerUpLamps);
#endif

  // Alternate displays between high score and blank
  if (CurrentTime < 16000) {
    if (AttractLastHeadMode != 1) {
      RPU_SetDisplayCredits(Credits, !FreePlayMode);
      RPU_SetDisplayBallInPlay(0, true);
      Display_ClearOverride(0xFF);
    }
    AttractLastHeadMode = 1;
    for (byte count = 0; count < RPU_NUMBER_OF_PLAYER_DISPLAYS; count++) {
      if (!(claimDisplays & (1 << count))) Display_UpdateDisplays(count);
    }
    RPU_SetLampState(LAMP_HEAD_HIGH_SCORE, 0);
#ifdef RPU_OS_USE_PINNODE_LINK
  // (the high score takes all four displays, so it waits while a claim code is up)
  } else if ((CurrentTime / 8000) % 2 == 0 && !PinNode_ClaimCodesShowing()) {
#else
  } else if ((CurrentTime / 8000) % 2 == 0) {
#endif

    if (AttractLastHeadMode != 2) {
      Display_SetLastTimeScoreChanged(CurrentTime);
    }
    AttractLastHeadMode = 2;
    for (byte count = 0; count < RPU_NUMBER_OF_PLAYER_DISPLAYS; count++) {
      if (!(claimDisplays & (1 << count))) Display_UpdateDisplays(count, false, false, false, HighScore);
    }
    RPU_SetLampState(LAMP_HEAD_HIGH_SCORE, 1);
    for (byte count=0; count<4; count++) {
      // (not a lamp the claim display has: this runs after it in the pass, and would win)
      if (!(claimDisplays & (1 << count))) RPU_SetLampState(PlayerUpLamps[count], 0);
    }
  } else {
    if (AttractLastHeadMode != 3) {
      if (CurrentTime < 32000) {
        for (int count = 0; count < RPU_NUMBER_OF_PLAYERS_ALLOWED; count++) {
          CurrentScores[count] = 0;
        }
        CurrentNumPlayers = 0;
      }
      Display_SetLastTimeScoreChanged(CurrentTime);
    }

    RPU_SetLampState(LAMP_HEAD_HIGH_SCORE, 0);
    for (byte count=0; count<4; count++) {
      if (claimDisplays & (1 << count)) continue;
      if (count<CurrentNumPlayers) Display_UpdateDisplays(count, false, false, false, CurrentScores[count]);
      else if (CurrentNumPlayers==0) Display_UpdateDisplays(count, false, false, false, 0);
      else RPU_SetDisplayBlank(count, 0x00);   // not in the last game: don't leave the high score showing there
    }

    AttractLastHeadMode = 3;
  }

  byte attractPlayfieldPhase = ((CurrentTime / 5000) % 6);

  ShowLampAnimation(attractPlayfieldPhase % 3, 20, CurrentTime, 18, false, false);

#ifdef RPU_OS_USE_PINNODE_LINK
  // Operator actions from the phone: one-shots, and entering diagnostics
  returnState = HandleOperatorAction(curState);

  // Start pressed in the app (after a check-in): a new game, as with the credit button.
  // It spends a credit unless free play is on. With "starts from the app" off, the machine
  // takes none (the library answers for it).
  uint16_t startRequest;
  if (PinNodeCtrl_TakeAppStart(&startRequest, PinNode_AppStartsOn())) {
    if (AddPlayer(true)) {
      returnState = MACHINE_STATE_INIT_GAMEPLAY;
      PinNodeCtrl_StartResult(startRequest, PINNODE_START_OK, 1);
    } else {
      PinNodeCtrl_StartResult(startRequest, PINNODE_START_NO_CREDITS, 0);
    }
  }
#endif

  byte switchHit;
  while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    if (switchHit == SW_CREDIT_RESET) {
      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3 ) {
      AddCoinToAudit(SwitchToChuteNum(switchHit));
      AddCoin(SwitchToChuteNum(switchHit));
#ifdef RPU_OS_USE_PINNODE_LINK
      PinNodeCtrl_Coin(SwitchToChuteNum(switchHit) + 1, Credits);   // credits after the coin
#endif
    }
    if (switchHit == SW_SELF_TEST_SWITCH) {
      Menus.EnterOperatorMenu();
    }
  }

  // If the user was holding the menu button when the game started
  // then kick the balls
  if (CurrentTime < 4000) {
    if (RPU_ReadSingleSwitchState(SW_SELF_TEST_SWITCH)) {
      if (OperatorSwitchPressStarted==0) {
        OperatorSwitchPressStarted = CurrentTime;
      } else if (CurrentTime > (OperatorSwitchPressStarted+500)) {
        Menus.EnterOperatorMenu();
        Menus.BallEjectInProgress(true);
      }
    } else {
      OperatorSwitchPressStarted = 0;
    }
  }

  return returnState;
}





////////////////////////////////////////////////////////////////////////////
//
//  Game Play functions
//
////////////////////////////////////////////////////////////////////////////
byte CountBits(unsigned short intToBeCounted) {
  byte numBits = 0;

  for (byte count = 0; count < 16; count++) {
    numBits += (intToBeCounted & 0x01);
    intToBeCounted = intToBeCounted >> 1;
  }

  return numBits;
}


void SetGameMode(byte newGameMode) {
  LastGameMode = GameMode;
  GameMode = newGameMode;
  GameModeStartTime = 0;
  GameModeEndTime = 0;

  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Game Mode = %d\n", newGameMode);
    Serial.write(buf);
  }
}

byte CountBallsInTrough() {

  byte numBalls = RPU_ReadSingleSwitchState(SW_OUTHOLE);

  return numBalls;
}



void AddToBonus(byte bonus) {
  Bonus[CurrentPlayer] += bonus;
  if (Bonus[CurrentPlayer] > MAX_DISPLAY_BONUS) {
    Bonus[CurrentPlayer] = MAX_DISPLAY_BONUS;
/*    
    // Some games award a special when
    // bonus is maxed out
    if (!SpecialCollected) {
      if (SpecialAvailable) {
        AwardSpecial();  
        SpecialAvailable = false;
      } else {
        SpecialAvailable = true;
      }
    }
*/    
  } else {
    BonusChanged = CurrentTime;
  }
}


void IncreaseBonusX() {
  if (BonusX[CurrentPlayer] < 10) {
    if (BonusX[CurrentPlayer] == 1) {
      BonusX[CurrentPlayer] = 2;
//      QueueNotification(SOUND_EFFECT_VP_BONUS_2X, 5);
    } else if (BonusX[CurrentPlayer] == 2) {
      BonusX[CurrentPlayer] = 3;
//      QueueNotification(SOUND_EFFECT_VP_BONUS_3X, 5);
    } else if (BonusX[CurrentPlayer] == 3) {
      BonusX[CurrentPlayer] = 5;
//      QueueNotification(SOUND_EFFECT_VP_BONUS_5X, 5);
    } else if (BonusX[CurrentPlayer] == 5) {
      BonusX[CurrentPlayer] = 10;
//      QueueNotification(SOUND_EFFECT_VP_BONUS_10X, 5);
    }
    BonusXAnimationStart = CurrentTime;
  }

}



unsigned long GameStartNotificationTime = 0;
boolean WaitForBallToReachOuthole = false;
unsigned long UpperBallEjectTime = 0;

int InitGamePlay(boolean curStateChanged) {

  if (curStateChanged) {
    RPU_TurnOffAllLamps();
    SetGeneralIlluminationOn(true);
    GameStartNotificationTime = CurrentTime;
    Audio.StopAllAudio();
    QueueNotification(SOUND_EFFECT_VP_ADD_PLAYER_1, 10);
    for (byte count = 0; count < RPU_NUMBER_OF_PLAYERS_ALLOWED; count++) RPU_SetDisplayBlank(count, 0x00);
    RPU_SetDisplayCredits(0, false);
    RPU_SetDisplayBallInPlay(1, true);
    NumberOfBallsLocked = 0;
#ifdef RPU_OS_USE_PINNODE_LINK
    PinNodeCtrl_GameStart(Credits, FreePlayMode);   // credits left after the start
    PinNode_ClaimCodesStop();   // the last game's codes are gone: no flashing lamp carries into play
#endif

    // CUSTOMIZATION TODO:
    // Add any game-start parameters here    
  }

  boolean showBIP = (CurrentTime / 100) % 2;
  RPU_SetDisplayBallInPlay(1, showBIP ? true : false);

  if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
    RPU_PushToSolenoidStack(SOL_SAUCER, SaucerSolenoidStrength, true);
  }

  // The start button has been hit only once to get
  // us into this mode, so we assume a 1-player game
  // at the moment
  RPU_SetCoinLockout((Credits >= MaximumCredits) ? true : false);

  // Reset displays & game state variables
  for (int count = 0; count < RPU_NUMBER_OF_PLAYERS_ALLOWED; count++) {
    // Initialize game-specific variables
    Bonus[count] = 0;
    BonusX[count] = 1;
    CurrentAchievements[count] = 0;
    CurrentScores[count] = 0;
    ExtraBallAvailable[count] = 0;
    SpinnerProgress[count] = 0;
  }

  SamePlayerShootsAgain = false;
  CurrentBallInPlay = 1;
  CurrentNumPlayers = 1;
  CurrentPlayer = 0;
  NumberOfBallsInPlay = 0;
  LastTimePopHit = 0;
  LastTiltWarningTime = 0;
  Display_ClearOverride(0xFF);
  Display_UpdateDisplays(0xFF);
  RPU_EnableSolenoidStack();

  return MACHINE_STATE_INIT_NEW_BALL;
}



int InitNewBall(bool curStateChanged) {

  // If we're coming into this mode for the first time
  // then we have to do everything to set up the new ball
  if (curStateChanged) {

    if (GameRulesSelection==GAME_RULES_PROGRESSIVE) {
      // if the operator has installed "progressive" rules, 
      // then we change the rules based on player number.
      // If we're 1 player, it's EASY
      // First player is EASY, last player is HARD,
      // and in-between players are MEDIUM
      byte rulesLevel = GAME_RULES_EASY;
      if (CurrentPlayer==0) rulesLevel = GAME_RULES_EASY;
      else if (CurrentPlayer==(CurrentNumPlayers-1)) rulesLevel = GAME_RULES_HARD;
      else rulesLevel = GAME_RULES_MEDIUM;
      LoadRuleDefaults(rulesLevel);
      if (CurrentBallInPlay==1) QueueNotification(SOUND_EFFECT_VP_GAME_RULES_EASY + (rulesLevel-GAME_RULES_EASY), 10);
    }

    RPU_TurnOffAllLamps();
    BallFirstSwitchHitTime = 0;

    // The bonus countdown turns the solenoid stack off and a tilt turns the flippers off,
    // so every ball turns them back on (as RPU games do; ExampleMachine left this out).
    RPU_SetDisableFlippers(false);
    RPU_EnableSolenoidStack();
#ifdef RPU_OS_USE_PINNODE_LINK
    PinNodeCtrl_BallStart(CurrentPlayer + 1, CurrentBallInPlay);
    PinNodeScoreAtBallStart = CurrentScores[CurrentPlayer];
#endif

    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    if (CurrentNumPlayers > 1 && (CurrentBallInPlay != 1 || CurrentPlayer != 0) && !SamePlayerShootsAgain) AlertPlayerUp();
    SamePlayerShootsAgain = false;

    RPU_SetDisplayBallInPlay(CurrentBallInPlay);
    RPU_SetLampState(LAMP_HEAD_TILT, 0);
    for (byte count = 0; count < 4; count++) {
      if (count==CurrentPlayer) RPU_SetLampState(PlayerUpLamps[count], 1, 0, 250);
      else if (count<CurrentNumPlayers) RPU_SetLampState(PlayerUpLamps[count], 1);
      else RPU_SetLampState(PlayerUpLamps[count], 0);
      RPU_SetDisplayBlank(count, 0);
    }

    if (BallSaveNumSeconds > 0) {
      RPU_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, 500);
    }

    BallSaveUsed = false;
    BallTimeInTrough = 0;
    NumTiltWarnings = 0;

    // Initialize game-specific start-of-ball lights & variables
    GameModeStartTime = 0;
    GameModeEndTime = 0;
    GameMode = GAME_MODE_SKILL_SHOT;

    SpecialCollected = false;
    SpecialAvailable = false;

    PopBumperProgress = 0;
    PlayfieldMultiplier = 1;
    PlayfieldMultiplierTimeLeft = 0;
    BonusXAnimationStart = 0;
    Bonus[CurrentPlayer] = 1;
    BonusX[CurrentPlayer] = 1;
    Display_ResetDisplayTrackingVariables();
    LastTimePopHit = 0;

    SetBallSave(0);

    LastSpinnerHitTime = 0;
    LastTimeSaucerSeen = 0;

    // CUSTOMIZATION TODO:
    // Add any ball start parameters here    

    // Reset Drop Targets
    LeftDropTargets.ResetDropTargets(CurrentTime + 100, true, true);
#ifndef MACHINE_BALLY_STAR_TREK
    RightDropTargets.ResetDropTargets(CurrentTime + 100, true, true);
#endif

    RPU_PushToTimedSolenoidStack(SOL_SERVE_BALL, BallServeSolenoidStrength, CurrentTime + 1000, true);
    LastTimeBallServed = CurrentTime + 1000;
    
    NumberOfBallsInPlay = 1;

    PlayBackgroundSong(SOUND_EFFECT_RALLY_MUSIC_1 + (CurrentBallInPlay-1));
  }

  return MACHINE_STATE_NORMAL_GAMEPLAY;
  
  LastTimeThroughLoop = CurrentTime;
}




unsigned long SaucerClosedStart = 0;

void CheckSaucerForStuckBall() {
  if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
    LastTimeSaucerSeen = CurrentTime;
    if (SaucerClosedStart == 0) {
      SaucerClosedStart = CurrentTime;
    } else {
      if (CurrentTime > (SaucerClosedStart + 1500)) {
        RPU_PushToSolenoidStack(SOL_SAUCER, SaucerSolenoidStrength, true);
      }
    }
  } else {
    SaucerClosedStart = 0;
  }

}

void UpdateDropTargets() {
  LeftDropTargets.Update(CurrentTime);
#ifndef MACHINE_BALLY_STAR_TREK
  RightDropTargets.Update(CurrentTime);
#endif
}



byte GameModeStage;
boolean DisplaysNeedRefreshing = false;
unsigned long LastTimePromptPlayed = 0;
unsigned long LastTimeAwardsChecked = 0;
unsigned long LastTimeJackpotAdjusted = 0;
unsigned long LastLoopTick = 0;



// CUSTOMIZATION TODO:
// Put your game logic here.
// This function manages all timers, flags, and lights
int ManageGameMode() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  boolean specialAnimationRunning = false;
  boolean statusRunning = false;

  UpdateDropTargets();

  if ((CurrentTime - LastSwitchHitTime) > 3000) TimersPaused = true;
  else TimersPaused = false;

  CheckSaucerForStuckBall();

  switch ( GameMode ) {
    case GAME_MODE_SKILL_SHOT:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0;
        LastTimePromptPlayed = CurrentTime;
        GameModeStage = 0;
        SetGeneralIlluminationOn(true);
      }

      // The switch handler will award the skill shot
      // (when applicable) and this mode will move
      // to unstructured play when any valid switch is
      // recorded

      if (CurrentTime > (LastTimePromptPlayed + 20000)) {
        //RPU_FireContinuousSolenoid(0x20, 30);
        AlertPlayerUp();
        LastTimePromptPlayed = CurrentTime;
        Audio.OutputTracksPlaying();
      }
      
      // If we've seen a tilt before plunge, then
      // we can show a countdown timer here
      if (LastTiltWarningTime) {
        if ( CurrentTime > (LastTiltWarningTime + 30000) ) {
          LastTiltWarningTime = 0;
        } else {
          byte secondsSinceWarning = (CurrentTime - LastTiltWarningTime) / 1000;
          for (byte count = 0; count < RPU_NUMBER_OF_PLAYERS_ALLOWED; count++) {
            if (count == CurrentPlayer && !statusRunning) Display_OverrideScoreDisplay(count%RPU_NUMBER_OF_PLAYER_DISPLAYS, 30 - secondsSinceWarning, DISPLAY_OVERRIDE_ANIMATION_CENTER);
          }
          DisplaysNeedRefreshing = true;
        }
      } else if (DisplaysNeedRefreshing) {
        DisplaysNeedRefreshing = false;
        if (!statusRunning) Display_ClearOverride(0xFF);
      } else {
        if ( ((CurrentTime/1000)%10)>7 ) {        
          if (GameModeStage!=2) {

            // This code flutters the player up during skill shot to let them know

            for (byte count = 0; count < RPU_NUMBER_OF_PLAYERS_ALLOWED; count++) {
#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
              if (!statusRunning && count==CurrentPlayer) Display_OverrideScoreDisplay(count%RPU_NUMBER_OF_PLAYER_DISPLAYS, ((unsigned long)CurrentPlayer + 1) * 1111111, DISPLAY_OVERRIDE_ANIMATION_FLUTTER);
#else              
              if (!statusRunning && count==CurrentPlayer) Display_OverrideScoreDisplay(count%RPU_NUMBER_OF_PLAYER_DISPLAYS, ((unsigned long)CurrentPlayer + 1) * 111111, DISPLAY_OVERRIDE_ANIMATION_FLUTTER);
#endif              
            }
            GameModeStage = 2;
          }
        } else {
          if (GameModeStage!=1) {
            if (!statusRunning) Display_ClearOverride(0xFF);
            GameModeStage = 1;
          }
        }
      }

      if (BallFirstSwitchHitTime != 0) {
        Display_ClearOverride(0xFF);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      break;

    case GAME_MODE_UNSTRUCTURED_PLAY:
      // If this is the first time in this mode
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        DisplaysNeedRefreshing = true;
        if (DEBUG_MESSAGES) {
          Serial.write("Entering unstructured play\n");
        }
        SetGeneralIlluminationOn(true);
        unsigned short songNum = SOUND_EFFECT_BACKGROUND_SONG_1;
        if (CurrentBallInPlay>1 && CurrentBallInPlay<5) songNum = SOUND_EFFECT_BACKGROUND_SONG_2 + (CurrentTime%3);
        if (CurrentBallInPlay==BallsPerGame) songNum = SOUND_EFFECT_BACKGROUND_SONG_5;
        if (Audio.GetBackgroundSong() != songNum) {
          PlayBackgroundSong(songNum);
        }
        GameModeStage = 0;
        LastTimePromptPlayed = 0;
      }

      // Display Overrides in Unstructured Play
      if (LastSpinnerHitTime) {

        if (CurrentTime > (LastSpinnerHitTime + 5000)) {
          LastSpinnerHitTime = 0;
        } else {
          if (SpinnerProgress[CurrentPlayer] && SpinnerProgress[CurrentPlayer] < 100) {
            int spinnerHitsRemaining = 100 - SpinnerProgress[CurrentPlayer];
            Display_OverrideScoreDisplay(CurrentPlayer, spinnerHitsRemaining, DISPLAY_OVERRIDE_ANIMATION_CENTER);
          }
        }
        DisplaysNeedRefreshing = true;
      } else if (PlayfieldMultiplier > 1) {
        // Playfield X value is only reset during unstructured play
        if (PlayfieldMultiplierTimeLeft && (CurrentTime > LastLoopTick)) {
          unsigned long numTicks = CurrentTime - LastLoopTick;
          if (numTicks > PlayfieldMultiplierTimeLeft) {
            PlayfieldMultiplierTimeLeft = 0;
            PlayfieldMultiplier = 1;
          } else {
            PlayfieldMultiplierTimeLeft -= numTicks;
          }
        } else {
          for (byte count = 0; count < RPU_NUMBER_OF_PLAYERS_ALLOWED; count++) {
            if (count != CurrentPlayer && !statusRunning) Display_OverrideScoreDisplay(count, PlayfieldMultiplier, DISPLAY_OVERRIDE_SYMMETRIC_BOUNCE);
          }
          DisplaysNeedRefreshing = true;
        }
      } else if (DisplaysNeedRefreshing) {
        DisplaysNeedRefreshing = false;
        Display_ClearOverride(0xFF);
      }

      if (LastTimePopHit) {
        if (CurrentTime < (LastTimePopHit+480) && (PopBumperProgress%10)==0) {
          specialAnimationRunning = true;
          byte popOutPhase = ((CurrentTime - LastTimePopHit)/20);
          ShowLampAnimationSingleStep(4, popOutPhase);
        }
      }

      // Check to see if player has earned an extra ball lamp
      if (CurrentTime > (LastTimeAwardsChecked+1000) && ExtraBallAvailable[CurrentPlayer]==0) {
        LastTimeAwardsChecked = CurrentTime;
        // if extra ball criteria met
//        if () {
//          ExtraBallAvailable[CurrentPlayer] = 1;        
//        }
      }

      break;

  }

  if ( !statusRunning && !specialAnimationRunning && NumTiltWarnings <= MaxTiltWarnings ) {
    // CUSTOMIZATION TODO:
    // add in any custom lamp functions here
    ShowBonusLamps();
    ShowShootAgainLamp();
  }
  ShowPlayerLamps();

  if (Display_UpdateDisplays(0xFF, false, (BallFirstSwitchHitTime == 0) ? true : false, (BallFirstSwitchHitTime > 0 && ((CurrentTime - Display_GetLastTimeScoreChanged()) > 2000)) ? true : false)) {
    Audio.StopSound(SOUND_EFFECT_SCORE_TICK);
    PlaySoundEffect(SOUND_EFFECT_SCORE_TICK);
  }

  // Check to see if ball is in the outhole
  // (not until the serve has had its chance: the ball waits in the outhole to be served,
  // and upstream pulsed the serve coil twice because of it)
  if (CountBallsInTrough() > (TotalBallsLoaded - (NumberOfBallsInPlay + NumberOfBallsLocked)) && CurrentTime > (LastTimeBallServed + 500)) {

    if (BallTimeInTrough == 0) {
      // If this is the first time we're seeing too many balls in the trough, we'll wait to make sure
      // everything is settled
      BallTimeInTrough = CurrentTime;
    } else {

      // Make sure the ball stays on the sensor for at least
      // 0.5 seconds to be sure that it's not bouncing or passing through
      if ((CurrentTime - BallTimeInTrough) > 750) {

        if ((BallFirstSwitchHitTime == 0 && NumTiltWarnings <= MaxTiltWarnings)) {
          // Nothing hit yet, so return the ball to the player
          RPU_PushToTimedSolenoidStack(SOL_SERVE_BALL, BallServeSolenoidStrength, CurrentTime);
          BallTimeInTrough = 0;
          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
        } else {
          // if we haven't used the ball save, and we're under the time limit, then save the ball
          if (BallSaveEndTime && CurrentTime < (BallSaveEndTime + BALL_SAVE_GRACE_PERIOD)) {
            RPU_PushToTimedSolenoidStack(SOL_SERVE_BALL, BallServeSolenoidStrength, CurrentTime + 100);

            RPU_SetLampState(LAMP_SHOOT_AGAIN, 0);
            BallTimeInTrough = CurrentTime;
            returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

            if (NumberOfBallSavesRemaining && NumberOfBallSavesRemaining != 0xFF) {
              NumberOfBallSavesRemaining -= 1;
              if (NumberOfBallSavesRemaining == 0) {
                BallSaveEndTime = 0;
                if (DEBUG_MESSAGES) {
                  Serial.write("Last ball save\n");
                }
                QueueNotification(SOUND_EFFECT_VP_BALL_SAVE, 10);
              } else {
                if (DEBUG_MESSAGES) {
                  Serial.write("Not last ball save\n");
                }                
              }
            }

          } else {

            NumberOfBallsInPlay -= 1;
            if (NumberOfBallsInPlay == 0) {
              Display_ClearOverride(0xFF);
              Audio.StopAllAudio();
              //PlaySoundEffect(SOUND_EFFECT_BALL_OVER);
              returnState = MACHINE_STATE_COUNTDOWN_BONUS;
            }
          }
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  LastLoopTick = CurrentTime;
  LastTimeThroughLoop = CurrentTime;
  return returnState;
}



unsigned long CountdownStartTime = 0;
unsigned long LastCountdownReportTime = 0;
unsigned long BonusCountDownEndTime = 0;
byte DecrementingBonusCounter;
byte IncrementingBonusXCounter;
byte TotalBonus = 0;
byte TotalBonusX = 0;
byte BonusSoundIncrement;
boolean CountdownBonusHurryUp = false;
int LastBonusSoundPlayed = 0;

int CountDownDelayTimes[] = {130, 130, 130, 100, 100, 60, 60, 60, 60, 60};

int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {
    
    // Turn off solenoids
    RPU_DisableSolenoidStack();

    if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
      RPU_PushToSolenoidStack(SOL_SAUCER, SaucerSolenoidStrength, true);
    }

    CountdownStartTime = CurrentTime;
    LastCountdownReportTime = CurrentTime;
    ShowBonusLamps();
    IncrementingBonusXCounter = 1;
    DecrementingBonusCounter = Bonus[CurrentPlayer];
    TotalBonus = Bonus[CurrentPlayer];
    TotalBonusX = BonusX[CurrentPlayer];
    CountdownBonusHurryUp = false;
    BonusSoundIncrement = 0;
    LastBonusSoundPlayed = 0;

    BonusCountDownEndTime = 0xFFFFFFFF;
    // Some sound cards have a special index
    // for a "sound" that will turn off
    // the current background drone or currently
    // playing sound
    //    PlaySoundEffect(SOUND_EFFECT_STOP_BACKGROUND);
  }

  // CUSTOMIZATION TODO:
  // If your game has flipper switches, you can use them to 
  // speed up the bonus countdown
  //if (RPU_ReadSingleSwitchState(SW_LEFT_FLIPPER) && RPU_ReadSingleSwitchState(SW_RIGHT_FLIPPER)) CountdownBonusHurryUp = true;

  unsigned long countdownDelayTime = (unsigned long)(CountDownDelayTimes[IncrementingBonusXCounter - 1]);
  if (CountdownBonusHurryUp && countdownDelayTime > ((unsigned long)CountDownDelayTimes[9])) countdownDelayTime = CountDownDelayTimes[9];

  if ((CurrentTime - LastCountdownReportTime) > countdownDelayTime) {

    if (DecrementingBonusCounter) {

      // Only give sound & score if this isn't a tilt
      if (NumTiltWarnings <= MaxTiltWarnings) {
        int soundToPlay = SOUND_EFFECT_BONUS_1 + (BonusSoundIncrement/4);
        if (soundToPlay>SOUND_EFFECT_BONUS_8) soundToPlay = SOUND_EFFECT_BONUS_8;
        if (LastBonusSoundPlayed!=0) Audio.StopSound(LastBonusSoundPlayed);
        PlaySoundEffect(soundToPlay);
        LastBonusSoundPlayed = soundToPlay;
        BonusSoundIncrement += 1;
        CurrentScores[CurrentPlayer] += 1000;
      }

      DecrementingBonusCounter -= 1;
      Bonus[CurrentPlayer] = DecrementingBonusCounter;
      ShowBonusLamps();

    } else if (BonusCountDownEndTime == 0xFFFFFFFF) {
      IncrementingBonusXCounter += 1;
      if (BonusX[CurrentPlayer] > 1) {
        DecrementingBonusCounter = TotalBonus;
        Bonus[CurrentPlayer] = TotalBonus;
        ShowBonusLamps();
        BonusX[CurrentPlayer] -= 1;
        if (BonusX[CurrentPlayer] == 9) BonusX[CurrentPlayer] = 8;
      } else {
        BonusX[CurrentPlayer] = TotalBonusX;
        Bonus[CurrentPlayer] = TotalBonus;
        BonusCountDownEndTime = CurrentTime + 1000;
      }
    }
    LastCountdownReportTime = CurrentTime;
  }

  if (CurrentTime > BonusCountDownEndTime) {

    if (DEBUG_MESSAGES) {
      Serial.write("Count down over, moving to ball over\n");
    }
    // Reset any lights & variables of goals that weren't completed
    BonusCountDownEndTime = 0xFFFFFFFF;
    return MACHINE_STATE_BALL_OVER;
  }

  return MACHINE_STATE_COUNTDOWN_BONUS;
}



void CheckHighScores() {
  unsigned long highestScore = 0;
  int highScorePlayerNum = 0;
  for (int count = 0; count < CurrentNumPlayers; count++) {
    if (CurrentScores[count] > highestScore) {
      highestScore = CurrentScores[count];
      highScorePlayerNum = count;
    }
  }

  if (highestScore > HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      AddCredit(false, 3);
      RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
    }
    RPU_WriteULToEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, highestScore);
    RPU_WriteULToEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count = 0; count < RPU_NUMBER_OF_PLAYERS_ALLOWED; count++) {
      if (count == highScorePlayerNum) {
        RPU_SetDisplay(count, CurrentScores[count], true, 2);
      } else {
        RPU_SetDisplayBlank(count, 0x00);
      }
    }

    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, KNOCKER_SOLENOID_STRENGTH, CurrentTime, true);
    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, KNOCKER_SOLENOID_STRENGTH, CurrentTime + 300, true);
    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, KNOCKER_SOLENOID_STRENGTH, CurrentTime + 600, true);
  }
}


unsigned long MatchSequenceStartTime = 0;
unsigned long MatchDelay = 150;
byte MatchDigit = 0;
byte NumMatchSpins = 0;
byte ScoreMatches = 0;

int ShowMatchSequence(boolean curStateChanged) {
  if (!MatchFeature) return MACHINE_STATE_ATTRACT;

  if (curStateChanged) {
    MatchSequenceStartTime = CurrentTime;
    MatchDelay = 1500;
    MatchDigit = CurrentTime % 10;
    NumMatchSpins = 0;
    RPU_SetLampState(LAMP_HEAD_MATCH, 1, 0);
    RPU_SetDisableFlippers();
    ScoreMatches = 0;
  }

  if (NumMatchSpins < 40) {

    if (NumMatchSpins<10) {
      byte blankMask = (0x3F << ((NumMatchSpins)/2)) & 0x3F;
      RPU_SetDisplayBlank(0, RPU_GetDisplayBlank(0) & blankMask);
      RPU_SetDisplayBlank(1, RPU_GetDisplayBlank(1) & blankMask);      
    } else if (NumMatchSpins>16 && NumMatchSpins<40) {
      if (CurrentNumPlayers>2) {
        unsigned long scoreMatch1 = CurrentScores[0] % 100;
        unsigned long scoreMatch2 = CurrentScores[1] % 100;
        unsigned long scoreMatch3 = CurrentScores[2] % 100;
        unsigned long scoreMatch4 = CurrentScores[3] % 100;
        unsigned long display1, display2;
        byte display1Mask, display2Mask;
        byte shiftPhase = (NumMatchSpins - 16)/3;
        switch (shiftPhase) {
          case 0:
            display1 = 10*scoreMatch1;
            display1Mask = 0x18;
            display2 = 10*scoreMatch2;
            display2Mask = 0x18;
            break;
          case 1:
            display1 = 100*scoreMatch1;
            display1Mask = 0x0C;
            display2 = 100*scoreMatch2;
            display2Mask = 0x0C;
            break;
          case 2:
            display1 = 1000*scoreMatch1;
            display1Mask = 0x06;
            display2 = 1000*scoreMatch2 + scoreMatch3/10;
            display2Mask = 0x26;
            break;
          case 3:
            display1 = 10000*scoreMatch1;
            display1Mask = 0x03;
            display2 = 10000*scoreMatch2 + scoreMatch3;
            display2Mask = 0x33;
            break;
          case 4:
            display1 = 10000*scoreMatch1;
            display1Mask = 0x03;
            display2 = scoreMatch3 * 10;
            display2Mask = 0x18;
            break;
          case 5:
            display1 = 10000*scoreMatch1;
            display1Mask = 0x03;
            display2 = 100*scoreMatch3;
            display2Mask = 0x0C;
            break;
          case 6:
            display1 = 10000*scoreMatch1 + scoreMatch2/10;
            display1Mask = 0x23;
            display2 = 1000*scoreMatch3;
            display2Mask = 0x06;
            if (CurrentNumPlayers==4) {
              display2 += scoreMatch4 / 10;
              display2Mask = 0x26;
            }
            break;
          case 7:
            display1 = 10000*scoreMatch1 + scoreMatch2;
            display1Mask = 0x33;
            display2 = 10000*scoreMatch3;
            display2Mask = 0x03;
            if (CurrentNumPlayers==4) {
              display2 += scoreMatch4;
              display2Mask = 0x33;
            }            
            break;
        }
        RPU_SetDisplayBlank(0, display1Mask);
        RPU_SetDisplayBlank(1, display2Mask);
        RPU_SetDisplay(0, display1);
        RPU_SetDisplay(1, display2);        
      }
    }
    
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      MatchDigit += 1;
      if (MatchDigit > 9) MatchDigit = 0;
      //PlaySoundEffect(10+(MatchDigit%2));
      PlaySoundEffect(SOUND_EFFECT_MATCH_SPIN);
      RPU_SetDisplayBallInPlay((int)MatchDigit * 10);
      MatchDelay += 50 + 4 * NumMatchSpins;
      NumMatchSpins += 1;
      RPU_SetLampState(LAMP_HEAD_MATCH, NumMatchSpins % 2, 0);

      if (NumMatchSpins == 40) {
        RPU_SetLampState(LAMP_HEAD_MATCH, 0);
        MatchDelay = CurrentTime - MatchSequenceStartTime;
      }
    }
  }

  if (NumMatchSpins >= 40 && NumMatchSpins <= 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      if ( (CurrentNumPlayers > (NumMatchSpins - 40)) && ((CurrentScores[NumMatchSpins - 40] / 10) % 10) == MatchDigit) {
        ScoreMatches |= (1 << (NumMatchSpins - 40));
        AddSpecialCredit();
        MatchDelay += 1000;
        NumMatchSpins += 1;
        RPU_SetLampState(LAMP_HEAD_MATCH, 1);
      } else {
        NumMatchSpins += 1;
      }
      if (NumMatchSpins == 44) {
        MatchDelay += 5000;
      }
    }
  }

  if (NumMatchSpins > 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      return MACHINE_STATE_ATTRACT;
    }
  }

  for (int count = 0; count < RPU_NUMBER_OF_PLAYERS_ALLOWED; count++) {
    if (CurrentNumPlayers<3) {
      if ((ScoreMatches >> count) & 0x01) {
        // If this score matches, we're going to flash the last two digits
        byte upperMask = 0x0F;
        byte lowerMask = 0x30;
        if (RPU_OS_NUM_DIGITS == 7) {
          upperMask = 0x1F;
          lowerMask = 0x60;
        }
        if ( (CurrentTime / 200) % 2 ) {
          RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) & upperMask);
        } else {
          RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) | lowerMask);
        }
      }
    } else {
      // With more than 2 players, the scores will be compressed
      if ((ScoreMatches >> count) & 0x01) {
        byte displayMask0 = 0, displayMask1 = 0;
        if (RPU_OS_NUM_DIGITS == 7) {
          switch (count) {
            case 0: displayMask0 = 0x03; break;
            case 1: displayMask0 = 0x60; break;
            case 2: displayMask1 = 0x03; break;
            case 3: displayMask1 = 0x60; break;
          }
        } else {
          switch (count) {
            case 0: displayMask0 = 0x03; break;
            case 1: displayMask0 = 0x30; break;
            case 2: displayMask1 = 0x03; break;
            case 3: displayMask1 = 0x30; break;
          }
        }
        
        if ( (CurrentTime / 200) % 2 ) {
          RPU_SetDisplayBlank(0, RPU_GetDisplayBlank(0) & ~displayMask0);
          RPU_SetDisplayBlank(1, RPU_GetDisplayBlank(1) & ~displayMask1);
        } else {
          RPU_SetDisplayBlank(0, RPU_GetDisplayBlank(0) | displayMask0);
          RPU_SetDisplayBlank(1, RPU_GetDisplayBlank(1) | displayMask1);
        }
      }
    }
  }

  return MACHINE_STATE_MATCH_MODE;
}




////////////////////////////////////////////////////////////////////////////
//
//  Switch Handling functions
//
////////////////////////////////////////////////////////////////////////////
/*
  // Example lock function

  void HandleLockSwitch(byte lockIndex) {

  if (GameMode==GAME_MODE_UNSTRUCTURED_PLAY) {
    // If this player has a lock available
    if (PlayerLockStatus[CurrentPlayer] & (LOCK_1_AVAILABLE<<lockIndex)) {
      // Lock the ball
      LockBall(lockIndex);
      SetGameMode(GAME_MODE_OFFER_LOCK);
    } else {
      if ((MachineLocks & (LOCK_1_ENGAGED<<lockIndex))==0) {
        // Kick unlocked ball
        RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);
      }
    }
  }
  }

*/

unsigned long LastSelfTestSwitchTime = 0;

int HandleSystemSwitches(int curState, byte switchHit) {
  int returnState = curState;
  switch (switchHit) {
    case SW_SELF_TEST_SWITCH:
      // During a game the menu needs two presses, between 0.3 and 3 seconds apart. The
      // self-test switch is read from an interrupt line, not the switch matrix, and one
      // stray pulse on it used to stop the game and blank the displays.
      if (LastSelfTestSwitchTime != 0 && (CurrentTime - LastSelfTestSwitchTime) > 300 && (CurrentTime - LastSelfTestSwitchTime) < 3000) {
        LastSelfTestSwitchTime = 0;
        Menus.EnterOperatorMenu();
      } else {
        LastSelfTestSwitchTime = CurrentTime;
      }
      break;
    case SW_COIN_1:
    case SW_COIN_2:
    case SW_COIN_3:
      AddCoinToAudit(SwitchToChuteNum(switchHit));
      AddCoin(SwitchToChuteNum(switchHit));
#ifdef RPU_OS_USE_PINNODE_LINK
      PinNodeCtrl_Coin(SwitchToChuteNum(switchHit) + 1, Credits);   // credits after the coin
#endif
      break;
    case SW_CREDIT_RESET:
      if (MachineState == MACHINE_STATE_MATCH_MODE) {
        // If the first ball is over, pressing start again resets the game
        if (Credits >= 1 || FreePlayMode) {
          if (!FreePlayMode) {
            Credits -= 1;
            RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
            RPU_SetDisplayCredits(Credits, !FreePlayMode);
          }
          returnState = MACHINE_STATE_INIT_GAMEPLAY;
        }
      } else {
        CreditResetPressStarted = CurrentTime;
      }
      break;
    case SW_OUTHOLE:
      // Some machines have a kicker to move the ball
      // from the outhole to the re-shooter ramp
      break;
//  case SW_TILT:      
//  case SW_ROLL_TILT:      
    case SW_PLUMB_TILT:
      if (BallFirstSwitchHitTime) {
        if ( CurrentTime > (LastTiltWarningTime + TILT_WARNING_DEBOUNCE_TIME) ) {
          LastTiltWarningTime = CurrentTime;
          NumTiltWarnings += 1;
#ifdef RPU_OS_USE_PINNODE_LINK
          PinNodeCtrl_Tilt(CurrentPlayer + 1, NumTiltWarnings > MaxTiltWarnings ? PINNODE_TILT_TILT : PINNODE_TILT_WARNING);
#endif
          if (NumTiltWarnings > MaxTiltWarnings) {
            SetGeneralIlluminationOn(false);
            RPU_DisableSolenoidStack();
            RPU_SetDisableFlippers(true);
            RPU_TurnOffAllLamps();
            RPU_SetLampState(LAMP_HEAD_TILT, 1);
            Audio.StopAllAudio();
            if (BallSaveEndTime) {
              BallSaveEndTime = 0;
              NumberOfBallSavesRemaining = 0;
            }
            PlaySoundEffect(SOUND_EFFECT_TILT);
          } else {
            PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
          }
        }
      } else {
        // Tilt before ball is plunged -- show a timer in ManageGameMode if desired
        if ( CurrentTime > (LastTiltWarningTime + TILT_WARNING_DEBOUNCE_TIME) ) {
          PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
        }
        LastTiltWarningTime = CurrentTime;
      }
      break;
  }

  return returnState;
}







void HandleLeftDropTarget(byte switchHit) {

  byte result;
  unsigned long numTargetsDown = 0;
  result = LeftDropTargets.HandleDropTargetHit(switchHit);
  numTargetsDown = (unsigned long)CountBits(result);
  boolean soundPlayed = false;
  boolean scoreAdded = false;

  boolean cleared = LeftDropTargets.CheckIfBankCleared();

  if (cleared) {
    LeftDropTargets.ResetDropTargets(CurrentTime + 500, true);
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 5000;
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_COMPLETE);
    soundPlayed = true;
    scoreAdded = true;
  }

  if (!soundPlayed) {
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_SOUND_1);
  }
  if (!scoreAdded) {
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * numTargetsDown * 100;
  }

}


#ifndef MACHINE_BALLY_STAR_TREK
void HandleRightDropTarget(byte switchHit) {

  byte result;
  unsigned long numTargetsDown = 0;
  result = RightDropTargets.HandleDropTargetHit(switchHit);
  numTargetsDown = (unsigned long)CountBits(result);
  boolean soundPlayed = false;
  boolean scoreAdded = false;

  boolean cleared = RightDropTargets.CheckIfBankCleared();

  if (cleared) {
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 5000;
    RightDropTargets.ResetDropTargets(CurrentTime + 500, true);
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_COMPLETE);
    soundPlayed = true;
    scoreAdded = true;
  }

  if (!soundPlayed) {
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_SOUND_1);
  }
  if (!scoreAdded) {
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * numTargetsDown * 100;
  }
}
#endif





void HandleSpinnerProgress() {

  LastSpinnerHitTime = CurrentTime;
  CurrentScores[CurrentPlayer] += 100 * PlayfieldMultiplier;

}


void ValidateAndRegisterPlayfieldSwitch() {
  LastSwitchHitTime = CurrentTime;
  if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
}


// CUSTOMIZATION TODO:
// This is where switch handling happens
void HandleGamePlaySwitches(byte switchHit) {

  switch (switchHit) {

#ifdef MACHINE_BALLY_STAR_TREK
    case SW_LEFT_BUMPER:
    case SW_RIGHT_BUMPER:
#endif
    case SW_POP_BUMPER:
      LastTimePopHit = CurrentTime;
//      PopBumperProgress += 1;
      CurrentScores[CurrentPlayer] += 100 * PlayfieldMultiplier;
      PlaySoundEffect(SOUND_EFFECT_POP_BUMPER);
#ifdef RPU_OS_USE_PINNODE_LINK
      // CUSTOMIZATION TODO:
      // Stats are coalesced by the library, so a call per hit is cheap
      PinNodeCtrl_Stat(CurrentPlayer + 1, STAT_POP_BUMPER_HITS, PINNODE_STAT_ADD, 1);
#endif
      ValidateAndRegisterPlayfieldSwitch();
      break;

    case SW_LEFT_INLANE:
      ValidateAndRegisterPlayfieldSwitch();
      break;

    case SW_RIGHT_INLANE:
      ValidateAndRegisterPlayfieldSwitch();
      break;

    case SW_LEFT_OUTLANE:
      SetBallSave(4000, 0, true); // Add 4 grace seconds to ball save if it's still on
      ValidateAndRegisterPlayfieldSwitch();
      break;

    case SW_RIGHT_OUTLANE:
      SetBallSave(4000, 0, true); // Add 4 grace seconds to ball save if it's still on
      ValidateAndRegisterPlayfieldSwitch();
      break;

    case SW_LEFT_SLING:
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10;
      PlaySoundEffect(SOUND_EFFECT_LEFT_SLING);
      if (GameMode!=GAME_MODE_SKILL_SHOT) ValidateAndRegisterPlayfieldSwitch();
      break;

    case SW_RIGHT_SLING:
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10;
      PlaySoundEffect(SOUND_EFFECT_RIGHT_SLING);
      if (GameMode!=GAME_MODE_SKILL_SHOT) ValidateAndRegisterPlayfieldSwitch();
      break;

    case SW_DROP_L_1:
    case SW_DROP_L_2:
    case SW_DROP_L_3:
#ifdef MACHINE_BALLY_STAR_TREK
    case SW_DROP_L_4:
#endif
      HandleLeftDropTarget(switchHit);
      ValidateAndRegisterPlayfieldSwitch();
      break;

#ifdef MACHINE_BALLY_STAR_TREK
    // the rest of the Star Trek's playfield: 100 points each in this nothing game
    case SW_A_ROLLOVER:
    case SW_B_ROLLOVER:
    case SW_L_TARGET:
    case SW_2L_TARGET:
    case SW_Y_TARGET:
    case SW_LEFT_MIDDLE_LANE:
    case SW_RIGHT_MIDDLE_LANE:
    case SW_LOWER_HS_ROLLOVER:
    case SW_UPPER_HS_ROLLOVER:
    case SW_UPPER_500_TARGET:
    case SW_3K_TARGET:
    case SW_ADV_BONUS_BUMPERS:
      CurrentScores[CurrentPlayer] += 100 * PlayfieldMultiplier;
      ValidateAndRegisterPlayfieldSwitch();
      break;
#else
    case SW_DROP_R_1:
    case SW_DROP_R_2:
    case SW_DROP_R_3:
      HandleRightDropTarget(switchHit);
      ValidateAndRegisterPlayfieldSwitch();
      break;

    case SW_LEFT_SPINNER:
      HandleSpinnerProgress();
      ValidateAndRegisterPlayfieldSwitch();
      break;

    case SW_RIGHT_SPINNER:
      HandleSpinnerProgress();
      ValidateAndRegisterPlayfieldSwitch();
      break;
#endif

    case SW_SAUCER:
      // Saucer kitckout is now handled by debouncer
      // We'll validate the PF with this switch, unless
      // we just fired the Saucer as part of the ball search.
      LastTimeSaucerSeen = CurrentTime;
      ValidateAndRegisterPlayfieldSwitch();
      break;


  }

}


int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];

  // Very first time into gameplay loop
  if (curState == MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitGamePlay(curStateChanged);
  } else if (curState == MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged);
  } else if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = ManageGameMode();
  } else if (curState == MACHINE_STATE_COUNTDOWN_BONUS) {
    Display_ClearOverride(0xFF);
    Display_UpdateDisplays(0xFF, true);
    returnState = CountdownBonus(curStateChanged);
  } else if (curState == MACHINE_STATE_BALL_OVER) {
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
#ifdef RPU_OS_USE_PINNODE_LINK
    // A saved ball never gets here (ManageGameMode re-serves it), so this is the real end of
    // the ball, after the bonus count. Stats sent before BallEnd go out ahead of it.
    PinNodeCtrl_Stat(CurrentPlayer + 1, STAT_BEST_BALL, PINNODE_STAT_MAX, CurrentScores[CurrentPlayer] - PinNodeScoreAtBallStart);
    PinNodeCtrl_BallEnd(CurrentPlayer + 1, CurrentBallInPlay, CurrentScores[CurrentPlayer]);
#endif

    if (SamePlayerShootsAgain) {
      QueueNotification(SOUND_EFFECT_VP_SHOOT_AGAIN, 10);
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {

      CurrentPlayer += 1;
      if (CurrentPlayer >= CurrentNumPlayers) {
        CurrentPlayer = 0;
        CurrentBallInPlay += 1;
      }

      scoreAtTop = CurrentScores[CurrentPlayer];

      if (CurrentBallInPlay > BallsPerGame) {
#ifdef RPU_OS_USE_PINNODE_LINK
        PinNodeCtrl_GameEnd(CurrentScores, CurrentNumPlayers, PINNODE_END_NORMAL);
        // CUSTOMIZATION TODO:
        // Award end-of-game achievements here (they may follow GameEnd)
        for (byte count = 0; count < CurrentNumPlayers; count++) PinNodeCtrl_Achievement(count + 1, ACH_FIRST_GAME);
#endif
        CheckHighScores();
        PlaySoundEffect(SOUND_EFFECT_GAME_OVER);
        for (int count = 0; count < CurrentNumPlayers; count++) {
          RPU_SetDisplay(count, CurrentScores[count], true, 2);
        }

        for (byte count = 0; count < 4; count++) {
          RPU_SetLampState(PlayerUpLamps[count], 0);
        }

        if (MatchEnabled) {
          returnState = MACHINE_STATE_MATCH_MODE;
        } else {
          returnState = MACHINE_STATE_ATTRACT;
        }
      }
      else returnState = MACHINE_STATE_INIT_NEW_BALL;
    }
  } else if (curState == MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);
  }

  byte switchHit;
  unsigned long lastBallFirstSwitchHitTime = BallFirstSwitchHitTime;

  while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    returnState = HandleSystemSwitches(curState, switchHit);
    if (NumTiltWarnings <= MaxTiltWarnings) HandleGamePlaySwitches(switchHit);
  }

#ifdef RPU_OS_USE_PINNODE_LINK
  // Start pressed in the app during a game: add a player, as with the credit button
  uint16_t startRequest;
  if (PinNodeCtrl_TakeAppStart(&startRequest, PinNode_AppStartsOn())) {
    if (CurrentBallInPlay >= 2) PinNodeCtrl_StartResult(startRequest, PINNODE_START_NOT_NOW, 0);
    else if (CurrentNumPlayers >= RPU_NUMBER_OF_PLAYERS_ALLOWED) PinNodeCtrl_StartResult(startRequest, PINNODE_START_MAX_PLAYERS, 0);
    else if (AddPlayer()) PinNodeCtrl_StartResult(startRequest, PINNODE_START_OK, CurrentNumPlayers);
    else PinNodeCtrl_StartResult(startRequest, PINNODE_START_NO_CREDITS, 0);
  }
#endif

  if (CreditResetPressStarted) {
    if (CurrentBallInPlay < 2) {
      // If we haven't finished the first ball, we can add players
      AddPlayer();
      if (DEBUG_MESSAGES) {
        Serial.write("Start game button pressed\n\r");
      }
      CreditResetPressStarted = 0;
    } else {
      if (RPU_ReadSingleSwitchState(SW_CREDIT_RESET)) {
        if (TimeRequiredToResetGame != 99 && (CurrentTime - CreditResetPressStarted) >= ((unsigned long)TimeRequiredToResetGame * 1000)) {
          // If the first ball is over, pressing start again resets the game
          if (Credits >= 1 || FreePlayMode) {
            if (!FreePlayMode) {
              Credits -= 1;
              RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
              RPU_SetDisplayCredits(Credits, !FreePlayMode);
            }
#ifdef RPU_OS_USE_PINNODE_LINK
            // The game is abandoned for a new one: close it so the node doesn't wait for it
            PinNodeCtrl_GameEnd(CurrentScores, CurrentNumPlayers, PINNODE_END_RESET);
#endif
            returnState = MACHINE_STATE_INIT_GAMEPLAY;
            CreditResetPressStarted = 0;
          }
        }
      } else {
        CreditResetPressStarted = 0;
      }
    }

  }

  // (with ball save off there is no grace period either)
  if (lastBallFirstSwitchHitTime == 0 && BallFirstSwitchHitTime != 0 && BallSaveNumSeconds > 0) {
    BallSaveEndTime = BallFirstSwitchHitTime + ((unsigned long)BallSaveNumSeconds) * 1000;
    NumberOfBallSavesRemaining = 1;
  }
  if (CurrentTime > (BallSaveEndTime + BALL_SAVE_GRACE_PERIOD)) {
    BallSaveEndTime = 0;
    NumberOfBallSavesRemaining = 0;
  }

  if (!ScrollingScores && CurrentScores[CurrentPlayer] > RPU_OS_MAX_DISPLAY_SCORE) {
    CurrentScores[CurrentPlayer] -= RPU_OS_MAX_DISPLAY_SCORE;
    if (!TournamentScoring) AddSpecialCredit();
  }

  if (scoreAtTop != CurrentScores[CurrentPlayer]) {
    Display_SetLastTimeScoreChanged(CurrentTime);
    if (!TournamentScoring) {
      for (int awardCount = 0; awardCount < 3; awardCount++) {
        if (AwardScores[awardCount] != 0 && scoreAtTop < AwardScores[awardCount] && CurrentScores[CurrentPlayer] >= AwardScores[awardCount]) {
          // Player has just passed an award score, so we need to award it
          if (((ScoreAwardReplay >> awardCount) & 0x01)) {
            AddSpecialCredit();
          } else {
            AwardExtraBall(true);
          }
        }
      }
    }

  }

  return returnState;
}


#if (RPU_MPU_ARCHITECTURE>=10)
unsigned long LastLEDUpdateTime = 0;
byte LEDPhase = 0;
#endif

#ifdef DEBUG_SHOW_LOOPS_PER_SECOND
unsigned long NumLoops = 0;
unsigned long LastLoopReportTime = 0;
#endif

void loop() {

  CurrentTime = millis();
  int newMachineState = MachineState;
  
#ifdef DEBUG_SHOW_LOOPS_PER_SECOND
  NumLoops += 1;
  if (LastLoopReportTime==0) LastLoopReportTime = CurrentTime;
  if (CurrentTime>(LastLoopReportTime+1000)) {
    LastLoopReportTime = CurrentTime;
    char buf[128];
    sprintf(buf, "Loop running at %lu Hz\n", NumLoops);
    Serial.write(buf);
    NumLoops = 0;
  }
#endif

  if (Menus.OperatorMenusActive()) {
    RunOperatorMenu();
  } else {
    if (RestoreBackgroundTrack!=BACKGROUND_TRACK_NONE) {
      PlayBackgroundSong(RestoreBackgroundTrack);
      RestoreBackgroundTrack = BACKGROUND_TRACK_NONE;
    }
    if (MachineState < 0) {
      newMachineState = 0;
    } else if (MachineState == MACHINE_STATE_ATTRACT) {
      newMachineState = RunAttractMode(MachineState, MachineStateChanged);
    } else if (MachineState == MACHINE_STATE_DIAGNOSTICS) {
      newMachineState = RunDiagnosticsMode(MachineState, MachineStateChanged);
    } else {
      newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
    }  
  }
  
  if (newMachineState != MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  RPU_Update(CurrentTime);
  Audio.Update(CurrentTime);

#ifdef RPU_OS_USE_PINNODE_LINK
  // PinNode: run the link (a frame every 5 ms; keep every pass of loop() well under 100 ms),
  // report the mode for the app, and service HELLO, settings, audits and app requests.
  RPULink_Update(CurrentTime);
  {
    boolean inGame = (MachineState > MACHINE_STATE_ATTRACT && MachineState != MACHINE_STATE_DIAGNOSTICS);
    byte mode = (Menus.OperatorMenusActive() || MachineState == MACHINE_STATE_DIAGNOSTICS) ? PINNODE_MODE_SERVICE
              : inGame ? PINNODE_MODE_GAME : PINNODE_MODE_ATTRACT;
    // (during the match sequence the ball number has already moved past the last ball)
    PinNodeCtrl_Mode(mode, inGame ? ((CurrentBallInPlay > BallsPerGame) ? BallsPerGame : CurrentBallInPlay) : 0, inGame ? CurrentPlayer + 1 : 0);
  }
  // Credits added from the app (the owner's "Add credit"): in attract only, the game's own
  // way (saved, shown, coin lockout), and never past the maximum.
  uint16_t creditRequest;
  byte creditsAsked;
  if (PinNodeCtrl_TakeCreditAdd(&creditRequest, &creditsAsked)) {
    if (MachineState != MACHINE_STATE_ATTRACT) {
      PinNodeCtrl_CreditResult(creditRequest, PINNODE_CREDIT_NOT_NOW, 0, Credits);
    } else if (creditsAsked == 0 || (unsigned int)Credits + creditsAsked > MaximumCredits) {
      PinNodeCtrl_CreditResult(creditRequest, PINNODE_CREDIT_OVER_MAX, 0, Credits);
    } else {
      AddCredit(false, creditsAsked);
      PinNodeCtrl_CreditResult(creditRequest, PINNODE_CREDIT_OK, creditsAsked, Credits);
    }
  }
  // the machine's credits, for the app: shown there, and no Start button at none
  PinNodeCtrl_Credits(Credits, FreePlayMode);
  PinNodeCtrl_Update(CurrentTime, false);   // no Mega firmware update over the WiFi socket
#endif

#if (RPU_MPU_ARCHITECTURE>=10)
  if (LastLEDUpdateTime == 0 || (CurrentTime - LastLEDUpdateTime) > 250) {
    LastLEDUpdateTime = CurrentTime;
    RPU_SetBoardLEDs((LEDPhase % 8) == 1 || (LEDPhase % 8) == 3, (LEDPhase % 8) == 5 || (LEDPhase % 8) == 7);
    LEDPhase += 1;
  }
#endif

}
