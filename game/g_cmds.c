/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "g_local.h"
#include "m_player.h"


char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = FindItem("Jacket Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.WriteShort(cl->pers.inventory[i]);
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
}


int PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void Cmd_PlayerList_f(edict_t *ent)
{
	int i;
	char st[80];
	char text[1400];
	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		sprintf(st, "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}

void Cmd_RocketJump_f(edict_t* ent) {
	//vec3_t forward = { 0,0,-1 };
	vec3_t forward, right, start;
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	if (!ent || ent->health <= 0) return;
	
	VectorCopy(ent->s.origin, start);
	VectorNormalize(right);
	VectorScale(right, 4, right);
	VectorAdd(start, right, start);
	fire_rocket(ent, start, forward, 1000, 1000, 100, 50);
	VectorNormalize(right);
	VectorScale(right, -8, right);
	VectorAdd(start, right, start);
	fire_rocket(ent, start, forward, 1000, 1000, 100, 50);
	
}

void Cmd_findME_f(edict_t* ent) {
	int x = ent->s.origin[0];
	int z = ent->s.origin[1];
	int y = ent->s.origin[2];
	char msg[50];
	sprintf(msg, "%d, %d, %d, %f, %f, %f\n", x, z, y, ent->client->v_angle[0], ent->client->v_angle[1], ent->client->v_angle[2]);
	//sprintf(msg, "%d, %d, %d\n", ent->prevLocationP[0], ent->prevLocationP[1], ent->prevLocationP[2]);
	gi.cprintf(ent, PRINT_HIGH, "%s", msg);
	//-842, 408, 886 - player
	//-512 - enemy -> -0.998672
	/*ent->s.origin[0] = -842;
	ent->s.origin[1] = 886;
	ent->s.origin[2] = 408;*/
}

void Cmd_MainScreen_f(edict_t* ent)
{
	ent->client->showinventory = false; //true
	ent->client->showscores = false;
	ent->client->showdrive = false;
	ent->client->showstat = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged) && ent->client->showmain)
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true; //false
	ent->client->showmain = true;
	ent->client->pers.helpchanged = 0;
	char	string[1024];


	// send the layout
	Com_sprintf(string, sizeof(string),
		"xv 32 yv 8 picn inventory "			// background
		"xv 50 yv 40 string2 \"bullets remaining: %d\" "		// skill
		"xv 50 yv 50 string2 \"d-revolver:    0, 0 \" "		// level name
		"xv 50 yv 60 string2 \"k-slash:       10, 1 \" "		// help 1
		"xv 50 yv 70 string2 \"mchn-blaster:  10, 2\" "		// help 2
		"xv 50 yv 80 string2 \"hand canon:    15, 3\" "
		"xv 50 yv 90 string2 \"tesla laser:   15, 4 \" "
		"xv 50 yv 100 string2 \"time bomb:     10, 5 \" "
		"xv 50 yv 110 string2 \"the big shot:  15, 6 \" "
		"xv 50 yv 120 string2 \"mental dart:   10, 7 \" "
		"xv 50 yv 130 string2 \"dimm-dropper:  15, 8 \" "
		"xv 50 yv 140 string2 \"g-accelerator: 200, 9 \" ",
		ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))]);

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
	gi.unicast(ent, true);
}
void Cmd_DriveScreen_f(edict_t* ent)
{
	ent->client->showinventory = false;
	ent->client->showscores = false;
	ent->client->showmain = false;
	ent->client->showstat = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged) && ent->client->showdrive)
	{
		ent->client->showhelp = false;
		ent->client->showdrive = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->showdrive = true;
	ent->client->pers.helpchanged = 0;
	char	string[1024];

	Com_sprintf(string, sizeof(string),
		"xv 32 yv 8 picn inventory "
		"xv 50 yv 30 string2 \"Select a drive\" "
		"xv 50 yv 40 string2 \"None: 0, 0\" "
		"xv 50 yv 50 string2 \"Drain: 5, 1\" "
		"xv 50 yv 60 string2 \"Boost: 5, 2\" "
		"xv 50 yv 70 string2 \"Stun: 5, 3\" "
		"xv 50 yv 80 string2 \"Virus: 5, 4\" "
		"xv 50 yv 90 string2 \"Luck: 5, 5\" "
		"xv 50 yv 100 string2 \"Dilute: 5, 6\" ");

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
	gi.unicast(ent, true);
}

void Cmd_StatScreen_f(edict_t* ent)
{
	ent->client->showinventory = false;
	ent->client->showscores = false;
	ent->client->showmain = false;
	ent->client->showdrive = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged) && ent->client->showstat)
	{
		ent->client->showhelp = false;
		ent->client->showdrive = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->showstat = true;
	ent->client->pers.helpchanged = 0;
	char	string[1024];
	char	bar[11] = "----------\0";

	int hold = ent->client->pers.exp_points;
	int ind = 0;
	while (hold > 0 && ind < 10) {
		hold -= 40;
		bar[ind] = 'X';
		ind++;
	}

	//stats
	//attackmod/defensemod, purchasables
	Com_sprintf(string, sizeof(string),
		"xv 32 yv 8 picn inventory "
		"xv 50 yv 30 string2 \"Lv:%d\" "
		"xv 50 yv 40 string2 \"[%s]:%dpnts\" "
		"xv 50 yv 50 string2 \"atk: %d\" "
		"xv 50 yv 60 string2 \"def: %d\" "
		"xv 50 yv 70 string2 \"bullets: %d\" "
		"xv 50 yv 90 string2 \"level up: %d, 0\" "
		"xv 50 yv 100 string2 \"Katana: 100, 1\" "
		"xv 50 yv 110 string2 \"Hand Cannon: 200, 2\" "
		"xv 50 yv 120 string2 \"TimeBomb: 200, 3\" "
		"xv 50 yv 130 string2 \"Gamma Accelerator: 800, 4\" ", ent->client->pers.lv, bar, ent->client->pers.exp_points,
		ent->client->pers.attackMod, ent->client->pers.defenseMod, ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))], 
		ent->client->pers.exp_max);

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
	gi.unicast(ent, true);
}

void Cmd_oppentTurn_f(edict_t* ent) {
	vec3_t view = { -1,-180,0 };
	vec3_t forward;
	vec3_t right;
	int damage = 15;

	if (ent->client->stunTimer > 0) {
		ent->client->stunTimer--;
		return;
	}

	AngleVectors(view, forward, right, NULL);

	level.turn++;
	level.post = 0;
	if (ent->client->bTimer > 0)
		ent->client->bTimer--;
	if (ent->client->weakTimer > 0) {
		damage -= 7;
		ent->client->weakTimer--;
	}
	monster_fire_blaster(ent->opponent, ent->opponent->s.origin, forward, damage - ent->client->pers.defenseMod, 500, 39, EF_BLASTER);
	T_Damage(ent, ent->opponent, ent->opponent, forward, ent->s.origin, vec3_origin, damage, 0, 0, 0); //for now
	if (ent->client->virusTimer > 0) {
		T_Damage(ent->opponent, ent, ent, forward, ent->opponent->s.origin, vec3_origin, 5, 0, 0, 0);
		ent->client->virusTimer--;
	}
}

//sudo gui
void Cmd_selectOP_f(edict_t* ent, int op) {
	int	m_costs[10] = { 0, 10, 10, 15, 15, 10, 15, 10, 15, 200 };
	int	d_costs[7] = { 0, 5, 5, 5, 5, 5, 5 };
	int	s_costs[5] = { ent->client->pers.exp_max, 100, 200, 200, 800 };
	int stock = ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))];
	vec3_t forward, right, start;
	int chance, shot;
	vec3_t below = { 0, 0, -1 };
	int damage = 20;
	int luck;

	if (ent->client->showmain && stock >= m_costs[op]){
		switch(op)
		{
			case 0: //drive-revolver
				ent->client->slash = false;
				Cmd_DriveScreen_f(ent);
				break;
			case 1: //katana
				if (!(ent->client->buycode & 1))
					return;
				ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] -= m_costs[op];
				ent->client->slash = true;
				Cmd_DriveScreen_f(ent);
				break;
			case 2: //machine blaster
				ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] -= m_costs[op];
				ent->client->repeatB = 0; //weapon fires in clientThink
				ent->client->showhelp = false;
				ent->client->showmain = false;
				break;
			case 3: //hand cannon
				if (!((ent->client->buycode >> 1) & 1))
					return;
				ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] -= m_costs[op];
				AngleVectors(ent->client->v_angle, forward, right, NULL);
				VectorCopy(ent->s.origin, start);

				VectorNormalize(right);
				VectorScale(right, 4, right);
				VectorAdd(start, right, start);
				fire_rocket(ent, start, forward, 15 + ent->client->pers.attackMod, 1000, 100, 50);

				VectorNormalize(right);
				VectorScale(right, -8, right);
				VectorAdd(start, right, start);
				fire_rocket(ent, start, forward, 15 + ent->client->pers.attackMod, 1000, 100, 50);

				ent->client->showhelp = false;
				ent->client->showmain = false;
				level.turn++;
				break;
			case 4: //tesla laser
				ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] -= m_costs[op];
				ent->client->repeatL = 0; //weapon fires in clientThink
				ent->client->showhelp = false;
				ent->client->showmain = false;
				level.turn++;
				break;
			case 5: //time bomb
				if (!((ent->client->buycode >> 2) & 1))
					return;
				ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] -= m_costs[op];
				ent->client->bTimer = 4;
				weapon_grenadelauncher_fire(ent);
				ent->client->showhelp = false;
				ent->client->showmain = false;
				level.turn++;
				break;
			case 6: //the big shot
				ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] -= m_costs[op];
				chance = rand() % 20;
				shot = (chance == 1) ? 100: 0;
				AngleVectors(ent->client->v_angle, forward, right, NULL);
				VectorCopy(ent->s.origin, start);
				fire_rocket(ent, start, forward, shot, 1000, 100, 50);
				ent->client->showhelp = false;
				ent->client->showmain = false;
				level.turn++;
				break;
			case 7: //mental dart
				ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] -= m_costs[op];
				AngleVectors(ent->client->v_angle, forward, right, NULL);
				VectorCopy(ent->s.origin, start);
				fire_blaster(ent, start, forward, 5 + ent->client->pers.attackMod, 100, EF_BLASTER, false);
				ent->client->weakTimer = 3;
				ent->client->showhelp = false;
				ent->client->showmain = false;
				level.turn++;
				break;
			case 8: //dimmnesional dropper
				ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] -= m_costs[op];
				chance = rand() % 20;
				VectorCopy(ent->opponent->s.origin, start);
				start[2] += 180;
				if(chance == 0)
					fire_bfg(ent, start, below, 200, 400, 500);
				else if(chance > 0 && chance < 10)
					fire_rocket(ent, start, below, 25 + ent->client->pers.attackMod, 100, 100, 50);
				else
					fire_blaster(ent, start, below, 5 + ent->client->pers.attackMod, 50, EF_BLASTER, false);
				ent->client->showhelp = false;
				ent->client->showmain = false;
				level.turn++;
				break;
			case 9: //gamma accelerator
				if (!((ent->client->buycode >> 3) & 1))
					return;
				ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] -= m_costs[op];
				AngleVectors(ent->client->v_angle, forward, right, NULL);
				VectorCopy(ent->s.origin, start);
				fire_bfg(ent, start, forward, 200, 400, 500);
				ent->client->showhelp = false;
				ent->client->showmain = false;
				level.turn++;
				break;
			default: break;
		}
	}
	else if (ent->client->showdrive && op < 6 && stock >= d_costs[op]){
		ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] -= d_costs[op];
		switch(op)
		{
			case 0: //none
				Blaster_Fire(ent, vec3_origin, damage + ent->client->pers.attackMod, ent->client->slash, EF_BLASTER);
				ent->client->showhelp = false;
				ent->client->showdrive = false;
				level.turn++;
				break;
			case 1: //drain
				Blaster_Fire(ent, vec3_origin, damage + ent->client->pers.attackMod, ent->client->slash, EF_BLASTER);
				ent->health += 10;
				ent->client->showhelp = false;
				ent->client->showdrive = false;
				level.turn++;
				break;
			case 2: //boost
				Blaster_Fire(ent, vec3_origin, damage+10 + ent->client->pers.attackMod, ent->client->slash, EF_BLASTER);
				ent->client->showhelp = false;
				ent->client->showdrive = false;
				level.turn++;
				break;
			case 3: //stun
				Blaster_Fire(ent, vec3_origin, damage + ent->client->pers.attackMod, ent->client->slash, EF_BLASTER);
				if(ent->client->stunTimer == 0)
					ent->client->stunTimer = 3;
				ent->client->showhelp = false;
				ent->client->showdrive = false;
				level.turn++;
				break;
			case 4: //virus
				Blaster_Fire(ent, vec3_origin, damage + ent->client->pers.attackMod, ent->client->slash, EF_BLASTER);
				if (ent->client->virusTimer == 0)
					ent->client->virusTimer = 3;
				ent->client->showhelp = false;
				ent->client->showdrive = false;
				level.turn++;
				break;
			case 5: //luck
				luck = rand() % 5;
				if (luck == 4)
					damage *= 5;
				Blaster_Fire(ent, vec3_origin, damage + ent->client->pers.attackMod, ent->client->slash, EF_BLASTER);
				ent->client->showhelp = false;
				ent->client->showdrive = false;
				level.turn++;
				break;
			case 6: //dilute
				Blaster_Fire(ent, vec3_origin, damage + ent->client->pers.attackMod, ent->client->slash, EF_BLASTER);
				ent->client->weakTimer = 1;
				ent->client->showhelp = false;
				ent->client->showdrive = false;
				level.turn++;
				break;
			default: break;
		}
	}
	else if(ent->client->showstat && op < 5 && stock >= d_costs[op]){ //go to stats screen
		if (op > 0 && ((ent->client->buycode >> (op-1)) & 1))
			return;
		ent->client->pers.exp_points -= s_costs[op];
		switch (op)
		{
			case 0: //level up
				ent->client->pers.lv++;
				ent->client->pers.exp_max += 200;
				ent->client->pers.attackMod += 1;
				ent->client->pers.defenseMod += 1;
				ent->client->showhelp = false;
				ent->client->showstat = false;
				break;
			case 1: //buy katana
				ent->client->buycode |= 1; //0001
				ent->client->showhelp = false;
				ent->client->showstat = false;
				break;
			case 2: //buy hand cannon
				ent->client->buycode |= 2; //0010
				ent->client->showhelp = false;
				ent->client->showstat = false;
				break;
			case 3: //buy time bomb
				ent->client->buycode |= 4; //0100
				ent->client->showhelp = false;
				ent->client->showstat = false;
				break;
			case 4: //buy gamma accelerator //1000
				ent->client->buycode |= 8;
				ent->client->showhelp = false;
				ent->client->showstat = false;
				break;
			default: break;
		}
	}
}

Cmd_buyall_f(edict_t* ent) {
	ent->client->buycode = 15;
}

Cmd_lvUp_f(edict_t* ent) {
	ent->client->pers.lv++;
	ent->client->pers.exp_max += 200;
	ent->client->pers.attackMod += 1;
	ent->client->pers.defenseMod += 1;
}

/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	if (Q_stricmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "help") == 0)
	{	
		Cmd_Help_f (ent);
		return;
	}
	if (Q_stricmp(cmd, "emer") == 0)
	{
		Cmd_MainScreen_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "buyall") == 0)
	{
		Cmd_buyall_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "lvup") == 0)
	{
		Cmd_lvUp_f(ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_stricmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	else if (Q_stricmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "inven") == 0)
		Cmd_StatScreen_f (ent);
	else if (Q_stricmp (cmd, "invnext") == 0)
		SelectNextItem (ent, -1);
	else if (Q_stricmp (cmd, "invprev") == 0)
		SelectPrevItem (ent, -1);
	else if (Q_stricmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_stricmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_stricmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_stricmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	else if (Q_stricmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);
	else if (Q_stricmp (cmd, "wave") == 0)
		Cmd_Wave_f (ent);
	else if (Q_stricmp(cmd, "playerlist") == 0)
		Cmd_PlayerList_f(ent);
	else if (Q_stricmp(cmd, "rocketjump") == 0)
		Cmd_RocketJump_f(ent);
	else if (Q_stricmp(cmd, "findME") == 0)
		Cmd_findME_f(ent);
	
	else if (Q_stricmp(cmd, "w0") == 0)
		Cmd_selectOP_f(ent, 0);
	else if (Q_stricmp(cmd, "w1") == 0)
		Cmd_selectOP_f(ent, 1);
	else if (Q_stricmp(cmd, "w2") == 0)
		Cmd_selectOP_f(ent, 2);
	else if (Q_stricmp(cmd, "w3") == 0)
		Cmd_selectOP_f(ent, 3);
	else if (Q_stricmp(cmd, "w4") == 0)
		Cmd_selectOP_f(ent, 4);
	else if (Q_stricmp(cmd, "w5") == 0)
		Cmd_selectOP_f(ent, 5);
	else if (Q_stricmp(cmd, "w6") == 0)
		Cmd_selectOP_f(ent, 6);
	else if (Q_stricmp(cmd, "w7") == 0)
		Cmd_selectOP_f(ent, 7);
	else if (Q_stricmp(cmd, "w8") == 0)
		Cmd_selectOP_f(ent, 8);
	else if (Q_stricmp(cmd, "w9") == 0)
		Cmd_selectOP_f(ent, 9);
	
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
