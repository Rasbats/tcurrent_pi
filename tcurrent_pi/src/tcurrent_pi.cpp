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


#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
  #include <wx/glcanvas.h>
#endif //precompiled headers

#include <wx/fileconf.h>
#include <wx/stdpaths.h>

#include "tcurrent_pi.h"

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
    return new tcurrent_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
    delete p;
}

//---------------------------------------------------------------------------------------------------------
//
//    tcurrent PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

#include "icons.h"


//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

tcurrent_pi::tcurrent_pi(void *ppimgr)
      :opencpn_plugin_17(ppimgr)
{
      // Create the PlugIn icons
      initialize_images();
      m_bShowtcurrent = false;
}

tcurrent_pi::~tcurrent_pi(void)
{
      delete _img_tcurrent_pi;
      delete _img_tcurrent;
}

int tcurrent_pi::Init(void)
{
      AddLocaleCatalog( _T("opencpn-tcurrent_pi") );

      // Set some default private member parameters
      m_tcurrent_dialog_x = 0;
      m_tcurrent_dialog_y = 0;
      m_tcurrent_dialog_sx = 200;
      m_tcurrent_dialog_sy = 400;
      m_ptcurrentDialog = NULL;
      m_ptcurrentOverlayFactory = NULL;

      ::wxDisplaySize(&m_display_width, &m_display_height);

      //    Get a pointer to the opencpn configuration object
      m_pconfig = GetOCPNConfigObject();
	 
      //    And load the configuration items
      LoadConfig();

      // Get a pointer to the opencpn display canvas, to use as a parent for the tcurrent dialog
      m_parent_window = GetOCPNCanvasWindow();

      //    This PlugIn needs a toolbar icon, so request its insertion if enabled locally
      if(m_btcurrentShowIcon)
          m_leftclick_tool_id = InsertPlugInTool(_T(""), _img_tcurrent, _img_tcurrent, wxITEM_CHECK,
                                                 _("tcurrent"), _T(""), NULL,
                                                 tcurrent_TOOL_POSITION, 0, this);

      return (WANTS_OVERLAY_CALLBACK |
              WANTS_OPENGL_OVERLAY_CALLBACK |
              WANTS_CURSOR_LATLON       |
              WANTS_TOOLBAR_CALLBACK    |
              INSTALLS_TOOLBAR_TOOL     |
              WANTS_CONFIG              |
              WANTS_PREFERENCES         
              //WANTS_PLUGIN_MESSAGING
            );
}

bool tcurrent_pi::DeInit(void)
{
    if(m_ptcurrentDialog) {
        m_ptcurrentDialog->Close();
        delete m_ptcurrentDialog;
        m_ptcurrentDialog = NULL;
    }

    delete m_ptcurrentOverlayFactory;
    m_ptcurrentOverlayFactory = NULL;

    return true;
}

int tcurrent_pi::GetAPIVersionMajor()
{
      return MY_API_VERSION_MAJOR;
}

int tcurrent_pi::GetAPIVersionMinor()
{
      return MY_API_VERSION_MINOR;
}

int tcurrent_pi::GetPlugInVersionMajor()
{
      return PLUGIN_VERSION_MAJOR;
}

int tcurrent_pi::GetPlugInVersionMinor()
{
      return PLUGIN_VERSION_MINOR;
}

wxBitmap *tcurrent_pi::GetPlugInBitmap()
{
      return _img_tcurrent_pi;
}

wxString tcurrent_pi::GetCommonName()
{
      return _T("tcurrent");
}


wxString tcurrent_pi::GetShortDescription()
{
      return _("tcurrent PlugIn for OpenCPN");
}


wxString tcurrent_pi::GetLongDescription()
{
      return _("tcurrent PlugIn for OpenCPN\nProvides an overlay of Tidal Current (Stream) Arrows.\n\n\
			   ");
}

void tcurrent_pi::SetDefaults(void)
{
}


int tcurrent_pi::GetToolbarToolCount(void)
{
      return 1;
}

void tcurrent_pi::ShowPreferencesDialog( wxWindow* parent )
{
    tcurrentPreferencesDialog *Pref = new tcurrentPreferencesDialog(parent);

    Pref->m_cbUseRate->SetValue(m_bCopyUseRate);
    Pref->m_cbUseDirection->SetValue(m_bCopyUseDirection);
	Pref->m_cbFillColour->SetValue(m_btcurrentUseHiDef);

 if( Pref->ShowModal() == wxID_OK ) {

	 //bool copyFillColour = true;


     bool copyrate = Pref->m_cbUseRate->GetValue();
     bool copydirection = Pref->m_cbUseDirection->GetValue();
	 bool FillColour = Pref->m_cbFillColour->GetValue();

		 if (m_btcurrentUseHiDef != FillColour){		 
			 m_btcurrentUseHiDef = FillColour;
		 }
	 
        if( m_bCopyUseRate != copyrate || m_bCopyUseDirection != copydirection ) {
             m_bCopyUseRate = copyrate;
             m_bCopyUseDirection = copydirection;           
         }

		
         if(m_ptcurrentDialog )
		 {
			 m_ptcurrentDialog->OpenFile(true);
			 m_ptcurrentDialog->m_bUseRate = m_bCopyUseRate;
			 m_ptcurrentDialog->m_bUseDirection = m_bCopyUseDirection;	
			 m_ptcurrentDialog->m_bUseFillColour = m_btcurrentUseHiDef;
		 }

		 if (m_ptcurrentOverlayFactory)
		 {
			 m_ptcurrentOverlayFactory->m_bShowRate = m_bCopyUseRate;
			 m_ptcurrentOverlayFactory->m_bShowDirection = m_bCopyUseDirection;
			 m_ptcurrentOverlayFactory->m_bShowFillColour = m_btcurrentUseHiDef;
		 }

         SaveConfig();
     }

	
}

void tcurrent_pi::OnToolbarToolCallback(int id)
{
    if(!m_ptcurrentDialog)
    {
		
        		
		m_ptcurrentDialog = new tcurrentUIDialog(m_parent_window, this);
        wxPoint p = wxPoint(m_tcurrent_dialog_x, m_tcurrent_dialog_y);
        m_ptcurrentDialog->Move(0,0);        // workaround for gtk autocentre dialog behavior
        m_ptcurrentDialog->Move(p);

        // Create the drawing factory
        m_ptcurrentOverlayFactory = new tcurrentOverlayFactory( *m_ptcurrentDialog );
        m_ptcurrentOverlayFactory->SetParentSize( m_display_width, m_display_height);
		
		
        
    }

   m_ptcurrentDialog->OpenFile(true);
      // Qualify the tcurrent dialog position
            bool b_reset_pos = false;

#ifdef __WXMSW__
        //  Support MultiMonitor setups which an allow negative window positions.
        //  If the requested window does not intersect any installed monitor,
        //  then default to simple primary monitor positioning.
            RECT frame_title_rect;
            frame_title_rect.left =   m_tcurrent_dialog_x;
            frame_title_rect.top =    m_tcurrent_dialog_y;
            frame_title_rect.right =  m_tcurrent_dialog_x + m_tcurrent_dialog_sx;
            frame_title_rect.bottom = m_tcurrent_dialog_y + 30;


            if(NULL == MonitorFromRect(&frame_title_rect, MONITOR_DEFAULTTONULL))
                  b_reset_pos = true;
#else
       //    Make sure drag bar (title bar) of window on Client Area of screen, with a little slop...
            wxRect window_title_rect;                    // conservative estimate
            window_title_rect.x = m_tcurrent_dialog_x;
            window_title_rect.y = m_tcurrent_dialog_y;
            window_title_rect.width = m_tcurrent_dialog_sx;
            window_title_rect.height = 30;

            wxRect ClientRect = wxGetClientDisplayRect();
            ClientRect.Deflate(60, 60);      // Prevent the new window from being too close to the edge
            if(!ClientRect.Intersects(window_title_rect))
                  b_reset_pos = true;

#endif

            if(b_reset_pos)
            {
                  m_tcurrent_dialog_x = 20;
                  m_tcurrent_dialog_y = 170;
                  m_tcurrent_dialog_sx = 300;
                  m_tcurrent_dialog_sy = 540;
            }

      //Toggle tcurrent overlay display
      m_bShowtcurrent = !m_bShowtcurrent;

      //    Toggle dialog?
      if(m_bShowtcurrent) {
          m_ptcurrentDialog->Show();
      } else {
          m_ptcurrentDialog->Hide();         
          }

      // Toggle is handled by the toolbar but we must keep plugin manager b_toggle updated
      // to actual status to ensure correct status upon toolbar rebuild
      SetToolbarItemState( m_leftclick_tool_id, m_bShowtcurrent );
      RequestRefresh(m_parent_window); // refresh mainn window
}

void tcurrent_pi::OntcurrentDialogClose()
{
    m_bShowtcurrent = false;
    SetToolbarItemState( m_leftclick_tool_id, m_bShowtcurrent );

    m_ptcurrentDialog->Hide();
    //if(m_ptcurrentDialog->pReq_Dialog) m_ptcurrentDialog->pReq_Dialog->Hide();

    SaveConfig();

    RequestRefresh(m_parent_window); // refresh mainn window

}

bool tcurrent_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp)
{
    if(!m_ptcurrentDialog ||
       !m_ptcurrentDialog->IsShown() ||
       !m_ptcurrentOverlayFactory)
        return false;

    m_ptcurrentDialog->SetViewPort( vp );
    m_ptcurrentOverlayFactory->RendertcurrentOverlay ( dc, vp );
    return true;
}

bool tcurrent_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp)
{
    if(!m_ptcurrentDialog ||
       !m_ptcurrentDialog->IsShown() ||
       !m_ptcurrentOverlayFactory)
        return false;

    m_ptcurrentDialog->SetViewPort( vp );
    m_ptcurrentOverlayFactory->RenderGLtcurrentOverlay ( pcontext, vp );
    return true;
}
void tcurrent_pi::SetCursorLatLon(double lat, double lon)
{
    if(m_ptcurrentDialog)
        m_ptcurrentDialog->SetCursorLatLon(lat, lon);
}

bool tcurrent_pi::LoadConfig(void)
{
    wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

    if(!pConf)
        return false;

    pConf->SetPath ( _T( "/PlugIns/tcurrent" ) );

	m_bCopyUseRate = pConf->Read ( _T ( "tcurrentUseRate" ),1);
    m_bCopyUseDirection = pConf->Read ( _T ( "tcurrentUseDirection" ), 1);
	m_btcurrentUseHiDef = pConf->Read ( _T ( "tcurrentUseFillColour" ), 1);

    m_tcurrent_dialog_sx = pConf->Read ( _T ( "tcurrentDialogSizeX" ), 300L );
    m_tcurrent_dialog_sy = pConf->Read ( _T ( "tcurrentDialogSizeY" ), 540L );
    m_tcurrent_dialog_x =  pConf->Read ( _T ( "tcurrentDialogPosX" ), 20L );
    m_tcurrent_dialog_y =  pConf->Read ( _T ( "tcurrentDialogPosY" ), 170L );


	
    return true;
}

bool tcurrent_pi::SaveConfig(void)
{
    wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

    if(!pConf)
        return false;

    pConf->SetPath ( _T( "/PlugIns/tcurrent" ) );
    pConf->Write ( _T ( "tcurrentUseRate" ), m_bCopyUseRate );
    pConf->Write ( _T ( "tcurrentUseDirection" ), m_bCopyUseDirection );
	pConf->Write ( _T ( "tcurrentUseFillColour" ), m_btcurrentUseHiDef );

    pConf->Write ( _T ( "tcurrentDialogSizeX" ),  m_tcurrent_dialog_sx );
    pConf->Write ( _T ( "tcurrentDialogSizeY" ),  m_tcurrent_dialog_sy );
    pConf->Write ( _T ( "tcurrentDialogPosX" ),   m_tcurrent_dialog_x );
    pConf->Write ( _T ( "tcurrentDialogPosY" ),   m_tcurrent_dialog_y );

	
    return true;
}

void tcurrent_pi::SetColorScheme(PI_ColorScheme cs)
{
    DimeWindow(m_ptcurrentDialog);
}

