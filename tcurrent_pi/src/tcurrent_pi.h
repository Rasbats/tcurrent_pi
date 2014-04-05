/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  tcurrent Plugin
 * Author:   David Register, Mike Rossiter
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 */

#ifndef _tcurrentPI_H_
#define _tcurrentPI_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
  #include <wx/glcanvas.h>
#endif //precompiled headers

#define     PLUGIN_VERSION_MAJOR    1
#define     PLUGIN_VERSION_MINOR    0

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    7

#include "../../../include/ocpn_plugin.h"
#include "tcurrentOverlayFactory.h"
#include "tcurrentUIDialog.h"

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

#define tcurrent_TOOL_POSITION    -1          // Request default positioning of toolbar tool

class tcurrent_pi : public opencpn_plugin_17
{
public:
      tcurrent_pi(void *ppimgr);
      ~tcurrent_pi(void);

//    The required PlugIn Methods
      int Init(void);
      bool DeInit(void);

      int GetAPIVersionMajor();
      int GetAPIVersionMinor();
      int GetPlugInVersionMajor();
      int GetPlugInVersionMinor();
      wxBitmap *GetPlugInBitmap();
      wxString GetCommonName();
      wxString GetShortDescription();
      wxString GetLongDescription();

//    The override PlugIn Methods
      bool RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp);
	  bool RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp);
      void SetCursorLatLon(double lat, double lon);
      void SendTimelineMessage(wxDateTime time);
      void SetDefaults(void);
      int GetToolbarToolCount(void);
      void ShowPreferencesDialog( wxWindow* parent );
      void OnToolbarToolCallback(int id);

// Other public methods
      void SettcurrentDialogX    (int x){ m_tcurrent_dialog_x = x;};
      void SettcurrentDialogY    (int x){ m_tcurrent_dialog_y = x;}
      void SettcurrentDialogSizeX(int x){ m_tcurrent_dialog_sx = x;}
      void SettcurrentDialogSizeY(int x){ m_tcurrent_dialog_sy = x;}
      void SetColorScheme(PI_ColorScheme cs);

      void OntcurrentDialogClose();

      bool GetCopyRate() { return  m_bCopyUseRate; }
      bool GetCopyDirection() { return  m_bCopyUseDirection; }
	  bool GetCopyColour() { return m_btcurrentUseHiDef ; }
     // wxString GetCopyPort() { return  m_bCopyusePort; }
      tcurrentOverlayFactory *GettcurrentOverlayFactory(){ return m_ptcurrentOverlayFactory; }

	  
     // wxString         myPort;
	 //wxString SetPort(){ return myPort;}
	 // wxString myNewPort; 
	 // void SetKMLDir(wxString KML_dir){ m_KML_dir = KML_dir;};
	

private:
      bool LoadConfig(void);
      bool SaveConfig(void);

      wxFileConfig     *m_pconfig;
      wxWindow         *m_parent_window;

      tcurrentUIDialog     *m_ptcurrentDialog;
      tcurrentOverlayFactory *m_ptcurrentOverlayFactory;

      int              m_display_width, m_display_height;
      int              m_leftclick_tool_id;

      int              m_tcurrent_dialog_x, m_tcurrent_dialog_y;
      int              m_tcurrent_dialog_sx, m_tcurrent_dialog_sy;

	  

	  //wxString myPort;

	 // int myNewPort; 
	 // 

      //    Controls added to Preferences panel
      wxCheckBox              *m_ptcurrentUseHiDef;
      wxCheckBox              *m_ptcurrentUseGradualColors;

     // tcurrentTimelineRecordSet *m_pLastTimelineSet;

      // preference data
      bool              m_btcurrentUseHiDef;
      bool              m_btcurrentUseGradualColors;
	  bool              m_bCopyUseRate;
      bool              m_bCopyUseDirection;

      int              m_bTimeZone;
     
      int              m_bStartOptions;
      wxString         m_RequestConfig;
     
      
      bool             m_btcurrentShowIcon;

      int              m_height;

      bool        m_bShowtcurrent;
};

//----------------------------------------------------------------------------------------
// Prefrence dialog definition
//----------------------------------------------------------------------------------------

class tcurrentPreferencesDialog : public tcurrentPreferencesDialogBase
{
public:
    tcurrentPreferencesDialog( wxWindow *pparent)
    : tcurrentPreferencesDialogBase(pparent) {}
    ~tcurrentPreferencesDialog() {}

private:
    
};
#endif
