/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  tcurrent Object
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 *
 */

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include <wx/glcanvas.h>
#include <wx/graphics.h>
#include <wx/progdlg.h>

#include "tcurrentUIDialog.h"
#include "tcurrentUIDialogBase.h"
#include "tcurrentOverlayFactory.h"
#include <vector>

#include "dychart.h"
//#include "cutil.h"

using namespace std;

class Position;
class tcurrentUIDialog;
class PlugIn_ViewPort;

enum OVERLAP { _IN, _ON, _OUT };

#define NUM_CURRENT_ARROW_POINTS 9
static wxPoint CurrentArrowArray[NUM_CURRENT_ARROW_POINTS] = { wxPoint( 0, 0 ), wxPoint( 0, -10 ),
        wxPoint( 55, -10 ), wxPoint( 55, -25 ), wxPoint( 100, 0 ), wxPoint( 55, 25 ), wxPoint( 55,
                10 ), wxPoint( 0, 10 ), wxPoint( 0, 0 )
                                                             };

//----------------------------------------------------------------------------------------------------------
//    tcurrent Overlay Factory Implementation
//----------------------------------------------------------------------------------------------------------
tcurrentOverlayFactory::tcurrentOverlayFactory( tcurrentUIDialog &dlg )
	: m_dlg(dlg), m_Positions(dlg.my_positions), m_bool(dlg.mBool),m_fromHW(dlg.button_id) //, m_portXML2(dlg.m_portXML)
{
    m_dFont_map = new wxFont( 10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
    m_dFont_war = new wxFont( 16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL );

  //  m_ptcurrentTimelineRecordSet = NULL;
    m_last_vp_scale = 0.;
	m_bShowRate = m_dlg.m_bUseRate;
	m_bShowDirection = m_dlg.m_bUseDirection;
	m_bShowFillColour = m_dlg.m_bUseFillColour;
    //for(int i=0; i<tcurrentOverlaySettings::SETTINGS_COUNT; i++)
      //  m_pOverlay[i] = NULL;
}

tcurrentOverlayFactory::~tcurrentOverlayFactory()
{
  ///  ClearCachedData();
}

void tcurrentOverlayFactory::Reset()
{
   // m_ptcurrentTimelineRecordSet = NULL;

  ///  ClearCachedData();
}

bool tcurrentOverlayFactory::RenderGLtcurrentOverlay( wxGLContext *pcontext, PlugIn_ViewPort *vp )
{
    m_pdc = NULL;                  // inform lower layers that this is OpenGL render
    return DoRendertcurrentOverlay( vp );
}

bool tcurrentOverlayFactory::RendertcurrentOverlay( wxDC &dc, PlugIn_ViewPort *vp )
{

#if wxUSE_GRAPHICS_CONTEXT
    wxMemoryDC *pmdc;
    pmdc = wxDynamicCast(&dc, wxMemoryDC);
    wxGraphicsContext *pgc = wxGraphicsContext::Create( *pmdc );
    m_gdc = pgc;
    m_pdc = &dc;
#else
    m_pdc = &dc;
#endif
	   
	m_pdc = &dc;
    return DoRendertcurrentOverlay( vp );
}



bool tcurrentOverlayFactory::DoRendertcurrentOverlay( PlugIn_ViewPort *vp )
{
    

    m_Message_Hiden.Empty();

	    //    If the scale has changed, clear out the cached bitmaps

    m_last_vp_scale = vp->view_scale_ppm;
	

	RenderMyArrows(vp);

    if( !m_Message_Hiden.IsEmpty() )
        DrawMessageWindow( m_Message_Hiden , vp->pix_width, vp->pix_height, m_dFont_map );
	
	
	 DrawMessageWindow( m_Message_Hiden , vp->pix_width, vp->pix_height, m_dFont_map );
    return true;
}

void tcurrentOverlayFactory::RenderMyArrows(PlugIn_ViewPort *vp )
{		    
	       wxPoint p;			
	
		   double myX, myY;
		   myX = 50;
     	   myY = 0;
									
		   vector<Position>m_new = m_dlg.my_positions;
			
		   double value,  decValue; 
		   double value1,decValue1;
		   
		   wxString mLat;
		   wxString mBitLat;
		   wxString mDecLat;

		   wxString mLon;
		   wxString mBitLon;
		   wxString mDecLon;

		   double latF, lonF;
		   int n=0;
		   int m_len;		   
		   
		   int m = m_dlg.m_choice1->GetSelection();
		   wxString s = m_dlg.m_choice1->GetString(m);

		   PortTides myPortTides;
		   myPortTides = m_dlg.PopulatePortTides(s);
		   double myCurrent;

		   for(std::vector<Position>::iterator it = m_new.begin();  it != m_new.end(); it++){
           
			  mLat = (*it).lat;
			  mBitLat = mLat.Mid(0,2);
			  mDecLat = mLat.Right(4);			  			  
			  mDecLat.ToDouble(&decValue);

		    if(!mBitLat.ToDouble(&value)){ /* error! */ }
				
			    latF = value + (decValue/100/60);
				mLon = (*it).lon;
				m_len = mLon.Len();
				
			 if (m_len == 7)
			 {
					mBitLon = mLon.Mid(0,3);
				 }
				 
				 if (m_len == 6)
				 {
					mBitLon = mLon.Mid(0,2);				
				 }
				 
				 if  (m_len == 5)				 
				 {
					mBitLon = mLon.Mid(0,1);									
				 }

				  if  (m_len == 4)				 
				 {
					mBitLon = _T("00.00");									
				 }	

             if (mBitLon == wxString(_T("-")))
			 {
				 value1 = -0.00001;
			 }
			 else
			 {
				 if(!mBitLon.ToDouble(&value1)){ /* error! */}
			 }
			 
			 mDecLon = mLon.Right(4);

			 if(!mDecLon.ToDouble(&decValue1)){};		   
			 
			 if (value1 < 0)
			 {				
				lonF = value1 - decValue1/100/60; 
			 }
			 else
			 {
				lonF = value1 + decValue1/100/60;
			 }
			
            GetCanvasPixLL( vp, &p,latF, lonF );
			
			wxString port_num = (*it).port_num;		

			if (port_num == m_dlg.m_portXML){ 
										        
					// drawing scaled current arrows												
				    m_fromHW = m_dlg.button_id;

					wxString m_minus_plus = (*it).minus_plus[m_fromHW].Mid(0,3);
			
					double dir;
					m_minus_plus.ToDouble(&dir);

					double m_spdSpring;
					wxString m_speed = (*it).minus_plus[m_fromHW].Mid(3,2);
					m_speed.ToDouble(&m_spdSpring);
					m_spdSpring = m_spdSpring/10;

					double m_spdNeap;
					wxString m_speed2 = (*it).minus_plus[m_fromHW].Mid(5,2);
					m_speed2.ToDouble(&m_spdNeap);			
					m_spdNeap = m_spdNeap/10;
					int mmx, mmy;
					wxDisplaySizeMM( &mmx, &mmy );

					int sx, sy;
					wxDisplaySize( &sx, &sy );

					double m_pix_per_mm = ( (double) sx ) / ( (double) mmx );

					int mm_per_knot = 10;
					float current_draw_scaler = mm_per_knot * m_pix_per_mm * 100 / 100.0;

					myCurrent = m_dlg.CalcCurrent(myPortTides.m_spRange,myPortTides.m_npRange,m_spdSpring,m_spdNeap,m_dlg.myRange);
					if (myCurrent == 0 || myCurrent < 0)
						myCurrent = 00.001;

					double a1 = myCurrent*10;    //fabs( tcvalue ) * 10.;
					a1 = wxMax(1.0, a1);		// Current values less than 0.1 knot
												// will be displayed as 0
					double a2 = log10( a1 );

					double scale = current_draw_scaler * a2;			        
                  
					wxColour colour;
					GetGlobalColor( _T ( "UINFO" ), &colour );
					wxPen pen( colour, 1 );
					wxBrush brush(colour);
					
					if (m_pdc){
                    m_pdc->SetPen( pen );
                    m_pdc->SetBrush( brush );
					}

					drawCurrentArrow( p.x, p.y,
												dir - 90 , scale / 100, myCurrent );
					int shift = 0;
					int density = 0;
			        int first = 0;
					
					if(!m_pdc && m_bShowFillColour){
						drawGLPolygons(this, m_pdc, vp, density, first, DrawGLPolygon(), latF, lonF, shift);
					}


                   if (!m_pdc){
					   if( m_bShowRate){
                          
					      DrawGLLabels( this, m_pdc, vp, density,
                                first,  (DrawGLText( myCurrent )), latF, lonF, 0 ) ;
						  shift = 13;
					   }
					   if( m_bShowDirection){
							DrawGLLabels( this, m_pdc, vp, density,
                                first,  (DrawGLTextString(m_minus_plus)), latF, lonF, shift) ;
					   }
				   }			

					wxFont *pTCFont;
					pTCFont = wxTheFontList->FindOrCreateFont( 12, wxDEFAULT, wxNORMAL, wxBOLD, FALSE,
                                                   wxString( _T ( "Eurostile Extended" ) ) );
					char sbuf[20];					 
					
					if( m_bShowRate && m_pdc ) 
					{
						m_pdc->SetFont( *pTCFont );
						_snprintf( sbuf, 19, "%3.1f", myCurrent );
						m_pdc->DrawText( wxString( sbuf, wxConvUTF8 ), p.x, p.y );
						shift = 13;
                    }					 
					
					if ( m_bShowDirection && m_pdc)	
					{																
						m_pdc->DrawText( m_minus_plus, p.x, p.y + shift );
					}
					// end scaled current			
			}   // end if         

	    }// end for			   		   											   		
}

void tcurrentOverlayFactory::DrawMessageWindow( wxString msg, int x, int y , wxFont *mfont)
{
    if(msg.empty())
        return;

    wxMemoryDC mdc;
    wxBitmap bm( 1000, 1000 );
    mdc.SelectObject( bm );
    mdc.Clear();

    //wxFont mfont( 15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL );
    mdc.SetFont( *mfont );
    mdc.SetPen( *wxTRANSPARENT_PEN);
   // mdc.SetBrush( *wxLIGHT_GREY_BRUSH );
    mdc.SetBrush( wxColour(243, 229, 47 ) );
    int w, h;
    mdc.GetMultiLineTextExtent( msg, &w, &h );
    h += 2;
    int label_offset = 10;
    int wdraw = w + ( label_offset * 2 );
    mdc.DrawRectangle( 0, 0, wdraw, h );

    mdc.DrawLabel( msg, wxRect( label_offset, 0, wdraw, h ), wxALIGN_LEFT| wxALIGN_CENTRE_VERTICAL);
    mdc.SelectObject( wxNullBitmap );

    wxBitmap sbm = bm.GetSubBitmap( wxRect( 0, 0, wdraw, h ) );

    DrawOLBitmap( sbm, 0, y - ( GetChartbarHeight() + h ), false );
}

wxColour tcurrentOverlayFactory::GetSpeedColour(double my_speed){

	wxColour c_blue = wxColour(128, 248, 248);
	wxColour c_green = wxColour(0, 166, 80);
	wxColour c_yellow_orange = wxColour(253, 184, 19);
	wxColour c_orange = wxColour(248, 128, 64);
	wxColour c_red = wxColour(248, 0, 0);

	if (my_speed < 0.5){ return c_blue;}
	if ((my_speed >= 0.5) && (my_speed < 1.5)){ return c_green;}
	if ((my_speed >= 1.5) && (my_speed < 2.5)){ return c_yellow_orange;}
	if ((my_speed >= 2.5) && (my_speed < 3.5)){ return c_orange;}
	if ((my_speed >= 3.5) ){ return c_red;}

	return wxColour(0, 0, 0);
}

void tcurrentOverlayFactory::drawCurrentArrow(int x, int y, double rot_angle, double scale, double rate )
{   	
	
	wxPoint p[9];

    wxColour colour;	
	colour = GetSpeedColour( rate );
	
	c_GLcolour = colour;  // for filling GL arrows

	wxPen pen( colour, 2 );
	wxBrush brush(colour);

    if( m_pdc ) {
        m_pdc->SetPen( pen );
        m_pdc->SetBrush( brush);  //*wxTRANSPARENT_BRUSH);
    }

   
	if( scale > 1e-2 ) {

        float sin_rot = sin( rot_angle * PI / 180. );
        float cos_rot = cos( rot_angle * PI / 180. );

        // Move to the first point

        float xt = CurrentArrowArray[0].x;
        float yt = CurrentArrowArray[0].y;

        float xp = ( xt * cos_rot ) - ( yt * sin_rot );
        float yp = ( xt * sin_rot ) + ( yt * cos_rot );
        int x1 = (int) ( xp * scale );
        int y1 = (int) ( yp * scale );

		p[0].x = x;
		p[0].y = y;

		p_basic[0].x = 100;
		p_basic[0].y = 100;

        // Walk thru the point list
        for( int ip = 1; ip < NUM_CURRENT_ARROW_POINTS; ip++ ) {
            xt = CurrentArrowArray[ip].x;
            yt = CurrentArrowArray[ip].y;

            float xp = ( xt * cos_rot ) - ( yt * sin_rot );
            float yp = ( xt * sin_rot ) + ( yt * cos_rot );
            int x2 = (int) ( xp * scale );
            int y2 = (int) ( yp * scale );

			p_basic[ip].x = 100 + x2;
			p_basic[ip].y = 100 + y2;

			if (m_pdc){
				m_pdc->DrawLine( x1 + x, y1 + y, x2 + x, y2 + y );
			}
			else{
				DrawGLLine(x1 + x, y1 + y, x2 + x, y2 + y , 2, colour);
			}
			p[ip].x = x1 + x; 
            p[ip].y = y1 + y;

			x1 = x2;
            y1 = y2;            			
         }

		//p[9].x = x1;
		//p[9].y = y1;

		if( m_bShowFillColour && m_pdc){
			m_pdc->SetBrush(brush);
			m_pdc->DrawPolygon(9,p);
		}

    }
}

wxImage &tcurrentOverlayFactory::DrawGLText( double value ){

	wxString labels;
    int p =  1;//two decimals for pressure & inHG, one for small values    
	labels.Printf( _T("%.*f"), p, value );
	
	wxMemoryDC mdc(wxNullBitmap);

	wxFont *pTCFont;
					pTCFont = wxTheFontList->FindOrCreateFont( 12, wxDEFAULT, wxNORMAL, wxBOLD, FALSE,
                                                   wxString( _T ( "Eurostile Extended" ) ) );

   // wxFont mfont( 12, wxDEFAULT, wxNORMAL, wxBOLD );
    mdc.SetFont(*pTCFont);

    int w, h;
    mdc.GetTextExtent(labels, &w, &h);

    int label_offset = 10;   //5

    wxBitmap bm(w +  label_offset*2, h + 1);
    mdc.SelectObject(bm);
    mdc.Clear();

    wxColour text_color;

    GetGlobalColor( _T ("UINFD" ), &text_color );
    wxPen penText(text_color);
	mdc.SetPen(penText);

    mdc.SetBrush(*wxTRANSPARENT_BRUSH);
    mdc.SetTextForeground(text_color);
    mdc.SetTextBackground(wxTRANSPARENT);
          
    int xd = 0;
    int yd = 0;
//    mdc.DrawRoundedRectangle(xd, yd, w+(label_offset * 2), h+2, -.25);
    //mdc.DrawRectangle(xd, yd, w+(label_offset * 2), h+2);
    mdc.DrawText(labels, label_offset + xd, yd+1);
          
    mdc.SelectObject(wxNullBitmap);

    m_labelCache[value] = bm.ConvertToImage();

    m_labelCache[value].InitAlpha();

    wxImage &image = m_labelCache[value];

    unsigned char *d = image.GetData();
    unsigned char *a = image.GetAlpha();

    w = image.GetWidth(), h = image.GetHeight();
    for( int y = 0; y < h; y++ )
        for( int x = 0; x < w; x++ ) {
            int r, g, b;
            int ioff = (y * w + x);
            r = d[ioff* 3 + 0];
            g = d[ioff* 3 + 1];
            b = d[ioff* 3 + 2];

            a[ioff] = 255-(r+g+b)/3;
        }
		
		return m_labelCache[value];
}

wxImage &tcurrentOverlayFactory::DrawGLTextString( wxString myText ){

	wxString labels;
	labels = myText;
		
	wxMemoryDC mdc(wxNullBitmap);

    //wxFont mfont( 12, wxDEFAULT, wxNORMAL, wxBOLD );
	wxFont *pTCFont;
					pTCFont = wxTheFontList->FindOrCreateFont( 12, wxDEFAULT, wxNORMAL, wxBOLD, FALSE,
                                                   wxString( _T ( "Eurostile Extended" ) ) );	
    mdc.SetFont( *pTCFont);

    int w, h;
    mdc.GetTextExtent(labels, &w, &h);

    int label_offset = 10;   //5

    wxBitmap bm(w +  label_offset*2, h + 1);
    mdc.SelectObject(bm);
    mdc.Clear();

    wxColour text_color;

    GetGlobalColor( _T ("UINFD" ), &text_color );
    wxPen penText(text_color);
	mdc.SetPen(penText);

    mdc.SetBrush(*wxTRANSPARENT_BRUSH);
    mdc.SetTextForeground(text_color);
    mdc.SetTextBackground(wxTRANSPARENT);
          
    int xd = 0;
    int yd = 0;

    mdc.DrawText(labels, label_offset + xd, yd+1);          
    mdc.SelectObject(wxNullBitmap);

    m_labelCacheText[myText] = bm.ConvertToImage();

    m_labelCacheText[myText].InitAlpha();

    wxImage &image = m_labelCacheText[myText];

    unsigned char *d = image.GetData();
    unsigned char *a = image.GetAlpha();

    w = image.GetWidth(), h = image.GetHeight();
    for( int y = 0; y < h; y++ )
        for( int x = 0; x < w; x++ ) {
            int r, g, b;
            int ioff = (y * w + x);
            r = d[ioff* 3 + 0];
            g = d[ioff* 3 + 1];
            b = d[ioff* 3 + 2];

            a[ioff] = 255-(r+g+b)/3;
        }
	
	return m_labelCacheText[myText];
}

void tcurrentOverlayFactory::DrawGLLine( double x1, double y1, double x2, double y2, double width, wxColour myColour )
{
    {
        wxColour isoLineColor = myColour;
		//GetGlobalColor ( _T ( "UITX1" ), &isoLineColor );
		glColor4ub(isoLineColor.Red(), isoLineColor.Green(), isoLineColor.Blue(),
                     255/*isoLineColor.Alpha()*/);

		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_ENABLE_BIT |
                     GL_POLYGON_BIT | GL_HINT_BIT ); //Save state
        {

            //      Enable anti-aliased lines, at best quality
            glEnable( GL_LINE_SMOOTH );
            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
            glLineWidth( width );

            glBegin( GL_LINES );
            glVertex2d( x1, y1 );
            glVertex2d( x2, y2 );
            glEnd();
        }

        glPopAttrib();
    }
}
void tcurrentOverlayFactory::DrawOLBitmap( const wxBitmap &bitmap, wxCoord x, wxCoord y, bool usemask )
{
    wxBitmap bmp;
    if( x < 0 || y < 0 ) {
        int dx = ( x < 0 ? -x : 0 );
        int dy = ( y < 0 ? -y : 0 );
        int w = bitmap.GetWidth() - dx;
        int h = bitmap.GetHeight() - dy;
        /* picture is out of viewport */
        if( w <= 0 || h <= 0 ) return;
        wxBitmap newBitmap = bitmap.GetSubBitmap( wxRect( dx, dy, w, h ) );
        x += dx;
        y += dy;
        bmp = newBitmap;
    } else {
        bmp = bitmap;
    }
    if( m_pdc )
        m_pdc->DrawBitmap( bmp, x, y, usemask );
    else {
        wxImage image = bmp.ConvertToImage();
        int w = image.GetWidth(), h = image.GetHeight();

        if( usemask ) {
            unsigned char *d = image.GetData();
            unsigned char *a = image.GetAlpha();

            unsigned char mr, mg, mb;
            if( !image.GetOrFindMaskColour( &mr, &mg, &mb ) && !a ) printf(
                    "trying to use mask to draw a bitmap without alpha or mask\n" );

            unsigned char *e = new unsigned char[4 * w * h];
            {
                for( int y = 0; y < h; y++ )
                    for( int x = 0; x < w; x++ ) {
                        unsigned char r, g, b;
                        int off = ( y * image.GetWidth() + x );
                        r = d[off * 3 + 0];
                        g = d[off * 3 + 1];
                        b = d[off * 3 + 2];

                        e[off * 4 + 0] = r;
                        e[off * 4 + 1] = g;
                        e[off * 4 + 2] = b;

                        e[off * 4 + 3] =
                                a ? a[off] : ( ( r == mr ) && ( g == mg ) && ( b == mb ) ? 0 : 255 );
                    }
            }

            glColor4f( 1, 1, 1, 1 );

            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glRasterPos2i( x, y );
            glPixelZoom( 1, -1 );
            glDrawPixels( w, h, GL_RGBA, GL_UNSIGNED_BYTE, e );
            glPixelZoom( 1, 1 );
            glDisable( GL_BLEND );

            delete[] ( e );
        } else {
            glRasterPos2i( x, y );
            glPixelZoom( 1, -1 ); /* draw data from top to bottom */
            glDrawPixels( w, h, GL_RGB, GL_UNSIGNED_BYTE, image.GetData() );
            glPixelZoom( 1, 1 );
        }
    }
}

void tcurrentOverlayFactory::DrawGLLabels(tcurrentOverlayFactory *pof, wxDC *dc,
                                PlugIn_ViewPort *vp, int density, int first,
                                wxImage &imageLabel, double myLat, double myLon, int offset )
{

    //---------------------------------------------------------
    // Ecrit les labels
    //---------------------------------------------------------
        
         wxPoint ab;
         GetCanvasPixLL(vp, &ab, myLat, myLon);
                 
	     wxPoint cd;
         GetCanvasPixLL(vp, &cd,myLat, myLon);
                
         int w = imageLabel.GetWidth();
         int h = imageLabel.GetHeight();

         int label_offset = 0;
         int xd = (ab.x + cd.x-(w+label_offset * 2))/2;
         int yd = (ab.y + cd.y - h)/2 + offset;
                
         if(dc) {
                    /* don't use alpha for isobars, for some reason draw bitmap ignores
                       the 4th argument (true or false has same result) */
                    wxImage img(w, h, imageLabel.GetData(), true);
                    dc->DrawBitmap(img, xd, yd, false);
         } 
		 else { /* opengl */
                  
			int w = imageLabel.GetWidth(), h = imageLabel.GetHeight();

            unsigned char *d = imageLabel.GetData();
            unsigned char *a = imageLabel.GetAlpha();

            unsigned char mr, mg, mb;
            if( !imageLabel.GetOrFindMaskColour( &mr, &mg, &mb ) && !a ) wxMessageBox(_T(
                    "trying to use mask to draw a bitmap without alpha or mask\n" ));

            unsigned char *e = new unsigned char[4 * w * h];
            {
                for( int y = 0; y < h; y++ )
                    for( int x = 0; x < w; x++ ) {
                        unsigned char r, g, b;
                        int off = ( y * imageLabel.GetWidth() + x );
                        r = d[off * 3 + 0];
                        g = d[off * 3 + 1];
                        b = d[off * 3 + 2];

                        e[off * 4 + 0] = r;
                        e[off * 4 + 1] = g;
                        e[off * 4 + 2] = b;

                        e[off * 4 + 3] =
                                a ? a[off] : ( ( r == mr ) && ( g == mg ) && ( b == mb ) ? 0 : 255 );
                    }
            }

            glColor4f( 1, 1, 1, 1 );

            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glRasterPos2i( xd, yd );
            glPixelZoom( 1, -1 );
            glDrawPixels( w, h, GL_RGBA, GL_UNSIGNED_BYTE, e );
            glPixelZoom( 1, 1 );
            glDisable( GL_BLEND );

            delete[] ( e );

      }


}

wxImage &tcurrentOverlayFactory::DrawGLPolygon(){

	wxString labels;
	labels = _T("");  // dummy label for drawing with
	
	wxColour c_orange = c_GLcolour;

    wxPen penText(c_orange);
    wxBrush backBrush(c_orange);

    wxMemoryDC mdc(wxNullBitmap);

    wxFont mfont( 9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
    mdc.SetFont( mfont );

    int w, h;
    mdc.GetTextExtent(labels, &w, &h);
	
	w = 200;
	h = 200;

    wxBitmap bm(w, h);
    mdc.SelectObject(bm);
    mdc.Clear();

    mdc.SetPen(penText);
    mdc.SetBrush(backBrush);
    mdc.SetTextForeground(c_orange);
    mdc.SetTextBackground(c_orange);
          
    int xd = 0;
    int yd = 0;
   // mdc.DrawRoundedRectangle(xd, yd, w+(label_offset * 2), h+2, -.25);
/*
		
	                wxPoint p2;  
					p2.x = 100;
					p2.y = 100 ;

	                wxPoint z[9];
					z[0].x = p2.x;
                    z[0].y = p2.y;
                    z[1].x = p2.x;
                    z[1].y = p2.y -10 ;
                    z[2].x = p2.x + 55;
                    z[2].y = p2.y - 10;
                    z[3].x = p2.x + 55;
                    z[3].y = p2.y -25;
					z[4].x = p2.x + 100;
                    z[4].y = p2.y;
                    z[5].x = p2.x +55;
                    z[5].y = p2.y + 25 ;
                    z[6].x = p2.x + 55;
                    z[6].y = p2.y + 10;
                    z[7].x = p2.x;
                    z[7].y = p2.y +10;
					z[8].x = p2.x;
                    z[8].y = p2.y;

	*/
    
	mdc.DrawPolygon(9,p_basic,0);
    //mdc.DrawRectangle(xd, yd, w+(label_offset * 2), h+2);
    //mdc.DrawText(labels, label_offset + xd, yd+1);
          
    mdc.SelectObject(wxNullBitmap);

    m_labelCacheText[labels] = bm.ConvertToImage();

    m_labelCacheText[labels].InitAlpha();

    wxImage &image = m_labelCacheText[labels];

    unsigned char *d = image.GetData();
    unsigned char *a = image.GetAlpha();

    w = image.GetWidth(), h = image.GetHeight();
    for( int y = 0; y < h; y++ )
        for( int x = 0; x < w; x++ ) {
            int r, g, b;
            int ioff = (y * w + x);
            r = d[ioff* 3 + 0];
            g = d[ioff* 3 + 1];
            b = d[ioff* 3 + 2];

            a[ioff] = 255-(r+g+b)/3;
        }

    return m_labelCacheText[labels];
	/*
	                wxPoint p;  
					p.x = 200;
					p.y = 200;

	                wxPoint z[9];
					z[0].x = p.x;
                    z[0].y = p.y;
                    z[1].x = p.x;
                    z[1].y = p.y -10 ;
                    z[2].x = p.x + 55;
                    z[2].y = p.y - 10;
                    z[3].x = p.x + 55;
                    z[3].y = p.y -25;
					z[4].x = p.x + 100;
                    z[4].y = p.y;
                    z[5].x = p.x +55;
                    z[5].y = p.y + 25 ;
                    z[6].x = p.x + 55;
                    z[6].y = p.y + 10;
                    z[7].x = p.x;
                    z[7].y = p.y +10;
					z[8].x = p.x;
                    z[8].y = p.y;

	
    
	*/

}
void tcurrentOverlayFactory::drawGLPolygons(tcurrentOverlayFactory *pof, wxDC *dc,
                                PlugIn_ViewPort *vp, int density, int first,
                                wxImage &imageLabel, double myLat, double myLon, int offset )
{

    //---------------------------------------------------------
    // Ecrit les labels
    //---------------------------------------------------------
        
         wxPoint ab;
         GetCanvasPixLL(vp, &ab, myLat, myLon);
                 
	     wxPoint cd;
         GetCanvasPixLL(vp, &cd,myLat, myLon);
                
         int w = imageLabel.GetWidth();
         int h = imageLabel.GetHeight();

         int label_offset = 0;
         int xd = (ab.x + cd.x-(w+label_offset * 2))/2;
         int yd = (ab.y + cd.y - h)/2 + offset;
                
         if(dc) {
                    /* don't use alpha for isobars, for some reason draw bitmap ignores
                       the 4th argument (true or false has same result) */
                    wxImage img(w, h, imageLabel.GetData(), true);
                    dc->DrawBitmap(img, xd, yd, false);
         } 
		 else { /* opengl */
                  
			int w = imageLabel.GetWidth(), h = imageLabel.GetHeight();

            unsigned char *d = imageLabel.GetData();
            unsigned char *a = imageLabel.GetAlpha();

            unsigned char mr, mg, mb;
           if( !imageLabel.GetOrFindMaskColour( &mr, &mg, &mb ) && !a ) wxMessageBox(_T(
                    "trying to use mask to draw a bitmap without alpha or mask\n" ));

            unsigned char *e = new unsigned char[4 * w * h];
            {
                for( int y = 0; y < h; y++ )
                    for( int x = 0; x < w; x++ ) {
                        unsigned char r, g, b;
                        int off = ( y * imageLabel.GetWidth() + x );
                        r = d[off * 3 + 0];
                        g = d[off * 3 + 1];
                        b = d[off * 3 + 2];

                        e[off * 4 + 0] = r;
                        e[off * 4 + 1] = g;
                        e[off * 4 + 2] = b;

                        e[off * 4 + 3] =
                                a ? a[off] : ( ( r == mr ) && ( g == mg ) && ( b == mb ) ? 0 : 255 );
                    }
            }
			
            glColor4f( 1, 1, 1, 1 );

            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glRasterPos2i( xd, yd );
            glPixelZoom( 1, -1 );
            glDrawPixels( w, h, GL_RGBA, GL_UNSIGNED_BYTE, e );
            glPixelZoom( 1, 1 );
            glDisable( GL_BLEND );

            delete[] ( e );

      }


}


                   