#pragma once

namespace Game_GTASA
{
	class SCMlimits
	{
	public:
		static unsigned int iMAINsegmentSize;
		static unsigned int iMissionSize;

		static unsigned __int8* CTheScripts__ScriptSpace;
	
		// Returns script space size
		static unsigned int GetScriptSpaceSize();

		// Sets max size of main.scm
		static void SetMaxMAINsegmentSize(int maxMAINsegmentSize);

		// Sets max size of mission
		static void SetMaxMissionSize(int iMaxMissionSize);

		// Patches script space limits, should be executed when limits are set.
		static void PatchScriptLimits();
	};
}