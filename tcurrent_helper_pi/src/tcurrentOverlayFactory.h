/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  tcurrent Plugin Freinds
 * Author:   David Register
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 */

#include <map>
#include <vector>
#include <wx/string.h>

using namespace std;



//----------------------------------------------------------------------------------------------------------
//    tcurrent Overlay Specification
//----------------------------------------------------------------------------------------------------------

class tcurrentOverlay {
public:
    tcurrentOverlay( void )
    {
        m_iTexture = 0;
        m_pDCBitmap = NULL, m_pRGBA = NULL;
    }

    ~tcurrentOverlay( void )
    {
        if(m_iTexture)
          glDeleteTextures( 1, &m_iTexture );
        delete m_pDCBitmap, delete[] m_pRGBA;
    }

    unsigned int m_iTexture; /* opengl mode */

    wxBitmap *m_pDCBitmap; /* dc mode */
    unsigned char *m_pRGBA;

    int m_width;
    int m_height;
};

//----------------------------------------------------------------------------------------------------------
//    tcurrent Overlay Factory Specification
//----------------------------------------------------------------------------------------------------------

class tcurrentUIDialog;
//class tcurrentRecord;
//class tcurrentTimelineRecordSet;
class Position;

class tcurrentOverlayFactory {
public:
    tcurrentOverlayFactory( tcurrentUIDialog &dlg );
    ~tcurrentOverlayFactory();

    void SetSettings( bool hiDefGraphics, bool GradualColors )
    {
      m_hiDefGraphics = hiDefGraphics;
      m_bGradualColors = GradualColors;
    }

    void SetMessage( wxString message ) { m_Message = message; }
    void SetTimeZone( int TimeZone ) { m_TimeZone = TimeZone; }
    void SetParentSize( int w, int h ) { m_ParentSize.SetWidth(w) ; m_ParentSize.SetHeight(h) ;}
	bool RenderGLtcurrentOverlay( wxGLContext *pcontext, PlugIn_ViewPort *vp );
    bool RendertcurrentOverlay( wxDC &dc, PlugIn_ViewPort *vp );

    void Reset();
	wxImage &DrawGLText( double value);
	wxImage &DrawGLTextString( wxString myText);
	wxImage &DrawGLPolygon();

	void DrawGLLabels(tcurrentOverlayFactory *pof, wxDC *dc,
                               PlugIn_ViewPort *vp, int density, int first,
                               wxImage &imageLabel, double myLat, double myLon, int offset);

	void drawGLPolygons(tcurrentOverlayFactory *pof, wxDC *dc,
                                PlugIn_ViewPort *vp, int density, int first,
                                wxImage &imageLabel, double myLat, double myLon, int offset );

	void DrawGLLine( double x1, double y1, double x2, double y2, double width, wxColour myColour );
    void DrawOLBitmap( const wxBitmap &bitmap, wxCoord x, wxCoord y, bool usemask );
	PlugIn_ViewPort *vp;
	bool              m_bShowRate;
    bool              m_bShowDirection;
	bool              m_bShowFillColour;


private:

	bool inGL;
	vector<Position> new_positions;
    bool DoRendertcurrentOverlay( PlugIn_ViewPort *vp );
	void RenderMyArrows(PlugIn_ViewPort *vp );

    void DrawMessageWindow( wxString msg, int x, int y , wxFont *mfont);

    void drawWindArrowWithBarbs( int config, int x, int y, double vx, double vy,
                                 bool polar, bool south, wxColour arrowColor );
    void drawWaveArrow( int i, int j, double dir, wxColour arrowColor );

	wxColour GetSpeedColour(double my_speed);

	void drawCurrentArrow(int x, int y, double rot_angle, double scale, double rate );

    void drawSingleArrow( int i, int j, double dir, wxColour arrowColor, int width = 1 );

    void drawTransformedLine( wxPen pen, double si, double co, int di, int dj,
                              int i, int j, int k, int l );
    void drawPetiteBarbule( wxPen pen, bool south, double si, double co, int di, int dj, int b );
    void drawGrandeBarbule( wxPen pen, bool south, double si, double co, int di, int dj, int b );
    void drawTriangle( wxPen pen, bool south, double si, double co, int di, int dj, int b );

    double m_last_vp_scale;

	//  for GL
	wxColour c_GLcolour;
	wxPoint p_basic[9];
	//
    wxString m_Message;
    wxString m_Message_Hiden;
    int  m_TimeZone;
    wxSize  m_ParentSize;

    wxDC *m_pdc;
    wxGraphicsContext *m_gdc;

    wxFont *m_dFont_map;
    wxFont *m_dFont_war;

    bool m_hiDefGraphics;
    bool m_bGradualColors;

    std::map < double , wxImage > m_labelCache;
	std::map < wxString , wxImage > m_labelCacheText;

    tcurrentUIDialog &m_dlg;
  //  tcurrentOverlaySettings &m_Settings;
	vector<Position> &m_Positions;
	bool m_bool;
	int m_fromHW;
	wxString m_portXML2;
	
   // bool              m_bUseRate;
   // bool              m_bUseDirection;


	//double latN[100], lonN[100];

};
