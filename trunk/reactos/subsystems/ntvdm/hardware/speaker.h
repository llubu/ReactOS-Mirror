/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            speaker.h
 * PURPOSE:         PC Speaker emulation
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _SPEAKER_H_
#define _SPEAKER_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

/* FUNCTIONS ******************************************************************/

VOID SpeakerChange(VOID);

VOID SpeakerInitialize(VOID);
VOID SpeakerCleanup(VOID);

#endif // _SPEAKER_H_

/* EOF */
