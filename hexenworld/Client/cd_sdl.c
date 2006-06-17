/*
	cd_sdl.c
	$Id: cd_sdl.c,v 1.10 2006-06-17 16:36:11 sezero Exp $

	Copyright (C) 1996-1997  Id Software, Inc.
	Taken from the Twilight project with modifications
	to make it work with Hexen II: Hammer of Thyrion.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		51 Franklin St, Fifth Floor,
		Boston, MA  02110-1301  USA
*/


#include "cd_unix.h"

#ifdef	__USE_SDL_CDROM__

#include "sdl_inc.h"
#include "quakedef.h"

static qboolean cdValid = false;
static qboolean	playing = false;
static qboolean	wasPlaying = false;
static qboolean	enabled = true;
static qboolean playLooping = false;
static byte	remap[100];
static byte	playTrack;
static double	endOfTrack = -1.0, pausetime = -1.0;
static SDL_CD	*cd_handle;
static int	cd_dev = -1;
static float	cdvolume = 1.0;


static void CDAudio_Eject(void)
{
	if (!cd_handle || !enabled)
		return;

	if (SDL_CDEject(cd_handle) < 0)
		Con_Printf ("Unable to eject CD-ROM: %s\n", SDL_GetError ());
}

static int CDAudio_GetAudioDiskInfo(void)
{
	cdValid = false;

	if (!cd_handle)
		return -1;

	if ( ! CD_INDRIVE(SDL_CDStatus(cd_handle)) )
		return -1;

	cdValid = true;

	return 0;
}

void CDAudio_Play(byte track, qboolean looping)
{
	int	len_m, len_s, len_f;

	if (!cd_handle || !enabled)
		return;

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
			return;
	}

	track = remap[track];

	if (track < 1 || track > cd_handle->numtracks)
	{
		Con_Printf ("CDAudio_Play: Bad track number %d.\n", track);
		return;
	}

	if (cd_handle->track[track-1].type == SDL_DATA_TRACK)
	{
		Con_Printf ("CDAudio_Play: track %d is not audio\n", track);
		return;
	}

	if (playing)
	{
		if (playTrack == track)
			return;
		CDAudio_Stop ();
	}

	if (SDL_CDPlay(cd_handle, cd_handle->track[track-1].offset, cd_handle->track[track-1].length) < 0)
	{
		// ok, check for status now
		int cd_status = SDL_CDStatus(cd_handle);

		if (cd_status > 0)
			Con_Printf ("CDAudio_Play: Unable to play %d: %s\n", track, SDL_GetError ());
		return;
	}

	playLooping = looping;
	playTrack = track;
	playing = true;
	FRAMES_TO_MSF(cd_handle->track[track-1].length, &len_m, &len_s, &len_f);
	endOfTrack = realtime + ((double)len_m * 60.0) + (double)len_s + (double)len_f / (double)CD_FPS;

	/*
	 * Add the pregap for the next track.  This means that disc-at-once CDs
	 * won't loop smoothly, but they wouldn't anyway so it doesn't really
	 * matter.  SDL doesn't give us pregap information anyway, so you'll
	 * just have to live with it.
	 */
	endOfTrack += 2.0;
	pausetime = -1.0;
}

void CDAudio_Stop(void)
{
	if (!cd_handle || !enabled)
		return;

	if (!playing)
		return;

	if (SDL_CDStop(cd_handle) < 0)
		Con_Printf ("CDAudio_Stop: Unable to stop CD-ROM (%s)\n", SDL_GetError());

	wasPlaying = false;
	playing = false;
	pausetime = -1.0;
	endOfTrack = -1.0;
}

void CDAudio_Pause(void)
{
	if (!cd_handle || !enabled)
		return;

	if (!playing)
		return;

	if (SDL_CDPause(cd_handle) < 0)
		Con_Printf ("Unable to pause CD-ROM: %s\n", SDL_GetError());

	wasPlaying = playing;
	playing = false;
	pausetime = realtime;
}

void CDAudio_Resume(void)
{
	if (!cd_handle || !enabled)
		return;

	if (!cdValid)
		return;

	if (!wasPlaying)
		return;

	if (SDL_CDResume(cd_handle) < 0)
		Con_Printf ("Unable to resume CD-ROM: %s\n", SDL_GetError());
	playing = true;
	endOfTrack += realtime - pausetime;
	pausetime = -1.0;
}

static void CD_f (void)
{
	char	*command;
	int		ret, n;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("commands:");
		Con_Printf("on, off, reset, remap, \n");
		Con_Printf("play, stop, loop, pause, resume\n");
		Con_Printf("eject, info\n");
		return;
	}

	command = Cmd_Argv (1);

	if (Q_strcasecmp(command, "on") == 0)
	{
		enabled = true;
		return;
	}

	if (Q_strcasecmp(command, "off") == 0)
	{
		if (playing)
			CDAudio_Stop();
		enabled = false;
		return;
	}

	if (Q_strcasecmp(command, "reset") == 0)
	{
		enabled = true;
		if (playing)
			CDAudio_Stop();
		for (n = 0; n < 100; n++)
			remap[n] = n;
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if (Q_strcasecmp(command, "remap") == 0)
	{
		ret = Cmd_Argc () - 2;
		if (ret <= 0)
		{
			for (n = 1; n < 100; n++)
				if (remap[n] != n)
					Con_Printf ("  %u -> %u\n", n, remap[n]);
			return;
		}
		for (n = 1; n <= ret; n++)
			remap[n] = atoi(Cmd_Argv (n + 1));
		return;
	}

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo ();
		if (!cdValid)
		{
			Con_Printf("No CD in player.\n");
			return;
		}
	}

	if (Q_strcasecmp(command, "play") == 0)
	{
		CDAudio_Play((byte)atoi(Cmd_Argv (2)), false);
		return;
	}

	if (Q_strcasecmp(command, "loop") == 0)
	{
		CDAudio_Play((byte)atoi(Cmd_Argv (2)), true);
		return;
	}

	if (Q_strcasecmp(command, "stop") == 0)
	{
		CDAudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "pause") == 0)
	{
		CDAudio_Pause();
		return;
	}

	if (Q_strcasecmp(command, "resume") == 0)
	{
		CDAudio_Resume();
		return;
	}

	if (Q_strcasecmp(command, "eject") == 0)
	{
		if (playing)
			CDAudio_Stop();
		CDAudio_Eject();
		cdValid = false;
		return;
	}

	if (Q_strcasecmp(command, "info") == 0)
	{
		int	current_min, current_sec, current_frame;
		int	length_min, length_sec, length_frame;

		Con_Printf ("%u tracks\n", cd_handle->numtracks);

		if (playing)
			Con_Printf ("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		else if (wasPlaying)
			Con_Printf ("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);

		if (playing || wasPlaying)
		{
			SDL_CDStatus(cd_handle);
			FRAMES_TO_MSF(cd_handle->cur_frame, &current_min, &current_sec, &current_frame);
			FRAMES_TO_MSF(cd_handle->track[playTrack-1].length, &length_min, &length_sec, &length_frame);

			Con_Printf ("Current position: %d:%02d.%02d (of %d:%02d.%02d)\n",
						current_min, current_sec, current_frame * 60 / CD_FPS,
						length_min, length_sec, length_frame * 60 / CD_FPS);
		}
		Con_Printf ("Volume is %f\n", cdvolume);

		return;
	}
}

void CDAudio_Update(void)
{
	CDstatus	curstat;

	if (!cd_handle || !enabled)
		return;

	// if SDL supports cdrom volume control some day
	// I'll write proper code in here
	if (bgmvolume.value != cdvolume)
	{
		cdvolume = bgmvolume.value;
		if (cdvolume == 0)
			CDAudio_Pause();
		else
			CDAudio_Resume();

		return;
	}

	if (playing && realtime > endOfTrack)
	{
	//	curstat = cd_handle->status;
		curstat = SDL_CDStatus(cd_handle);
		if (curstat != CD_PLAYING && curstat != CD_PAUSED)
		{
			playing = false;
			endOfTrack = -1.0;
			if (playLooping)
				CDAudio_Play(playTrack, true);
		}
	}
}

int CDAudio_Init(void)
{
	int	i, x, sdl_num_drives;

	if (COM_CheckParm("-nocdaudio"))
		return -1;

	if (SDL_InitSubSystem(SDL_INIT_CDROM) < 0)
	{
		Con_Printf("Couldn't init SDL cdrom: %s\n", SDL_GetError());
		return -1;
	}

	sdl_num_drives = SDL_CDNumDrives ();
	Con_Printf ("SDL detected %d CD-ROM drive%c\n", sdl_num_drives,
					sdl_num_drives == 1 ? ' ' : 's');

	if (sdl_num_drives < 1)
		return -1;

	if ((i = COM_CheckParm("-cddev")) != 0 && i < com_argc - 1)
	{
		for (x = 0; x < sdl_num_drives; x++)
		{
			if (!Q_strcasecmp(SDL_CDName(x), com_argv[i+1]))
			{
				cd_dev = x;
				break;
			}
		}
		if (cd_dev == -1)
		{
			Con_Printf("SDL couldn't find cdrom device %s\n", com_argv[i+1]);
			return -1;
		}
	}

	if (cd_dev == -1)
		cd_dev = 0;	// default drive

	cd_handle = SDL_CDOpen(cd_dev);
	if (!cd_handle)
	{
		Con_Printf ("CDAudio_Init: Unable to open CD-ROM drive %d (%s)\n",
					SDL_CDName(cd_dev), SDL_GetError());
		return -1;
	}

	for (i = 0; i < 100; i++)
		remap[i] = i;
	enabled = true;
	cdvolume = bgmvolume.value;

	Con_Printf("CDAudio initialized (SDL, using %s)\n", SDL_CDName(cd_dev));

	if (CDAudio_GetAudioDiskInfo())
	{
		Con_Printf("CDAudio_Init: No CD in drive\n");
		cdValid = false;
	}

	Cmd_AddCommand ("cd", CD_f);

	return 0;
}

void CDAudio_Shutdown(void)
{
	if (!cd_handle)
		return;
	CDAudio_Stop();
	SDL_CDClose(cd_handle);
	cd_handle = NULL;
	cd_dev = -1;
	SDL_QuitSubSystem(SDL_INIT_CDROM);
}

#endif	// __USE_SDL_CDROM__

