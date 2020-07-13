#include "precompiled.h"
#include "VMProtectDefs.h"
#include "Keys.h"


#ifdef _DEBUG
#include <fstream>
//#define DEBUG_FILE

#ifdef ANIMATIONS_DEBUG
std::ofstream animationsfile;

void LagCompensation::WriteToAnimationFile(char* str)
{
	if (!animationsfile.is_open())
	{
		animationsfile.open("G:\\debug_an.txt", std::ios::out);
	}

	if (animationsfile.is_open())
	{
		animationsfile << str << std::endl;
	}
}


void LagCompensation::OutputAnimations(CustomPlayer* const pCPlayer, CBaseEntity* const Entity)
{
	if (!animationsfile.is_open())
	{
		animationsfile.open("G:\\debug_an.txt", std::ios::out);
	}

	if (animationsfile.is_open())
	{
#if 1
		bool update = false;
#else
		static float lastnextupdatetime = 0.0f;
		DWORD cl = GetServerClientEntity(1);
		CBaseEntity *pEnt = ServerClientToEntity(cl);
		float nextupdatetime = pEnt->GetNextLowerBodyyawUpdateTimeServer();
		bool update = false;

		if (nextupdatetime != lastnextupdatetime)
		{
			animationsfile << "Server LBY Update\n";
			lastnextupdatetime = nextupdatetime;
			update = true;
		}
#endif

		int numoverlays = Entity->GetNumAnimOverlays();

		for (int i = 0; i < numoverlays; i++)
		{
			C_AnimationLayer *layer = Entity->GetAnimOverlay(i);
			C_AnimationLayer *oldlayer = &pCPlayer->PersistentData.lastoutput_layers[i];

#if 1
			if (layer->_m_nSequence != oldlayer[i]._m_nSequence)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Sequence: " << oldlayer[i]._m_nSequence << " -> " << layer->_m_nSequence << " Activity: " << GetSequenceActivityNameForModel(Entity->GetModelPtr(), oldlayer[i]._m_nSequence) << " -> " << GetSequenceActivityNameForModel(Entity->GetModelPtr(), layer->_m_nSequence) << "\n";
				update = true;
			}
#endif

			if (layer->_m_flCycle != oldlayer[i]._m_flCycle)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Cycle: " << oldlayer[i]._m_flCycle << " -> " << layer->_m_flCycle << "\n";
				update = true;
			}

			if (layer->_m_flPlaybackRate != oldlayer[i]._m_flPlaybackRate)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Playback: " << oldlayer[i]._m_flPlaybackRate << " -> " << layer->_m_flPlaybackRate << "\n";
				update = true;
			}

			if (layer->m_nOrder != oldlayer[i].m_nOrder)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Order: " << oldlayer[i].m_nOrder << " -> " << layer->m_nOrder << "\n";
				update = true;
			}

			if (layer->m_nInvalidatePhysicsBits != oldlayer[i].m_nInvalidatePhysicsBits)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Physics: " << oldlayer[i].m_nInvalidatePhysicsBits << " -> " << layer->m_nInvalidatePhysicsBits << "\n";
				update = true;
			}

			if (layer->m_flWeight != oldlayer[i].m_flWeight)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Weight: " << oldlayer[i].m_flWeight << " -> " << layer->m_flWeight << "\n";
				update = true;
			}

			if (layer->m_flLayerAnimtime != oldlayer[i].m_flLayerAnimtime)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " AnimTime: " << oldlayer[i].m_flLayerAnimtime << " -> " << layer->m_flLayerAnimtime << "\n";
				update = true;
			}

			if (update)
				*oldlayer = *layer;
		}

		if (update)
			animationsfile << "\n" << std::endl;

		animationsfile.flush();
	}
}
#endif

#ifdef OUTPUT_SEQUENCE_ACTIVITY_NAMES
void LagCompensation::OutputSequencesAndAnimations(CBaseEntity* Entity)
{
	std::ofstream output_animations("G:\\sequence_names.txt", std::ios::trunc);
	if (output_animations.is_open())
	{
		output_animations << "enum SequenceNames : int\n{\n";
		for (int i = 0; ; i++)
		{
			const char* name = GetSequenceName(Entity, i);
			output_animations << name;
			if (!strcmp(name, "Unknown"))
			{
				output_animations << "\n";
				break;
			}
			output_animations << " = " << i << ",\n";
		}
		output_animations << "};" << std::endl;
	}
	std::ofstream output_sequence_activities("G:\\sequence_activities.txt", std::ios::trunc);
	if (output_sequence_activities.is_open())
	{
		output_sequence_activities << "Sequence Activities:\n";
		for (int i = 0; i < 1024; i++)
		{
			output_sequence_activities << i << " = " << GetSequenceActivity(Entity, i) << "\n";
		}
	}
	std::ofstream output_sequence_activity_names("G:\\sequence_activity_names.txt", std::ios::trunc);
	if (output_sequence_activity_names.is_open())
	{
		output_sequence_activity_names << "Sequence Activity Names:\n";
		for (int i = 0; i < 1024; i++)
		{
			const char* name = GetSequenceActivityNameForModel(Entity->GetModelPtr(), i);
			if (name)
			{
				output_sequence_activity_names << i << " = " << name << "\n";
			}
			else
			{
				output_sequence_activity_names << i << " = " << "Unknown" << "\n";
			}
		}
		output_sequence_activity_names.close();
	}
	std::ofstream activity_list("G:\\activity_list.txt", std::ios::trunc);
	if (activity_list.is_open())
	{
		activity_list << "enum Activities : int\n{\nACT_INVALID = -1,\n";
		for (int i = 0; ; i++)
		{
			const char* name = ActivityList_NameForIndex(i);
			if (!name)
			{
				activity_list << "ACTIVITY_LIST_MAX\n";
				break;
			}
			activity_list << name << " = " << i << ",\n";
		}
		activity_list << "};" << std::endl;
	}
}
#endif

#endif
