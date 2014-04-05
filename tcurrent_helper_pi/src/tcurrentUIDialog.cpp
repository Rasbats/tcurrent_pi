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

#include "wx/wx.h"
#include "wx/tokenzr.h"
#include "wx/datetime.h"
#include "wx/sound.h"
#include <wx/wfstream.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/debug.h>
#include <wx/graphics.h>

#include <wx/stdpaths.h>

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "tcurrent_pi.h"
#include "folder.xpm"
#include "icons.h"

#include <wx/arrimpl.cpp>
#include <vector>

#include "resource.h"
#include <iostream> 
#include <fstream>
#include <windows.h>
#include <memory.h> 

enum
{
            FORWARD_ONE_HOUR_STEP    =3600,
            FORWARD_TEN_MINUTES_STEP =600,
            FORWARD_ONE_MINUTES_STEP =60,
            BACKWARD_ONE_HOUR_STEP    =-3600,
            BACKWARD_TEN_MINUTES_STEP =-600,
            BACKWARD_ONE_MINUTES_STEP =-60,
			// menu items   
        CLIENT_QUIT = 10000,   
        CLIENT_OPEN,   
       
        // id for socket   
        SOCKET_ID   
};

                 // Handle to DLL
HINSTANCE hinstDLL;
using namespace std;


#if defined (_WIN32)
int round (double x) {
	int i = (int) x;
	if (x >= 0.0) {
		return ((x-i) >= 0.5) ? (i + 1) : (i);
	} else {
		return (-x+i >= 0.5) ? (i - 1) : (i);
	}
}
#endif

#define FAIL(X) do { error = X; goto failed; } while(0)

//WX_DEFINE_OBJARRAY( ArrayOftcurrentRecordSets );

enum { SAILDOCS,ZYtcurrent };
enum { GFS,COAMPS,RTOFS };


//date/time in the desired time zone format
static wxString TToString( const wxDateTime date_time, const int time_zone )
{
    wxDateTime t( date_time );
    t.MakeFromTimezone( wxDateTime::UTC );
    if( t.IsDST() ) t.Subtract( wxTimeSpan( 1, 0, 0, 0 ) );
    switch( time_zone ) {
        case 0: return t.Format( _T(" %a %d-%b-%Y  %H:%M "), wxDateTime::Local ) + _T("LOC");//:%S
        case 1:
        default: return t.Format( _T(" %a %d-%b-%Y %H:%M  "), wxDateTime::UTC ) + _T("UTC");
    }
}

#if !wxCHECK_VERSION(2,9,4) /* to work with wx 2.8 */
#define SetBitmap SetBitmapLabel
#endif


tcurrentUIDialog::tcurrentUIDialog(wxWindow *parent, tcurrent_pi *ppi)
: tcurrentUIDialogBase(parent)
{
    mBool = false;
	pParent = parent;
    pPlugIn = ppi;

    wxFileConfig *pConf = GetOCPNConfigObject();

    if(pConf) {
        pConf->SetPath ( _T ( "/Settings/tcurrent" ) );

		pConf->Read ( _T ( "tcurrentUseRate" ), &m_bUseRate );
        pConf->Read ( _T ( "tcurrentUseDirection" ), &m_bUseDirection);
		pConf->Read ( _T ( "tcurrentUseFillColour" ), &m_bUseFillColour);

		pConf->Read ( _T ( "tcurrentPort" ), &m_PortSelected);
    }

    m_bpPrev->SetBitmap(wxBitmap( prev1 ));
    m_bpNext->SetBitmap(wxBitmap( next1 ));
    m_bpNow->SetBitmap(*_img_Clock );

    this->Connect( wxEVT_MOVE, wxMoveEventHandler( tcurrentUIDialog::OnMove ) );

    DimeWindow( this );

    Fit();
    SetMinSize( GetBestSize() );

	LoadStandardPorts();  // From StandardPorts.xml Load the port choice control
	m_choice1->SetStringSelection(m_PortSelected);  // from opencpn.ini
	LoadHarmonics();  // Open the tcdata tidal information
	OpenXML();  // Fill myPositions (tidal diamonds) with tidal current information
	OpenFile();	// Set up variables
	OnStartSetupHW(); // Set up the HW control and information text controls
}

tcurrentUIDialog::~tcurrentUIDialog()
{
    wxFileConfig *pConf = GetOCPNConfigObject();;

    if(pConf) {
        pConf->SetPath ( _T ( "/Settings/tcurrent" ) );

		pConf->Write ( _T ( "tcurrentUseRate" ), m_bUseRate );
		pConf->Write ( _T ( "tcurrentUseDirection" ), m_bUseDirection );
		pConf->Write ( _T ( "tcurrentUseFillColour" ), m_bUseFillColour );

		int c = m_choice1->GetSelection();
		wxString myP = m_choice1->GetString(c);
		pConf->Write ( _T ( "tcurrentPort" ), myP );  

    }
}

void tcurrentUIDialog::SetCursorLatLon( double lat, double lon )
{
    m_cursor_lon = lon;
    m_cursor_lat = lat;

}

void tcurrentUIDialog::SetViewPort( PlugIn_ViewPort *vp )
{
    if(m_vp == vp)  return;

    m_vp = new PlugIn_ViewPort(*vp);

}

void tcurrentUIDialog::OnClose( wxCloseEvent& event )
{
    pPlugIn->OntcurrentDialogClose();
}

void tcurrentUIDialog::OnMove( wxMoveEvent& event )
{
    //    Record the dialog position
    wxPoint p = GetPosition();
    pPlugIn->SettcurrentDialogX( p.x );
    pPlugIn->SettcurrentDialogY( p.y );

    event.Skip();
}

void tcurrentUIDialog::OnSize( wxSizeEvent& event )
{
    //    Record the dialog size
    wxSize p = event.GetSize();
    pPlugIn->SettcurrentDialogSizeX( p.x );
    pPlugIn->SettcurrentDialogSizeY( p.y );

    event.Skip();
}

void tcurrentUIDialog::OpenFile(bool newestFile)
{
	m_bUseRate = pPlugIn->GetCopyRate();
	m_bUseDirection = pPlugIn->GetCopyDirection();
	m_bUseFillColour = pPlugIn->GetCopyColour();


	button_id = 6; //High Water
	next_id	= 7;
	back_id   = 5;
	m_myChoice = 0;

	label_array[0] = wxT("HW-6");
	label_array[1] = wxT("HW-5");
	label_array[2] = wxT("HW-4");
	label_array[3] = wxT("HW-3");
	label_array[4] = wxT("HW-2");
	label_array[5] = wxT("HW-1");
	label_array[6] = wxT("High Water");
	label_array[7] = wxT("HW+1");
	label_array[8] = wxT("HW+2");
	label_array[9] = wxT("HW+3");
	label_array[10] = wxT("HW+4");
	label_array[11] = wxT("HW+5");
	label_array[12] = wxT("HW+6");

}

void tcurrentUIDialog::OnStartSetupHW()
{
   	int m = m_choice1->GetSelection();
	wxString s = m_choice1->GetString(m);
	m_portXML = FindPortXMLUsingChoice(s);

	if (m_portXML ==  _T(""))
	{
		wxMessageBox(_T("Port not found"), _T("Port finder"));
		return;	
	}

	int i = FindPortIDUsingChoice(s);

	if (i == 0)
	{
		wxMessageBox(_T("No tidal data"));
		m_staticText2->SetLabel(_T(""));
		m_staticText211->SetLabel(_T("High Water"));
		button_id = 6;
		return;
	}
	else
	{
	    CalcHW(i);
		SetCorrectHWSelection();
		myDateSelection = m_choice2->GetSelection();	
		wxString st_mydate = m_choice2->GetString(myDateSelection);
		m_dt.ParseDateTime(st_mydate);
		m_ts = wxTimeSpan::Hours(0) ;

		m_myChoice = myDateSelection;
		m_staticText2->SetLabel(st_mydate);
		m_staticText211->SetLabel(_T("High Water"));
		button_id = 6;
		return;
	}			
						
}


void tcurrentUIDialog::OnNow( wxCommandEvent& event )
{   	
		SetDateForNowButton();

		button_id = 6 + CalcHoursFromHWNow();
		if ( button_id == 13)
		{
			button_id = 12;
		}
		if ( button_id == -1)
		{
			button_id = 0;
		}

		m_staticText2->SetLabel(label_array[button_id]);
		
		wxDateTime this_now = wxDateTime::Now();
		wxString s0 = this_now.Format( _T ( "%a %d %b %Y"));
		wxString s1 = this_now.Format(_T("%H:%M"));			
		wxString s2 = s0 + _(" ") + s1;			
						
		m_staticText211->SetLabel(s2);

	
		SetCorrectHWSelection();
		m_myChoice = m_choice2->GetSelection();

		switch (button_id) {  // And make the label depending HW+6, HW-6 etc
		case 0:
			{
				next_id = 1;
				back_id = 12;
				m_myChoice--;
				break;
			}
		case 12:
			{
				next_id = 0;
				back_id = 11;
				m_myChoice++;
				break;
			}

		default:
			{
				next_id = button_id + 1;
				back_id = button_id - 1;			    
			}
		}

	RequestRefresh(pParent);
}


   


void tcurrentUIDialog::SetCorrectHWSelection()
{
	int i, c, s, t, t1, t2;
	wxDateTime d1,d2;

	c = m_choice2->GetCount();
	wxDateTime myChoiceDates[6], m_dt;

	c = m_choice2->GetCount();
	for  (i=0; i<c; i++)
	{				
		
		myChoiceDates[i].ParseDateTime(m_choice2->GetString(i));

	}
	
	wxDateTime this_now = wxDateTime::Now();
	t = this_now.GetTicks();

	for  (i=0; i<c; i++)
	{		
		    d1 = myChoiceDates[i];
			t1 = d1.GetTicks() - 6*60*60;
			d2 = myChoiceDates[i];
			t2 = d2.GetTicks() + 6*60*60;
			
			if (( t > t1  )  && (t < t2 ) )
			{
				s = i;
				i = 100;
				m_choice2->SetSelection(s);
				return;
			}
			else { s = 0;}


	}

	m_choice2->SetSelection(0);

}

void tcurrentUIDialog::OnDateSelChanged(wxDateEvent& event)
{
	m_staticText2->SetLabel(_T("  "));
	m_staticText211->SetLabel(_T("  "));
														
	//    Clear the Choice ListBox
	m_choice2->Clear();

	int m = m_choice1->GetSelection();
	wxString s = m_choice1->GetString(m);

	int i = FindPortIDUsingChoice(s);

	if (i == 0)
	{
		wxMessageBox(_T("No tidal data"));
		return;
	}
	CalcHW(i);
	SetCorrectHWSelection();
}

void tcurrentUIDialog::OnPortChanged(wxCommandEvent& event)
{	 
	 
	m_staticText2->SetLabel(_T("  "));
	m_staticText211->SetLabel(_T("  "));

	int m = m_choice1->GetSelection();
	wxString s = m_choice1->GetString(m);

	 int i = FindPortIDUsingChoice(s);
	 if (i == 0)
	 {		 
		 m_choice2->Clear();
		 wxMessageBox(_T("No tidal data found"),_T("Tidal Data"));
		 return;
	 }
	 else
	 {   
		CalcHW(i);
		SetCorrectHWSelection();
	 }
}


void tcurrentUIDialog::SetDateForNowButton()
{
	wxDateTime this_now = wxDateTime::Now();
    m_datePicker1->SetValue(this_now);

	int m = m_choice1->GetSelection();
	wxString string = m_choice1->GetString(m);
	m_portXML = FindPortXMLUsingChoice(string);	

	if (m_portXML ==  _T(""))
	{
		wxMessageBox(_T("Port not found"), _T("Port finder"));
		return;	
	}


	int id = FindPortIDUsingChoice(string);

	if (id == 0)
	{
		wxMessageBox(_T("No tidal data"));
		m_staticText2->SetLabel(_T(""));
		m_staticText211->SetLabel(_T("High Water"));
		button_id = 6;
		back_id = 5;
		next_id = 7;
		RequestRefresh(pParent);
		return;
	}
	else
	{
        		    		
	CalcHW(id);	

	int i, c, s, t, t1, t2;
	wxDateTime d1,d2;

	c = m_choice2->GetCount();
	wxDateTime myChoiceDates[6], m_dt;

	c = m_choice2->GetCount();
	for  (i=0; i<c; i++)
	{				
		
		myChoiceDates[i].ParseDateTime(m_choice2->GetString(i));

	}
	
	t = this_now.GetTicks();

	for  (i=0; i<c; i++)
	{		
		    d1 = myChoiceDates[i];
			t1 = d1.GetTicks() - 6*60*60;
			d2 = myChoiceDates[i];
			t2 = d2.GetTicks() + 6*60*60;
			
			if (( t > t1  )  && (t < t2 ) )
			{
				s = i;
				i = 100;
				m_choice2->SetSelection(s); 
				return;
			}
			else { s = 999;}


	}

	if (s == 999){

		if (t > t2){
			wxDateSpan myOneDay = wxDateSpan::Days(1);
			wxDateTime myDate = m_datePicker1->GetValue();		
			myDate.Add(myOneDay);
			m_datePicker1->SetValue(myDate);
			CalcHW(id);
			m_choice2->SetSelection(0);
			return;
		}

		if (t < t1){
			wxDateSpan myOneDay = wxDateSpan::Days(1);
			wxDateTime myDate = m_datePicker1->GetValue();		
			myDate.Subtract(myOneDay);
			m_datePicker1->SetValue(myDate);
			CalcHW(id);
			c = m_choice2->GetCount();
			m_choice2->SetSelection(c-1);
			return;
		}
	}
  }
}

PortTides tcurrentUIDialog::PopulatePortTides(wxString PortName)
{
	PortTides myPortTides;
	int m = m_choice1->GetSelection();
	wxString s = m_choice1->GetString(m);
	double spRange, npRange;

	int i = 0;
		for(std::vector<StandardPort>::iterator it = my_ports.begin();  it != my_ports.end(); it++){
           
			myPortTides.m_portID = (*it).PORT_NUMBER; 
			myPortTides.m_portName = (*it).PORT_NAME; 
			(*it).MEAN_SPRING_RANGE.ToDouble(&spRange);
			myPortTides.m_spRange = spRange;
			(*it).MEAN_NEAP_RANGE.ToDouble(&npRange);
			myPortTides.m_npRange = npRange;

			if (myPortTides.m_portName == s)
			   return myPortTides;						
			
			i++;
		}
		myPortTides.m_portName = _T("None");
		return myPortTides;
}

wxString tcurrentUIDialog::FindPortXMLUsingChoice(wxString inPortName)
{
	PortTides myPortTides;	
	
	wxString s = inPortName;

	int i = 0;
		for(std::vector<StandardPort>::iterator it = my_ports.begin();  it != my_ports.end(); it++){
           
			myPortTides.m_portID = (*it).PORT_NUMBER; 
			myPortTides.m_portName = (*it).PORT_NAME; 
			myPortTides.m_IDX = (*it).IDX; 

			if (myPortTides.m_portName == s)
			{
				return myPortTides.m_portID;						
			}

			i++;
		}

		return _T("");
}

int tcurrentUIDialog::FindPortIDUsingChoice(wxString inPortName)
{
	int i;	
	wxString s = inPortName;
	PortTides myPortTides;

	i = 0;
		for(std::vector<StandardPort>::iterator it = my_ports.begin();  it != my_ports.end(); it++){
           
			myPortTides.m_portID = (*it).PORT_NUMBER; 
			myPortTides.m_portName = (*it).PORT_NAME; 
			myPortTides.m_IDX = (*it).IDX; 

			if (myPortTides.m_portName == s){
				return FindPortID(myPortTides.m_IDX);					        					
			}
			
			i++;
		}
  return 0;
}
void tcurrentUIDialog::LoadHarmonics()
{
	  //  Establish a "home" location
        
	  g_SData_Locn = *GetpSharedDataLocation();

      // Establish location of Tide and Current data
      pTC_Dir = new wxString(_T("tcdata"));
      pTC_Dir->Prepend(g_SData_Locn);
      pTC_Dir->Append(wxFileName::GetPathSeparator());  
	
      wxString TCDir;
      TCDir = *pTC_Dir;
      
      wxLogMessage(_T("Using Tide/Current data from:  ") + TCDir);
	  wxString cache_locn = TCDir; 

	  wxString harm2test = TCDir;
      harm2test.Append( _T("HARMONIC") );
	
	  ptcmgr = new TCMgr(TCDir, cache_locn);     	
}

int tcurrentUIDialog::FindPortID(wxString myPort)
{	
	        for ( int i=1 ; i<ptcmgr->Get_max_IDX() +1 ; i++ )
            {				
						IDX_entry *pIDX = ptcmgr->GetIDX_entry (i);

                        char type = pIDX->IDX_type;             // Entry "TCtcIUu" identifier
                        if ( ( type == 't' ) ||  ( type == 'T' ) )  // only Tides
                        {                              
							  wxString s = wxString(pIDX->IDX_reference_name,wxConvUTF8); 
							  if ( s == myPort)
							  {								  
								  return i;
							  }							  
						}

			}
			return 0;
}
void tcurrentUIDialog::CalcHW(int PortCode)
{
	m_choice2->Clear();	

	if (PortCode == 0)
	{
		wxMessageBox(_T("No tidal data for this port"), _T("No Tidal Data"));
		return;
	}
	//    Figure out this computer timezone minute offset
        wxDateTime this_now = wxDateTime::Now();
        wxDateTime this_gmt = this_now.ToGMT();

#if wxCHECK_VERSION(2, 6, 2)
        wxTimeSpan diff = this_now.Subtract ( this_gmt );
#else
        wxTimeSpan diff = this_gmt.Subtract ( this_now );
#endif		        
		int diff_mins = diff.GetMinutes();	

		IDX_entry *pIDX = ptcmgr->GetIDX_entry ( PortCode );
		int station_offset = ptcmgr->GetStationTimeOffset(pIDX); //-60 for French Harmonics_V7.zip

        m_corr_mins = station_offset - diff_mins;
        if ( this_now.IsDST() )
              m_corr_mins += 60;				
		

		//    Establish the inital drawing day as today
        m_graphday = m_datePicker1->GetValue();
        wxDateTime graphday_00 = m_datePicker1->GetValue();
        time_t t_graphday_00 = graphday_00.GetTicks();

        //    Correct a Bug in wxWidgets time support
        if ( !graphday_00.IsDST() && m_graphday.IsDST() )
                t_graphday_00 -= 3600;
        if ( graphday_00.IsDST() && !m_graphday.IsDST() )
                t_graphday_00 += 3600;

        m_t_graphday_00_at_station = t_graphday_00 - ( m_corr_mins * 60 );

        //Get the timezone of the station
        int h = (station_offset / 60);
		int m = station_offset - (h * 60);
        if ( m_graphday.IsDST() )
              h += 1;
        m_stz.Printf(_T("Z %+03d:%02d"), h, m);
		m_staticText1->SetLabel(m_stz);
		//

		int i, c, n, e;
		c = 0;
		e = 0;
		double myArrayOfRanges[6];

		float tcmax, tcmin;
		float dir;
                        
		tcmax = -10;
        tcmin = 10;
        
		float val = 0;
		int list_index = 0 ;
		int array_index = 0;
        bool  wt = 0;
		float myLW, myHW;

		wxString sHWLW = _T("");

                        // get tide flow sens ( flood or ebb ? )
						
                        ptcmgr->GetTideFlowSens(m_t_graphday_00_at_station, BACKWARD_ONE_HOUR_STEP, pIDX->IDX_rec_num, tcv[0], val, wt);
		
						for ( i=0 ; i<26 ; i++ )
                        {
                                int tt = m_t_graphday_00_at_station + ( i * FORWARD_ONE_HOUR_STEP );
                                ptcmgr->GetTideOrCurrent ( tt, pIDX->IDX_rec_num, tcv[i], dir );
								
                                if ( tcv[i] > tcmax )
                                        tcmax = tcv[i];

                                                if ( tcv[i] < tcmin )
                                                   tcmin = tcv[i];                                                
                                                    if ( ! ((tcv[i] > val) == wt) )                // if tide flow sens change
                                                    {
                                                      float tcvalue;                                        //look backward for HW or LW
                                                      time_t tctime;
                                                      ptcmgr->GetHightOrLowTide(tt, BACKWARD_TEN_MINUTES_STEP, BACKWARD_ONE_MINUTES_STEP, tcv[i], wt, pIDX->IDX_rec_num, tcvalue, tctime);

                                                      wxDateTime tcd ;                                                              //write date
                                                      wxString s, s1, s2;
                                                      tcd.Set( tctime + ( m_corr_mins * 60 ) ) ;

													  s2 = tcd.Format ( _T ( "%a %d %b %Y"));
                                                      s.Printf(tcd.Format(_T("%H:%M  ")));													 

                                                      s1.Printf( _T("%05.2f "),tcvalue);    												  
	
													  Station_Data *pmsd = pIDX->pref_sta_data;                         //write unit
 													  
													  
													  ( wt )? sHWLW = _("HW") : sHWLW = _("LW"); 
													 											                                                        
													  // Fill the array with tide data
													  euTC[array_index][0] = s2 + _(" ") + s;													  													
													  euTC[array_index][1] = s1;
													  euTC[array_index][2] = wxString(pmsd->units_abbrv ,wxConvUTF8);
													  euTC[array_index][3] = sHWLW;

													  if (euTC[array_index][3] == _("LW")) 
													  {									
														myLW = tcvalue;
													  }
													  
													  if (euTC[array_index][3] == _("HW")) 
													  {
														myHW = tcvalue;
														m_choice2->Insert(euTC[array_index][0],list_index); 
														// nearestHW for the now button
														nearestHW[e] = euTC[array_index][0];
														e++;														
														list_index++;
													  }  

													   myRange = myHW - myLW;
													 
													  if ((abs(myRange) == myHW) || (abs(myRange) == myLW))
													  {
															// out of range
													  }
													  else
													  {
														  myArrayOfRanges[c] = myRange;
														  c++;
													  }
														
													  array_index++;
													  
                                                      wt = !wt ;     //change tide flow sens

                                                    }

													val = tcv[i];                                                                                                
                        }
						c--;
						n = 0;
						double AddRanges = 0;
						for (n; n<c; n++){
						   AddRanges = AddRanges + myArrayOfRanges[n];
						}
						// myRange for the speed of current calculation
						myRange = AddRanges/n;												
}

double tcurrentUIDialog::CalcCurrent(double m_spRange, double m_npRange, double m_spRateDiamond, double m_npRateDiamond, double m_rangeOnDay)
{
	if (m_spRateDiamond == m_npRateDiamond)
		return m_spRateDiamond;
	else {
		// y = mx + c
		double m,c,x; 
		m = (m_spRange - m_npRange) / (m_spRateDiamond - m_npRateDiamond);
		c = m_spRange - (m * m_spRateDiamond);
		x = (m_rangeOnDay - c)/m ;	
		return x;
	}
}


int tcurrentUIDialog::CalcHoursFromHWNow()
{
	
	wxDateTime myDateTime;
	wxTimeSpan diff;
	double myDiff, myTest;

	myTest = 26;
	
	wxDateTime this_now = wxDateTime::Now();
	int t = this_now.GetTicks();
	int i = 0;
	int m; 		
	double d;

	for (i; i<8;i++)
	{
		myDateTime.ParseDateTime(nearestHW[i]);
		m = myDateTime.GetTicks(); 

		d = t - m;
		myDiff = (d/60)/60;  
		
		if (abs(myDiff) < abs(myTest))
		{
			myTest = myDiff;
		}
	}		

	int c = m_choice2->GetCount();
	for  (c=0; c<8; c++)
	{
		for (i=0; i<8;i++)
		{
		if (m_choice2->GetString(c) == nearestHW[i])
			{
				m_choice2->SetSelection(c);
			}
		}
	}
	//m_myChoice = c;

	//wxString str_countPts =  wxString::Format(wxT("%f"), (double)myDiff);
    // wxMessageBox(str_countPts,_T("count_hours"));
	int f = round( myTest);   



    return f ;
}

int tcurrentUIDialog::round(double c)
{   
	// c = -0.52
	int a = c; //a =  0
	int b = 0;
	double input = c;

	if (a == 0)
	{
	  if (c < 0)
	  {
	  c = c + a;   // -0.52  
	  }
	  else	
	  {
		c = c - a;   //
	  }
	}
	else
	{
	c = c - a; //-2.6 --2 c = -0.6
	}
	
	if ( abs(c) > 0.5) 
	{
		b = 1;  //1
	}
	else
	{
		b = 0;
	}
	
	if ( a > 0) //a -2
	{
		c = a + b;
	}
	else{
		if (a == 0){  
			
			if (input >= 0){
				c = b;
			}
			else{				
				c -= b;
			}   
		}
		else{		
			c = a - b;
		}
	}
	//wxString str_countPts =  wxString::Format(wxT("%d"), (int)c);
    // wxMessageBox(str_countPts,_T("count_hours"));
	return c;
}


bool tcurrentUIDialog::LoadStandardPorts()
{
	/*
	HRSRC hSrc2 = NULL;
    HMODULE hMod2 = GetModuleHandle(_T("tcurrent_pi.dll"));
    hSrc2 = FindResource(hMod2, MAKEINTRESOURCE(IDR_RCDATA2), RT_RCDATA);

    unsigned int myResourceSize = ::SizeofResource( hMod2, hSrc2);

    HGLOBAL myResourceData2 = ::LoadResource( hMod2, hSrc2);
    void* pMyBinaryData2 = ::LockResource(myResourceData2);
    
   char * data = (char*)pMyBinaryData2;
	*/
  //  myFile102 = _T("102");
    bool f_done = GetFile(_T("102"));
	
	
	if (f_done) {wxMessageBox(_T("Loaded Ports"));}
	//char* myData = (char*)myData102;

    TiXmlDocument doc;
    doc.Parse((const char*)myData102, 0, TIXML_ENCODING_UTF8);
	TiXmlElement *root = doc.RootElement();

	StandardPort my_port;
    my_ports.clear();
	int my_count = 0;    

			int count = 0;

			wxString wxPORT_NUMBER,wxPORT_NAME,wxMEAN_SPRING_RANGE,wxMEAN_NEAP_RANGE,wxEXTRA;			

			for(TiXmlElement* e = root->FirstChildElement(); e; e = e->NextSiblingElement())
            count++;			
			int i = 0;
			for(TiXmlElement* e = root->FirstChildElement(); e; e = e->NextSiblingElement(), i++) {
			

                for(TiXmlElement* f = e->FirstChildElement(); f; f = f->NextSiblingElement()) {
                    if(!strcmp(f->Value(), "PORT_NUMBER")) {
						wxPORT_NUMBER = wxString::FromUTF8(f->GetText());		
						my_port.PORT_NUMBER = wxPORT_NUMBER;
					}
					
					 if(!strcmp(f->Value(), "PORT_NAME")) {
						wxPORT_NAME = wxString::FromUTF8(f->GetText());
						my_port.PORT_NAME = wxPORT_NAME;
					}

					 if(!strcmp(f->Value(), "MEAN_SPRING_RANGE")) {
						wxMEAN_SPRING_RANGE = wxString::FromUTF8(f->GetText());
						my_port.MEAN_SPRING_RANGE = wxMEAN_SPRING_RANGE;
					}

					  if(!strcmp(f->Value(), "MEAN_NEAP_RANGE")) {
						wxMEAN_NEAP_RANGE = wxString::FromUTF8(f->GetText());						
						my_port.MEAN_NEAP_RANGE = wxMEAN_NEAP_RANGE;
						

					}
					   if(!strcmp(f->Value(), "EXTRA")) {
						wxEXTRA = wxString::FromUTF8(f->GetText());						
				        my_port.EXTRA = wxEXTRA;
						my_port.IDX = wxEXTRA;
						//wxMessageBox(my_position.lon,_T("myLon"));
					    my_ports.push_back(my_port);
					}  

											  					
				  //end for						
             }//end for
        }//end for

	for(std::vector<StandardPort>::iterator it = my_ports.begin();  it != my_ports.end(); it++)
	{           
			wxPortName[i][1] = (*it).PORT_NAME;
			m_choice1->Append(wxPortName[i][1]);
			i++;
	}	
	m_choice1->SetSelection( 0 );
 
    return true;
}

bool tcurrentUIDialog::OpenXML()
{/*
	HRSRC hSrc = NULL;
    HMODULE hMod = GetModuleHandle(_T("tcurrent_pi.dll"));
    hSrc = FindResource(hMod, MAKEINTRESOURCE(IDR_RCDATA1), RT_RCDATA);

    unsigned int myResourceSize = ::SizeofResource( hMod, hSrc);

    HGLOBAL myResourceData = ::LoadResource( hMod, hSrc);
    void* pMyBinaryData = ::LockResource(myResourceData);
    
	  // Saved for writing to file method
	std::ofstream f("C:\\old\\x.txt",std::ios::out | std::ios::binary);
    // write to outfile
    f.write((char*)pMyBinaryData, myResourceSize);
    f.close();

	*/
    //char * data = (char*)pMyBinaryData;
	
	bool f_done = GetFile(_T("101"));
	
	
	if (f_done){wxMessageBox(_T("Loaded Tidal Streams"));}

    TiXmlDocument doc;
    doc.Parse((const char*)myData102, 0, TIXML_ENCODING_UTF8);
	TiXmlElement *root = doc.RootElement();
   
	Position my_position;
	
    my_positions.clear(); // Clear all the diamonds of information

	int my_count = 0;    

			int count = 0;

			wxString rte_lat, rte_lon;
			wxString stat_num, port_num, minus_6, minus_5, minus_4, minus_3 ,minus_2, minus_1, zero;
			wxString plus_1, plus_2,  plus_3, plus_4, plus_5, plus_6;

			for(TiXmlElement* e = root->FirstChildElement(); e; e = e->NextSiblingElement())
            count++;			
			int i = 0;
			for(TiXmlElement* e = root->FirstChildElement(); e; e = e->NextSiblingElement(), i++) {
			

                for(TiXmlElement* f = e->FirstChildElement(); f; f = f->NextSiblingElement()) {
                    if(!strcmp(f->Value(), "LATITUDE")) {
						rte_lat = wxString::FromUTF8(f->GetText());
						my_position.lat = rte_lat;
					}
					
					if(!strcmp(f->Value(), "LONGITUDE")) {
						rte_lon = wxString::FromUTF8(f->GetText());
						my_position.lon = rte_lon;
					}	
					if(!strcmp(f->Value(), "PORT_NUMBER")) {
						port_num = wxString::FromUTF8(f->GetText());
						my_position.port_num = port_num;
					}
					if(!strcmp(f->Value(), "MINUS_6")) {
						minus_6 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[0] = minus_6;
					}
					if(!strcmp(f->Value(), "MINUS_5")) {
						minus_5 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[1] = minus_5;
					}
					if(!strcmp(f->Value(), "MINUS_4")) {
						minus_4 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[2] = minus_4;
					}
					if(!strcmp(f->Value(), "MINUS_3")) {
						minus_3 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[3] = minus_3;
					}
					if(!strcmp(f->Value(), "MINUS_2")) {
						minus_2 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[4] = minus_2;
					}
					if(!strcmp(f->Value(), "MINUS_1")) {
						minus_1 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[5] = minus_1;
					}
					if(!strcmp(f->Value(), "ZERO")) {
						zero = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[6] = zero;
					}
					if(!strcmp(f->Value(), "PLUS_1")) {
						plus_1 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[7] = plus_1;
					}
					if(!strcmp(f->Value(), "PLUS_2")) {
						plus_2 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[8] = plus_2;
					}
					if(!strcmp(f->Value(), "PLUS_3")) {
						plus_3 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[9] = plus_3;
					}
					if(!strcmp(f->Value(), "PLUS_4")) {
						plus_4 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[10] = plus_4;
					}
					if(!strcmp(f->Value(), "PLUS_5")) {
						plus_5 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[11] = plus_5;
					}
					if(!strcmp(f->Value(), "PLUS_6")) {
						plus_6 = wxString::FromUTF8(f->GetText());
						my_position.minus_plus[12] = plus_6;

					    my_positions.push_back(my_position); // End of diamond information. Push to diamond object	
					}    
					  
					
	                 //PORT_NUMBER
						
                }//end for

        }//end for

	RequestRefresh( pParent );
    return true;

}

vector<Position> tcurrentUIDialog::OnRecord()
{	
return my_positions;
}

void tcurrentUIDialog::SetFromHW(int fromHW) 
{

 button_id = fromHW;
}

void tcurrentUIDialog::OnChooseTideButton(wxCommandEvent & event)
{	
	
	int m = m_choice1->GetSelection();
	wxString s = m_choice1->GetString(m);
	m_portXML = FindPortXMLUsingChoice(s);

	if (m_portXML ==  _T(""))
	{
		wxMessageBox(_T("Port not found"), _T("Port finder"));
		return;	
	}

	wxString st_mydate;
	int i = FindPortIDUsingChoice(s);

	if (i == 0)
	{
		// No tidal data
		m_staticText2->SetLabel(_T(""));
		m_staticText211->SetLabel(_T("High Water"));
		button_id = 6;
		st_mydate = _T("");
	}
	else
	{
		myDateSelection = m_choice2->GetSelection();	
		st_mydate = m_choice2->GetString(myDateSelection);
		m_dt.ParseDateTime(st_mydate);
		m_ts = wxTimeSpan::Hours(0) ;
		m_myChoice = myDateSelection;

	}

	m_staticText2->SetLabel(_T(""));	
		
	button_id = event.GetId(); // Find which button was pushed (HW, HW-6, HW+6)`

	switch (button_id) {  // And make the label depending HW+6, HW-6 etc
		case 6:
			{
			next_id = 7;
			back_id = 5;
			m_myChoice = m_choice2->GetSelection();
		    m_staticText2->SetLabel(st_mydate);
		    break;
			}
		case 0:
			{
            next_id = 1;
			back_id = 12;
			m_myChoice = m_choice2->GetSelection() - 1;
			m_ts = wxTimeSpan::Hours(6) ;
			m_dt.Subtract(m_ts);
			wxString s = m_dt.Format( _T ( "%a %d %b %Y %H:%M"));
			if (st_mydate == _T(""))
				m_staticText2->SetLabel(_T(""));
			else
				m_staticText2->SetLabel(s); 
		    break;
			}
		case 12:
			{
			next_id = 0;
			back_id = 11;
			m_myChoice = m_choice2->GetSelection() + 1;
		    m_ts = wxTimeSpan::Hours(6) ;
			m_dt.Add(m_ts);
			wxString s = m_dt.Format( _T ( "%a %d %b %Y %H:%M"));
			if (st_mydate == _T(""))
				m_staticText2->SetLabel(_T(""));
			else
				m_staticText2->SetLabel(s);
		    break;
			}
	}

	m_staticText211->SetLabel(label_array[button_id]);
	RequestRefresh( pParent );

}

void tcurrentUIDialog::OnPrev( wxCommandEvent& event )
{
    wxString s;
	wxString st_mydate;
	int c, p;
	
	button_id = back_id;

	myDateSelection = m_choice2->GetSelection();
	st_mydate = m_choice2->GetString(myDateSelection);
	
	wxDateTime myDate;
	wxDateSpan myOneDay = wxDateSpan::Days(1);
	// Test if we have gone beyond the current list of HW.
	//

	if (m_myChoice < 0)
	{	
		if (st_mydate == _T(""))
		{
			m_myChoice = 0;
		}
		else
		{
			myDate = m_datePicker1->GetValue();	
	
			myDate.Subtract(myOneDay);
			m_datePicker1->SetValue(myDate);
			
			p = m_choice1->GetSelection();		// Get the port selected
			wxString s = m_choice1->GetString(p);
			m_portXML = FindPortXMLUsingChoice(s);
			int f = FindPortIDUsingChoice(s);
			
			CalcHW(f);			
			c = m_choice2->GetCount();
			m_myChoice = c-1;
			m_choice2->SetSelection(m_myChoice);
			
		}
	}
	//
	// End of test.


	switch (button_id)
	{ 		
		case 12:
		{								    				
			if (m_myChoice < 0)
			{				
			    wxMessageBox(_T("Please choose a new date"));
				return;				
			}
			else
			{ 	
			back_id = 11;
			next_id = 0;
			m_choice2->SetSelection(m_myChoice);
			st_mydate = m_choice2->GetString(m_myChoice);
			m_dt.ParseDateTime(st_mydate);
			m_dt.Add(wxTimeSpan::Hours(6));
			s = m_dt.Format( _T ( "%a %d %b %Y %H:%M "));
			break;				
			}
		}
		case 0:						
		{					       
			back_id = 12;
			next_id = 1;
			m_myChoice --;

			myDateSelection = m_choice2->GetSelection();
			st_mydate = m_choice2->GetString(myDateSelection);
			m_dt.ParseDateTime(st_mydate);
			m_dt.Subtract(wxTimeSpan::Hours(6));
			s = m_dt.Format( _T ( "%a %d %b %Y %H:%M "));

			if (myDateSelection > 0)
			{
			m_choice2->SetSelection(myDateSelection - 1);			
			}
			

			break;			
		}
		default:
		{
			back_id --;		    
			myDateSelection = m_choice2->GetSelection();
			st_mydate = m_choice2->GetString(myDateSelection);
			m_dt.ParseDateTime(st_mydate);
			m_dt.Subtract(wxTimeSpan::Hours(6));
			m_dt.Add(wxTimeSpan::Hours(button_id));
			s = m_dt.Format( _T ( "%a %d %b %Y %H:%M "));
			next_id = button_id + 1; // to make change from forward to back work
		}		
	} // End switch	 
	
	if (st_mydate == _T(""))
		m_staticText2->SetLabel(_T(""));
	else
		m_staticText2->SetLabel(s);

	m_staticText211->SetLabel(label_array[button_id]);	
	RequestRefresh( pParent );	

}

void tcurrentUIDialog::OnNext( wxCommandEvent& event )
{
   
	wxString s;
	wxString st_mydate;

	//CreateFileArray();

	button_id = next_id;

	// Test if we have gone beyond the current list of HW.
	//
	int c = m_choice2->GetCount();
	myDateSelection = m_choice2->GetSelection();
	st_mydate = m_choice2->GetString(myDateSelection);
	wxDateTime m_testDT;
	m_testDT.ParseDateTime(st_mydate);
	m_testDT.Add(wxTimeSpan::Hours(6));

	wxDateTime myDate;
	wxDateSpan myOneDay = wxDateSpan::Days(1);
    
	int n = m_choice2->GetCount();
	
	if (m_myChoice >= n)
	{	
		if (st_mydate == _T(""))
		{
			m_myChoice = 0;
		}
		else		
		{
			myDate = m_datePicker1->GetValue();	
	
			myDate.Add(myOneDay);
			m_datePicker1->SetValue(myDate);
			
			int p = m_choice1->GetSelection();		// Get the port selected
			wxString s = m_choice1->GetString(p);
			m_portXML = FindPortXMLUsingChoice(s);
			int f = FindPortIDUsingChoice(s);
			
			CalcHW(f);			

			m_myChoice = 0;
			m_choice2->SetSelection(m_myChoice);
		}
		
	}
	//
	// End of test.


	switch (button_id)
	{ 		
		case 12:
		{								    				
			    next_id = 0;
			    m_myChoice++;
			    myDateSelection = m_choice2->GetSelection();
				st_mydate = m_choice2->GetString(myDateSelection);
				m_dt.ParseDateTime(st_mydate);
				m_dt.Add(wxTimeSpan::Hours(6));
				s = m_dt.Format( _T ( "%a %d %b %Y %H:%M "));
			    break;			    
		}
		case 0:						
		{					    			        
				int c = m_choice2->GetCount();								
				if (m_myChoice + 1 > c)
				{
					if (st_mydate != _T(""))
					{
					wxMessageBox(_T("Please choose a new date"));
					return;
					}
				}
				else
				{ 				
				next_id = 1;
				m_choice2->SetSelection(m_myChoice);
				st_mydate = m_choice2->GetString(m_myChoice);
				m_dt.ParseDateTime(st_mydate);
				m_dt.Subtract(wxTimeSpan::Hours(6));
				s = m_dt.Format( _T ( "%a %d %b %Y %H:%M "));           
				break;
				}
		}

		default:
		{
			next_id ++;		    
			myDateSelection = m_choice2->GetSelection();
			st_mydate = m_choice2->GetString(myDateSelection);
			m_dt.ParseDateTime(st_mydate);
			m_dt.Subtract(wxTimeSpan::Hours(6));
			m_dt.Add(wxTimeSpan::Hours(button_id));
			s = m_dt.Format( _T ( "%a %d %b %Y %H:%M "));
			back_id = next_id - 2;  // to make the forward back work
		}		
	} // End switch	    

	if (st_mydate == _T(""))
		m_staticText2->SetLabel(_T(""));
	else
		m_staticText2->SetLabel(s);
	
	m_staticText211->SetLabel(label_array[button_id]);

	RequestRefresh( pParent );
}


void tcurrentUIDialog::About(wxCommandEvent& event)
{
       wxMessageBox(wxString::Format(
_T("Tidal Data for UKHO Tidal Diamonds\n")
_T("--------------------------------------------------------------\n")
_T("The standard OpenCPN distribution has tidal data for the\n")
_T("following ports, which this plugin uses:\n")
_T("\n")
_T("PLYMOUTH (DEVONPORT)\n")   
_T("PORTSMOUTH\n")
_T("DOVER\n")
_T("SHEERNESS\n")  
_T("LOWESTOFT\n") 
_T("IMMINGHAM\n")
_T("LEITH\n") 
_T("ABERDEEN\n")
_T("WICK\n")   
_T("LERWICK\n")
_T("ULLAPOOL\n")
_T("LIVERPOOL (GLADSTONE DOCK)\n")
_T("HOLYHEAD\n")
_T("MILFORD HAVEN\n")
_T("PORT OF BRISTOL (AVONMOUTH)\n")  
_T("ST. HELIER\n")
_T("\n")
_T("Use this data with caution.\n")
_T("Use in conjunction with UKHO Tidal Stream Atlases and tidal diamonds\n")
_T("\n")                
_T("--------------------------------------------------------------------\n")
_T("\n")
_T("Note: 1 Rates shown are for a position corresponding to the centre\n")
_T("of the base of the arrow. Tidal rate is shown as knots.\n")
_T("Note: 2 Rates are calculated by using the method shown in UKHO Tidal\n")
_T("Stream Atlases. This is based on the average range for the day\n")
     ), _T("About Tidal Arrows"), wxOK | wxICON_INFORMATION, this);
}

bool tcurrentUIDialog::GetFile(wxString fileID){

	wxString xpath;
    xpath = wxStandardPaths::Get().GetPluginsDir();
	xpath = xpath + _T("\\plugins\\server.exe -f ");
	//wxMessageBox(xpath);
	wxString cmd =  xpath; // _("c:\\old\\server.exe -f ");
	cmd = cmd + _T(" ") + fileID;

    if ( !cmd )
        return false;

    int code = wxExecute(cmd, wxEXEC_ASYNC);

		wxString hostname =  _("localhost");
        wxIPV4address addr;   
        addr.Hostname(hostname);   
        addr.Service(5001);   
       
        // Create the socket   
        wxSocketClient* Socket = new wxSocketClient();   
       
        // Set up the event handler and subscribe to most events   
        Socket->SetEventHandler(*this, SOCKET_ID);   
       // Socket->SetNotify(wxSOCKET_CONNECTION_FLAG);   
       // Socket->Notify(true);   
       
        // Wait for the connection event   
        Socket->Connect(addr, true);   		
		Socket->WaitOnConnect(0, 200);
		
		return true;
}



void tcurrentUIDialog::OnSocketEvent(wxSocketEvent& event)   
    {   
		
        // All we need is the connection, that's the green light to proceed   
        if (event.GetSocketEvent() == wxSOCKET_CONNECTION)   
        {             

            CFileReceiveThread(wxT(""), event.GetSocket());					
        }   
    }
