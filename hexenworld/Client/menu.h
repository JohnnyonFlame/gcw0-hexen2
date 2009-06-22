/*
	menu.h

	$Id: menu.h,v 1.10 2009-06-22 14:00:42 sezero Exp $
*/

#ifndef __HX2_MENU_H
#define __HX2_MENU_H

//
// the net drivers should just set the apropriate bits in m_activenet,
// instead of having the menu code look through their internal tables
//
#define	MNET_IPX		1
#define	MNET_TCP		2

extern	int	m_activenet;

//
// menus
//
void M_Init (void);
void M_Keydown (int key);
void M_ToggleMenu_f (void);

void M_Menu_Options_f (void);
void M_Menu_Quit_f (void);

void M_Print (int cx, int cy, const char *str);
void M_Print2 (int cx, int cy, const char *str);
void M_PrintWhite (int cx, int cy, const char *str);

void M_Draw (void);
void M_DrawCharacter (int cx, int line, int num);

void M_DrawPic (int x, int y, qpic_t *pic);
void M_DrawTransPic (int x, int y, qpic_t *pic);
void M_DrawTextBox (int x, int y, int width, int lines);
#ifdef H2W
void M_DrawTextBox2 (int x, int y, int width, int lines);
#else
/* the bottom argument is for the intro messages of the expansion pack */
void M_DrawTextBox2 (int x, int y, int width, int lines, qboolean bottom);
#endif

void M_DrawCheckbox (int x, int y, int on);

void ScrollTitle (const char *name);

#endif	/* __HX2_MENU_H */

