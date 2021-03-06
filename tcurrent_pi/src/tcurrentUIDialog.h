/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  tcurrent Plugin Friends
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
 */

#ifndef __tcurrentUIDIALOG_H__
#define __tcurrentUIDIALOG_H__

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers
#include <wx/fileconf.h>
#include <wx/glcanvas.h>

#include "tcurrentUIDialogBase.h"
//#include "tcurrentSettingsDialog.h"
//#include "tcurrentReader.h"
//#include "tcurrentRecordSet.h"
#include "tinyxml.h"
#include <wx/progdlg.h>
#include <list>
#include <vector>

#include "tcmgr.h"
#include "wx/dateevt.h"
#include "wx/stattext.h"
#include "ocpn_plugin.h"

using namespace std;

#ifndef PI
#define PI        3.1415926535897931160E0      /* pi */
#endif

#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double*)&lNaN)
#endif

#define RT_RCDATA2           MAKEINTRESOURCE(999)

class tcurrentOverlayFactory;
class PlugIn_ViewPort;
class PositionRecordSet;


class wxFileConfig;
class tcurrent_pi;
class wxGraphicsContext;

class Position
{
public:
	double latD, lonD;
    wxString lat, lon;
	wxString stat_num, port_num;
	wxString minus_plus[12];
		
	wxString minus_6, minus_5, minus_4, minus_3 ,minus_2, minus_1, zero;
	wxString plus_1, plus_2,  plus_3, plus_4, plus_5, plus_6;
    Position *prev, *next; /* doubly linked circular list of positions */
};

class PortTides
{
public:

	wxString m_portID, m_portName, m_IDX;
	double m_spRange, m_npRange;

};

class StandardPort
{
public:
	wxString PORT_NUMBER,PORT_NAME,MEAN_SPRING_RANGE,MEAN_NEAP_RANGE,EXTRA,IDX;
};


class tcurrentUIDialog: public tcurrentUIDialogBase {
public:

    tcurrentUIDialog(wxWindow *parent, tcurrent_pi *ppi);
    ~tcurrentUIDialog();

    void OpenFile( bool newestFile = false );
    
    void SetCursorLatLon( double lat, double lon );
    void SetFactoryOptions( bool set_val = false );

    void SetViewPort( PlugIn_ViewPort *vp );
	PlugIn_ViewPort *vp;

	vector<Position> my_positions;
	vector<StandardPort> my_ports;
	wxString wxPortName[100][5];
	wxString selectedPort;

	vector<Position> my_points;
	bool mBool;
	vector<Position> OnRecord();
	void SetFromHW(int fromHW);

	int button_id;
	wxString m_portXML;
	double myRange;
	PortTides PopulatePortTides(wxString PortName);
	double CalcCurrent(double m_spRange, double m_npRange, double m_spRateDiamond, double m_npRateDiamond, double m_rangeOnDay);
	int CalcHoursFromHWNow();
	wxString nearestHW[8];
	int tcurrentUIDialog::round(double c);

	bool m_bUseRate;    
	bool m_bUseDirection; 
	bool m_bUseFillColour;

	wxString m_PortSelected;


private:
    void OnClose( wxCloseEvent& event );
    void OnMove( wxMoveEvent& event );
    void OnSize( wxSizeEvent& event );

	void OnStartSetupHW();
	void OnNow( wxCommandEvent& event );

	void CalcHW(int PortCode);
	void SetCorrectHWSelection();
	void OnDateSelChanged(wxDateEvent& event);
	void OnPortChanged(wxCommandEvent& event);
	void SetDateForNowButton();
	
	wxString FindPortXMLUsingChoice(wxString inPortName);
	int  FindPortIDUsingChoice(wxString inPortName);
	void LoadHarmonics();
	int  FindPortID(wxString myPort);
	bool LoadStandardPorts();
	bool OpenXML();

    void OnChooseTideButton(wxCommandEvent & event);
	void OnPrev( wxCommandEvent& event );
    void OnNext( wxCommandEvent& event );
	void About(wxCommandEvent& event);

    //    Data
    wxWindow *pParent;
    tcurrent_pi *pPlugIn;

    PlugIn_ViewPort  *m_vp;
    int m_lastdatatype;

    double m_cursor_lat, m_cursor_lon;

	int next_id;
	int back_id;
				// For the tide bit
	//wxDateTime m_dt;
	int m_myChoice;
	wxString	label_array[13];
	float       tcv[26];

	wxString         g_SData_Locn;
	TCMgr           *ptcmgr;
	wxString        *pTC_Dir;

	int         m_corr_mins;
    wxString    m_stz;
    int         m_t_graphday_00_at_station;
    wxDateTime  m_graphday;
    int         m_plot_y_offset;

	int myDateSelection;
	int myNewDateSelection;
	wxString    euTC[8][3];  //Date.Time, Height, Units, HW.LW
	wxDateTime m_dt;
	wxTimeSpan  m_ts;

	bool isNowButton;
};

#endif

